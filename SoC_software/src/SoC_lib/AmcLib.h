#ifndef AMC_LIB_H_
#define AMC_LIB_H_


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

#define ALT_AXI_FPGASLVS_OFST (0xC0000000) // axi_master
#define HW_FPGA_AXI_SPAN (0x40000000) // Bridge span 1GB
#define HW_FPGA_AXI_MASK ( HW_FPGA_AXI_SPAN - 1 )

//SDRAM 32000000-35ffffff //64 MB
#define SDRAM_64_BASE 0x21000000
#define SDRAM_64_SPAN 0x3FFFFFF

//SDRAM 36000000-36ffffff //16 MB
#define SDRAM_16_BASE 0x25000000
#define SDRAM_16_SPAN 0xFFFFFF

#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>


#include <stdint.h>
#include <inttypes.h>

//=====================================
//=====================================
/*#define TCP_BUF8_LEN			(SSS_TX_BUF_SIZE)
#define TCP_BUF16_LEN			(TCP_BUF8_LEN >> 1)
#define TCP_BUF32_LEN			(TCP_BUF8_LEN >> 2)*/
#define TCP_NB_ARG_32			(2000)
#define TCP_NB_ARG_16			(TCP_NB_ARG_32 *2)

//=====================================
//=====================================
typedef union _TCP_MSG
{
	/*alt_u8		buf8[TCP_BUF8_LEN];
	alt_u16		buf16[TCP_BUF16_LEN];
	alt_u32		buf32[TCP_BUF32_LEN];*/
	struct {
		uint16_t indic;					// Frame Index
		uint16_t stat;					// Command Status
		uint16_t cmd;					// Command Number
		uint16_t nbarg;					// Number of arguments
		uint32_t mask;					// Destination Mask
		union {
			uint32_t args32[TCP_NB_ARG_32];	// list of arguments
			uint16_t args16[TCP_NB_ARG_16];
		};
	};
}TCP_msg;


void TCPAction           (uint32_t client_message[10000], int read_size, void *virtual_base, int client_sock);
void UDPAction           (void *virtual_base,int socket_udp, struct sockaddr_in serv_udp);

//=====================================
//=====================================
//=====================================
//=====================================

#endif /* AMC_LIB_H_ */
