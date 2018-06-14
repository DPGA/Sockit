#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "hwlib.h"
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "hps_ARM.h"


#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/poll.h>

#include "AmcLib.c"
#include "sgdma.h"
#include "dma.h"


int main() {
	//struct timeval begin, end; //for debug

// Mapping des mémoires
	void *virtual_base, *axi_virtual_base, *sdram_64MB_add,*sdram_16MB_add;
	int fd;

	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}

	//HPS TO FPGA LIGHTWEIGHT BRIDGE
	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );
	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	//HPS TO FPGA BRIDGE
	axi_virtual_base = mmap( NULL, HW_FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd,ALT_AXI_FPGASLVS_OFST );
  if( axi_virtual_base == MAP_FAILED ) {
        printf( "ERROR: axi mmap() failed...\n" );
        close( fd );
        return( 1 );
    }

	//SHARED SDRAM 64MB & 16MB (pour les descripteurs)
	sdram_64MB_add=mmap( NULL, SDRAM_64_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, SDRAM_64_BASE );
	sdram_16MB_add=mmap( NULL, SDRAM_16_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, SDRAM_16_BASE );
	if( sdram_64MB_add == MAP_FAILED ) {
				printf( "ERROR: axi mmap() failed...\n" );
				close( fd );
				return( 1 );
	}



// Déclaration des sockets TCP et UDP
   int socket_desc , client_sock , c , read_size=1;
   struct sockaddr_in server , client;

   //Create socket
   socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	 if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	printf("setsockopt(SO_REUSEADDR) failed");
   if (socket_desc == -1)
   {
      printf("Could not create socket");
   }

   //Prepare the sockaddr_in structure
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port = htons( 8888 );

   //Bind
   if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
   {
      perror("bind failed. Error");
      return 1;
   }
   puts("\nBind done.\n");
	 listen(socket_desc , 3);

	 //Pour aller voir si il y a des données dans la socket TCP
	 int rv=0;
	 struct pollfd ufds[1];
	 ufds[0].fd=socket_desc;
	 ufds[0].events=POLLIN;

	 //Create UDP socket
	 int socket_udp = socket(AF_INET , SOCK_DGRAM , 0);
   //Set UPD server infos (PC side)
   struct sockaddr_in serv_udp;
   serv_udp.sin_family = AF_INET;
   serv_udp.sin_addr.s_addr = inet_addr("192.168.2.1");
   serv_udp.sin_port = htons( 60000 );
	 uint32_t client_message[10000];
	 int res = 0;

 // Change buffer size
 socklen_t optlen;
 int sendbuff;
 optlen = sizeof(sendbuff);
 res = getsockopt(socket_udp, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);

 if(res == -1)
     printf("Error getsockopt one");
 else
     printf("send buffer size = %d\n", sendbuff);

 sendbuff = 1000000000;
 printf("sets the send buffer to %d\n (max possible)", sendbuff);
 res = setsockopt(socket_udp, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff));
 res = getsockopt(socket_udp, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);

	 if(res == -1)
	     printf("Error getsockopt one");
	 else
	     printf("send buffer size = %d\n", sendbuff);

//Déclaration des espaces mémoires pour le sgdma
	 //create descriptors in the mapped memory
	 struct alt_avalon_sgdma_packed  *sgdma_desc1=sdram_16MB_add;
	 struct alt_avalon_sgdma_packed  *sgdma_desc2=sdram_16MB_add+sizeof(struct alt_avalon_sgdma_packed);
	 struct alt_avalon_sgdma_packed  *sgdma_desc_empty=sdram_16MB_add+3*sizeof(struct alt_avalon_sgdma_packed);

	 //Address to the physical space
	 void* sgdma_desc1_phys=(void*)SDRAM_16_BASE;
	 void* sgdma_desc2_phys=(void*)SDRAM_16_BASE+sizeof(struct alt_avalon_sgdma_packed);
	 void* sgdma_desc_empty_phys=(void*)SDRAM_16_BASE+3*sizeof(struct alt_avalon_sgdma_packed);

	 //create a pointer to the DMA controller base
	 h2p_lw_sgdma_addr=virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + SGDMA_0_BASE ) & ( unsigned long)( HW_REGS_MASK ) );

//Ecriture des headers de trames dans les espaces mémoires juste avant les espaces mémoires des données
	 uint32_t offset = 0x100000;
	 int i;
	 for (i=0;i<2;i++){
			 *((uint32_t *) (sdram_64MB_add-6+(i*offset)))=0x3012;
			 *((uint32_t *) (sdram_64MB_add-4+(i*offset)))=0x00000000;
	 }


	 //Clear de la FIFO du DCS
	 int j=0;
	 while( (alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_CSR_BASE ) & ( uint32_t )( HW_REGS_MASK ) )))) > 0 ) {
   alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));
	 j++;
 	 }
	 printf("DCS FIFO CLEAR (%i values)",j);

