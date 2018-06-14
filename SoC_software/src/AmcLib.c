
#include "AmcLib.h"
#include <errno.h>
#include <sys/time.h>


//Gestion du DCS
void TCPAction(uint32_t client_message[10000], int read_size, void *virtual_base, int client_sock)
{
  int i;
  TCP_msg* tcp = (TCP_msg*) client_message;
  uint32_t data,mess_rdy_reg;

  printf("Received message : ");



  for (  i = 0; i < tcp->nbarg; i++ ) {
    if (i==0) {
      data = tcp->args16[i] | 1<<16; } //Export command (CTRL_ENA="01")
    else {
      data = tcp->args16[i];
    }


    alt_write_word(( virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_HPS_TO_FPGA_IN_BASE ) & ( uint32_t )( HW_REGS_MASK ) )), data );
    printf("%x ",data);
  }

  //Reset de la FIFO daq quand on est pas en train d'acquérir des données
  if (tcp->cmd==0x65 && tcp->args16[1]==0x9)
  {
    printf("\nStart Daq, fifo daq reset disable.\n");
    alt_write_word(( virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + MESSAGE_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) )), 4 );
  }
  else if (tcp->cmd==0x65 && tcp->args16[1]==0xA)
  {
    printf("\nStop Daq, fifo daq reset enable.\n");
    alt_write_word(( virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + MESSAGE_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) )), 0 );
  }


  //Déclenche la lecture de la FIFO DCS (ecriture du registre "message_ready" dans le fpga)
  mess_rdy_reg=alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + MESSAGE_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));
  alt_write_word(( virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + MESSAGE_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) )), 1 | (0xFFFFFFFE & mess_rdy_reg) );
  alt_write_word(( virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + MESSAGE_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) )), 0 | (0xFFFFFFFE & mess_rdy_reg));

  printf("\n\nReading FIFO :\n");

  //Attente de données dans la FIFO DCS + timeout si pas de réponse
  struct timeval start,current;
  gettimeofday(&start, NULL);
  while( (alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_CSR_BASE ) & ( uint32_t )( HW_REGS_MASK ) )))) == 0 ) {
    gettimeofday(&current, NULL);
    if ((int) (((current.tv_sec*1000000)+current.tv_usec) - ((start.tv_sec*1000000)+start.tv_usec ) ) > 5000000 ) {
      printf("DCS time out. No response from ASM.\n");
      break;
    }
  }

//Lecture des données DCS renvoyées par l'ASM
  i=0;
  while( (alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_CSR_BASE ) & ( uint32_t )( HW_REGS_MASK ) )))) > 0 ) {
  tcp->args16[i] = alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));
  printf( "%x ",(int) tcp->args16[i] );
  i++;
  }

//Modification du header pour les données DCS renvoyées vers le PC
  tcp->nbarg=i;
  tcp->stat=alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_CSR_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));
  int size = 4*(i+6);
  write(client_sock , tcp , size);

}	// TCP_Action


//Utilisé uniquement dans la version1, inutile si on utilise le sgdma
void UDPAction(void *virtual_base, int socket_udp, struct sockaddr_in serv_udp )
{
  uint32_t fifo_lvl;
  uint32_t data;
  uint16_t buffer_send[18000];
  int packet_size, i,newsend;
  int shift=3;

  fifo_lvl=alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_F2H_DAQ_OUT_CSR_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));

    buffer_send[0] = 0x3012;
    buffer_send[1] = 0x0000;
    buffer_send[2] = 0x0000;

    newsend=0;
    if (fifo_lvl!=0) {
      for (i=shift ; i < sizeof(buffer_send) ;i=i+2) {
        data = alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_F2H_DAQ_OUT_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));


        buffer_send[i+1] = (data >> 16) & 0xFFFF ;
        buffer_send[i] = data & 0xFFFF;

        //SWAP BYTE ORDER
        buffer_send[i]=  ((buffer_send[i] & 0x00FF) << 8) | ((buffer_send[i] & 0xFF00) >> 8);
        buffer_send[i+1]=  ((buffer_send[i+1] & 0x00FF) << 8) | ((buffer_send[i+1] & 0xFF00) >> 8);

        packet_size = i+2;

        if (((buffer_send[i] == 0xFB10) || (buffer_send[i+1] == 0xFB10)) && (i>36))  {

          if (((buffer_send[i] >>16) == 0xFE10) || ((buffer_send[i+1] >>16) == 0xFE10)) {
            shift=4;
            buffer_send[3]=0xFE10;
          }
          else {
            shift=3;
          }
          newsend=1;
          break;
        }
      }
    }

    if (fifo_lvl>0  && newsend==1 ) {
        if(sendto( socket_udp, buffer_send, packet_size*2, 0, (struct sockaddr *)&serv_udp, sizeof(serv_udp) ) < 0 )
        {
            printf( "Send error : %s \n", strerror(errno));
        }
    }




}
