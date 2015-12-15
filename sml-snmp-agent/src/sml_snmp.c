/*
Copyright (c) 2011, Fraunhofer FOKUS - Fraunhofer Institute for Open Communication Systems FOKUS
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name of Fraunhofer FOKUS - Fraunhofer Institute for Open Communication Systems FOKUS
nor the names of its contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

///////////////////////////////////////////////////////////////////////
//  Module:			main.c
//  Author:		Andreas Bartusch <xelean@googlemail.com>
//  Contact@FOKUS:	bjoern.riemer@fokus.fraunhofer.de
//  Purpose:		
//  Description:	Provides the main control logic
//  Date:			22.02.2011
////////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "snmp.h"

// globals
extern pthread_mutex_t value_mutex;
extern unsigned int counter_tarif0;
extern unsigned int counter_tarif1;
extern unsigned int counter_tarif2;

char community_read[]="public";
char community_write[]="private";
time_t startup_time;

char oid[30][30] = { "1.3.6.1.2.1.1.1.0", "1.3.6.1.2.1.1.3.0", "1.3.6.1.4.1.39241.1.1.0",
	"1.3.6.1.4.1.39241.1.2.0", "1.3.6.1.4.1.39241.1.3.0", 
        "1.3.6.1.4.1.39241.1.8.0", "1.3.6.1.4.1.39241.1.8.1", "1.3.6.1.4.1.39241.1.8.2"
};
char description[]="volkszaehler.org / DAI Labor Berlin / Frauenhofer FOKUS";
char MasterName[]="libSML Masteragent";
char MasterLocation[]="Stromkasten";
unsigned int NumberOfAgents=1;
//functions

void debugg(unsigned char* packet,int length) {
	int position=0;
	int width=16;
	int i;
	char displayNumb[width];
	char displayChar[width];
	while (position<length) {
		for (i=0; i<width;i++) {
			displayNumb[i]=0x00;
			displayChar[i]=0x00;
		}
		for (i=0; i<(width); i++) {

			char temp;
			temp=packet[position++];
			displayNumb[i]=temp;
			if (temp<32) {
				temp='-';
			};
			displayChar[i]=temp;
			if (position>=length) {
				i=width;
			}
		}
		for (i=0; i<(width); i++) {
			printf("%02X", displayNumb[i]);
			if(i<(width-1)){printf(" ");}
		}
		printf(" -->   ");
		for (i=0;i< (width); i++) {
			printf("%c", displayChar[i]);
		}
		printf("\n");
	}
}

void process_varbind_list(struct varbind_list_rx* varbind_list){
	int i,timeticks;
	time_t t;
	printf("Try to lock mutex ...\n");
	pthread_mutex_lock( &value_mutex );
	printf("Locked mutex ...\n");
	for(i=0;i<varbind_list->varbind_idx;i++){

		if(!strcmp(( char* )&oid[0][0],( char* )varbind_list->varbind_list[i]->oid)){
			update_varbind(varbind_list->varbind_list[i],0x04,description);
		}
		if(!strcmp(( char* )&oid[1][0],( char* )varbind_list->varbind_list[i]->oid)){
			timeticks=(unsigned int)((time(&t)-startup_time)*100);
			update_varbind(varbind_list->varbind_list[i],0x043,&timeticks);
		}
		if(!strcmp(( char* )&oid[2][0],( char* )varbind_list->varbind_list[i]->oid)){
			update_varbind(varbind_list->varbind_list[i],0x04,MasterName);
		}
		if(!strcmp(( char* )&oid[3][0],( char* )varbind_list->varbind_list[i]->oid)){
			update_varbind(varbind_list->varbind_list[i],0x04,MasterLocation);
		}
		if(!strcmp(( char* )&oid[4][0],( char* )varbind_list->varbind_list[i]->oid)){
			update_varbind(varbind_list->varbind_list[i],0x02,&NumberOfAgents);
		}

		if(!strcmp(( char* )&oid[5][0],( char* )varbind_list->varbind_list[i]->oid)){
			printf("SNMP Request Counter Tarif 0: %d\n",counter_tarif0);
			update_varbind(varbind_list->varbind_list[i],0x02,&counter_tarif0);
		}
		if(!strcmp(( char* )&oid[6][0],( char* )varbind_list->varbind_list[i]->oid)){
			printf("SNMP Request Counter Tarif 1: %d\n",counter_tarif1);
			update_varbind(varbind_list->varbind_list[i],0x02,&counter_tarif1);
		}
		if(!strcmp(( char* )&oid[7][0],( char* )varbind_list->varbind_list[i]->oid)){
			printf("SNMP Request Counter Tarif 2: %d\n",counter_tarif2);
			update_varbind(varbind_list->varbind_list[i],0x02,&counter_tarif2);
		}
	}
	printf("Try to unlock mutex ...\n");
	pthread_mutex_unlock( &value_mutex );
	printf("Unlocked mutex ...\n");
}

void sendPacket(struct in_addr host,short port,int sock, struct snmp_message_tx* snmp_msg){
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr =host;
	bzero(&(server_addr.sin_zero),8);
	sendto(sock, snmp_msg->snmp_message, snmp_msg->snmp_message_len, 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	//printf("\nsend snmp-message to %s , %d \n",inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port));
	debugg(snmp_msg->snmp_message,snmp_msg->snmp_message_len);
}

void *snmp_agent(void *snmp_port){
	time(&startup_time);
	int sock;
	int bytes_read;
	socklen_t addr_len;
	char recv_data[1024];
	struct sockaddr_in server_addr , client_addr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("Socket");
		exit(1);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(161); 
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero),8);
	if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
		perror("Bind");
		exit(1);
	}
	addr_len = sizeof(struct sockaddr);
	fflush(stdout);
	while (1){
		bytes_read = recvfrom(sock,recv_data,1024,0,(struct sockaddr *)&client_addr, &addr_len);
		recv_data[bytes_read] = '\0';
		struct snmp_message_rx* snmp_msg=create_snmp_message_rx((unsigned char *)& recv_data[0]);
		if(snmp_msg!=NULL){
			disp_snmp_message_rx(snmp_msg);
			struct snmp_pdu_rx* snmp_pdu=create_snmp_pdu_rx(snmp_msg->snmp_pdu);
			if(snmp_pdu!=NULL){
				disp_snmp_pdu_rx(snmp_pdu);
				struct varbind_list_rx* varbind_list=create_varbind_list_rx(snmp_pdu->varbindings);
				disp_varbind_list_rx(varbind_list);
				unsigned int a=5; //testing
				struct varbind* varbind=create_varbind(varbind_list->varbind_list[0]->oid,0x02, &a);
				disp_varbind(varbind);
				process_varbind_list(varbind_list);
				disp_varbind_list_rx(varbind_list);
				struct varbind_list_tx* varbind_list_to_send=create_varbind_list_tx(varbind_list);
				struct snmp_pdu_tx* snmp_pdu_tx=create_snmp_pdu_tx(0xa2,snmp_pdu->request_id,0x00,0x00, varbind_list_to_send);
				struct snmp_message_tx* snmp_msg_tx= create_snmp_message_tx(snmp_msg->community, snmp_pdu_tx);
				sendPacket(client_addr.sin_addr,ntohs(client_addr.sin_port),sock, snmp_msg_tx);
				clr_snmp_message_tx(snmp_msg_tx);
				clr_snmp_pdu_tx(snmp_pdu_tx);
				clr_varbind_list_tx(varbind_list_to_send);
				clr_varbind(varbind);
				clr_varbind_list_rx(varbind_list);
				clr_snmp_pdu_rx(snmp_pdu);
			}
			clr_snmp_message_rx(snmp_msg);
		}
		fflush(stdout);
	}
	return 0;
}