// Fin de l'inititalisation
	 while(1){

	 //Regarde si il y a des datas dans la socket TCP
	 rv = poll(ufds,1,0);
	 if (rv == -1) {
		    printf("Poll error");
		}
		// Pas de données DCS détectés (socket TCP vide)
		else if (rv == 0 ) {

//Utilisé dans la Version1, lecture d'une FIFO dans le FPGA (temps d'accès très long)  => 32bits
			/*//Read DAQ FIFO and send UDP message
			int i;
			for (i=0;i<1000;i++) UDPAction(virtual_base,socket_udp, serv_udp);*/


//Utilisé dans la Version1, lecture d'une FIFO dans le FPGA (temps d'accès très long) => 64bits
			/*//READ 64BITS FIFO VIA 64 BITS AXI BUS
			uint32_t fifo_64b;
	 	  fifo_64b=alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_F2H_DAQ_64B_OUT_CSR_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));
	 	  //printf("fifolvl=%x ", fifo_64b);
			uint32_t value;
				if (fifo_64b!=0) {
					for (i=1;i<fifo_64b/2;i++) {
					value=alt_read_word((axi_virtual_base + ( ( unsigned long )(FIFO_F2H_DAQ_64B_OUT_BASE ) & ( unsigned long )( HW_FPGA_AXI_MASK ) )));
					value=value;
					//printf("value= %x\n", value);
					//printf("value= %" PRIx64 " \n", value);
					}
				}*/

			//Initialisation des descripteurs pour le sgdma : 2 descripteurs (un pour chaque demi DRS). Le descripteur vide sert à marquer la fin de la chaîne.
			 initDescriptor_STtoMM(sgdma_desc1, (void*) (SDRAM_64_BASE),
 						 sgdma_desc2_phys,0,
 						 (_SGDMA_DESC_CTRMAP_WRITE_FIXED_ADDRESS | _SGDMA_DESC_CTRMAP_OWNED_BY_HW));

 			 initDescriptor_STtoMM(sgdma_desc2, (void*) (SDRAM_64_BASE+offset),
 						 sgdma_desc_empty_phys,0,
 						 (_SGDMA_DESC_CTRMAP_WRITE_FIXED_ADDRESS | _SGDMA_DESC_CTRMAP_OWNED_BY_HW));

			 initDescriptor_STtoMM(sgdma_desc_empty,NULL,
						 sgdma_desc_empty_phys,0,
						 (_SGDMA_DESC_CTRMAP_WRITE_FIXED_ADDRESS));


						//Inititalisation du sgdma
						init_sgdma(_SGDMA_CTR_IE_CHAIN_COMPLETED);
						//Adresse du premier descripteur de la chaine
						setDescriptor(sgdma_desc1_phys);
						//Debut du transfert
						setControlReg(_SGDMA_CTR_IE_CHAIN_COMPLETED|_SGDMA_CTR_RUN);
						//Attente de la fin du transfert
						waitFinish();
						//Arret du run
						setControlReg(_SGDMA_CTR_IE_CHAIN_COMPLETED);

						int packet_size = sgdma_desc1->actual_bytes_transferred;
						//if (packet_size > 0) printf("size=%i nb=%x\n",packet_size, *((uint32_t *) (sdram_64MB_add+13) ));
						//gettimeofday(&begin, NULL);
						if (packet_size!=0) {
								for (i=0;i<2;i++) {

									if(sendto( socket_udp, sdram_64MB_add+(i*offset)-6, packet_size+6, 0, (struct sockaddr *)&serv_udp, sizeof(serv_udp) ) < 0 )
										{printf( "Send error : %s \n", strerror(errno));}
									}

						}
						//gettimeofday(&end, NULL);
						//double elapsed = (end.tv_sec - begin.tv_sec) +   ((end.tv_usec - begin.tv_usec)/1000000.0);
						//if (elapsed > 0.001) printf("%f\n",elapsed);


		//Données DCS détectées (TCP socket pas vide)
		} else {


			 printf("\nDCS data detected !\n");
		   puts("Waiting for incoming connections...");
		   c = sizeof(struct sockaddr_in);

		   //Accepte la connection du client
		   client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		   if (client_sock < 0)
		   {
		      puts("accept failed");
		      return 1;
		   }
		   puts("Connection accepted");

		  //Réception du message du client
		  while( (read_size = recv(client_sock , client_message , 1600 , 0)) > 0 )
		   {
				 if (read_size < 1000) {
				 		TCPAction(client_message, read_size, virtual_base,client_sock);
				 }
				 else {
					 while( (alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_CSR_BASE ) & ( uint32_t )( HW_REGS_MASK ) )))) > 0 ) {
				   alt_read_word((virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIFO_FPGA_TO_HPS_OUT_BASE ) & ( uint32_t )( HW_REGS_MASK ) )));
					 j++;
				 	 }
				 }
			 }

		   if(read_size == 0)
		   {
		      puts("\nClient disconnected");
		      fflush(stdout);
		   }
		   else if(read_size == -1)
		   {
		      perror("recv failed");
		   }

	 }

	} //fin du while


	/// unmap à faire si on veut exit proprement

	return( 0 );
}
