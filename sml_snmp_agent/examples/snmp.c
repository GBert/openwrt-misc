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
//  Module:			snmp.c
//  Author:		Andreas Bartusch <xelean@googlemail.com>
//  Contact@FOKUS:	bjoern.riemer@fokus.fraunhofer.de
//  Purpose:		
//  Description:	Implementation of the SNMP v1 logic
//  Date:			22.02.2011
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snmp.h"

extern void debugg(unsigned char*,int );

unsigned char oid_item_length(unsigned int value) {
    if (       ! (value & 0xffffff80) ) { // 2 ^  7
        return 1;
    } else if (! (value & 0xffffc000) ) { // 2 ^ 14
        return 2;
    } else if (! (value & 0xfffe0000) ) { // 2 ^ 21
        return 3;
    } else if (! (value & 0xf0000000) ) { // 2 ^ 28
        return 4;
    } else {
        return 5;
    }
}

int decode_integer(unsigned char* array, unsigned int* pointer){
	unsigned int buffer;
	*pointer=*pointer+1;
	switch (array[*pointer]) {
	case 0x01:
		{
			*pointer=*pointer+1;
			buffer=array[*pointer];
		}
		break;
	case 0x02:
		{
			*pointer=*pointer+1;
			buffer=0|array[*pointer];
			buffer<<=8;
			*pointer=*pointer+1;
			buffer|=array[*pointer];
		}
		break;
	case 0x03:
		{
			*pointer=*pointer+1;
			buffer=0|array[*pointer];
			buffer<<=8;
			*pointer=*pointer+1;
			buffer|=array[*pointer];
			buffer<<=8;
			*pointer=*pointer+1;
			buffer|=array[*pointer];
		}
		break;
	case 0x04:
		{
			*pointer=*pointer+1;
			buffer=0|array[*pointer];
			buffer<<=8;
			*pointer=*pointer+1;
			buffer|=array[*pointer];
			buffer<<=8;
			*pointer=*pointer+1;
			buffer|=array[*pointer];
			buffer<<=8;
			*pointer=*pointer+1;
			buffer|=array[*pointer];
		}
		break;
	default:
		{
			buffer=04;
		}
		break;
	}
	*pointer=*pointer+1;
	return buffer;
}

unsigned char* decode_string(unsigned char* array, unsigned int* pointer){
	*pointer=*pointer+1;
	unsigned char* string=(unsigned char*)calloc(1,(array[*pointer])+1);
	memcpy(string, &array[*pointer+1],array[*pointer]);
	*pointer=*pointer+array[*pointer]+1;
	return string;
}

unsigned char* decode_oid(unsigned char* array, unsigned int* pointer){
	unsigned char* string=NULL;
	unsigned char* buffer=(unsigned char*)calloc(100,sizeof(unsigned char));
	unsigned char* number=(unsigned char*)calloc(6,sizeof(unsigned char));
	unsigned int oid_node,oid_node_long=0;
	*pointer=*pointer+1;
	unsigned char length=array[*pointer];
	*pointer=*pointer+1;
	if(array[*pointer]==0x2b){
		strcat(( char* )buffer, "1.3");
	}
	length--;
	while(length){
		*pointer=*pointer+1;
		oid_node=array[*pointer];
                if(oid_node & 0x80) {
			oid_node_long |= oid_node & 0x7f;
			oid_node_long  = oid_node_long << 7;
		} else {
			if (oid_node_long) {
				oid_node_long |= oid_node;
				sprintf((char*)number,".%d",oid_node_long);
				strcat((char*)buffer,(char*)number);
				oid_node_long=0;
			} else {
				sprintf((char*)number,".%d",oid_node);
				strcat((char*)buffer,(char*)number);
			}
		}
		length--;
	}
	string=(unsigned char*)calloc(1,strlen((char*)buffer)+1);
	strcpy((char*)string,(char*)buffer);
	*pointer=*pointer+1;
	free(buffer);
	free(number);
	return string;
}

unsigned char* encode_oid(unsigned char* array){
	unsigned char* oid=NULL;
	unsigned char* buffer=(unsigned char*)calloc(1,strlen((char*)array)+1);
	unsigned char* number=(unsigned char*)calloc(6,sizeof(unsigned char));
	unsigned int index=2;
	unsigned char* oid_buffer=(unsigned char*)calloc(1,strlen((char*)array)+1);
	unsigned int value=0;
	char *saveptr;
	strcpy((char*)buffer,(char*)array);
	char* part;

	part=strtok_r((char*)buffer,".",&saveptr);
	value=atoi(part)*40;
	part=strtok_r(NULL,".",&saveptr);
	value+=atoi(part);
	oid_buffer[index] = value;
        index++;

	for (part=strtok_r(NULL,".",&saveptr); part ; part=strtok_r(NULL,".",&saveptr)) {
		value=atoi(part);
		unsigned char length = oid_item_length(value);
		signed char j = 0;
		oid_buffer[index + length-1] = value & 0x7F;
		for (j = length - 2 ; j >= 0; j--) {
			value = value >> 7;
			oid_buffer[index + j] = ( value & 0x7F ) | 0x80 ;
		}
		index += length;
	}

	oid_buffer[0]=0x06;
	oid_buffer[1]=index-2;
	oid=(unsigned char*)calloc(1,index);
	memcpy(oid,oid_buffer,index);
	free(buffer);
	free(number);
	free(oid_buffer);
	return oid;
}

unsigned char* encode_string(unsigned char* array){
	unsigned char* string=(unsigned char*)calloc(1,strlen((char*)array)+2);
	memcpy(&string[2],array,strlen((char*)array));
	string[0]=0x4;
	string[1]=(unsigned char)(strlen((char*)array));
	return string;
}

unsigned char* encode_integer(unsigned int value){
	unsigned char* integer_string=NULL;
	if(value>65535){
		integer_string=(unsigned char*)calloc(6,sizeof(unsigned char));
		integer_string[0]=0x02;
		integer_string[1]=0x04;
		integer_string[2]=(unsigned char)(value>>24);
		integer_string[3]=(unsigned char)(value>>16);
		integer_string[4]=(unsigned char)(value>>8);
		integer_string[5]=(unsigned char)(value);
	}
	else if(value>255){
		integer_string=(unsigned char*)calloc(4,sizeof(unsigned char));
		integer_string[0]=0x02;
		integer_string[1]=0x02;
		integer_string[2]=(unsigned char)(value>>8);
		integer_string[3]=(unsigned char)(value);
	}
	else{
		integer_string=(unsigned char*)calloc(3,sizeof(unsigned char));
		integer_string[0]=0x02;
		integer_string[1]=0x01;
		integer_string[2]=(unsigned char)(value);
	}
	return integer_string;
}

unsigned char* encode_integer_by_length(unsigned int value, unsigned char length, unsigned char typ){
	unsigned char* integer_string=NULL;
	if(length>2){
		integer_string=(unsigned char*)calloc(6,sizeof(unsigned char));
		integer_string[0]=typ;
		integer_string[1]=0x04;
		integer_string[2]=(unsigned char)(value>>24);
		integer_string[3]=(unsigned char)(value>>16);
		integer_string[4]=(unsigned char)(value>>8);
		integer_string[5]=(unsigned char)(value);
	}
	else if(value>1){
		integer_string=(unsigned char*)calloc(4,sizeof(unsigned char));
		integer_string[0]=typ;
		integer_string[1]=0x02;
		integer_string[2]=(unsigned char)(value>>8);
		integer_string[3]=(unsigned char)(value);
	}
	else{
		integer_string=(unsigned char*)calloc(3,sizeof(unsigned char));
		integer_string[0]=typ;
		integer_string[1]=0x01;
		integer_string[2]=(unsigned char)(value);
	}
	return integer_string;
}

unsigned char* return_pdu_type_string(unsigned char pdu_type){
	char* pdu_type_string=NULL;
	char GetRequest[]={'G','e','t','R','e','q','u','e','s','t','\0'};
	char GetResponse[]={'G','e','t','R','e','s','p','o','n','s','e','\0'};
	char GetNext[]={'G','e','t','N','e','x','t','\0'};
	char SetRequest[]={'S','e','t','R','e','q','u','e','s','t','\0'};
	char Error[]={'w','r','o','n','g',0x20,'P','D','U','\0'};
	switch (pdu_type) {
	case 0xa0:
		{
			pdu_type_string=calloc(1,strlen(GetRequest)+1);
			strcpy(pdu_type_string,GetRequest);
		}
		break;
	case 0xa1:
		{
			pdu_type_string=calloc(1,strlen(GetNext)+1);
			strcpy(pdu_type_string,GetNext);
		}
		break;
	case 0xa2:
		{
			pdu_type_string=calloc(1,strlen(GetResponse)+1);
			strcpy(pdu_type_string,GetResponse);
		}
		break;
	case 0xa3:
		{
			pdu_type_string=calloc(1,strlen(SetRequest)+1);
			strcpy(pdu_type_string,SetRequest);
		}
		break;
	default:
		{
			pdu_type_string=calloc(1,strlen(Error)+1);
			strcpy(pdu_type_string,Error);
		}
		break;
	}
	return (unsigned char*)pdu_type_string;
}

unsigned char* return_data_type_string(unsigned char data_type){
	char* data_type_string=NULL;
	char Integer[]={'I','n','t','e','g','e','r','\0'};
	char OctetString[]={'O','c','t','e','t',0x20,'S','t','r','i','n','g','\0'};
	char Null[]={'N','u','l','l','\0'};
	char ObjectIdentifier[]={'O','b','j','e','c','t',0x20,'I','d','e','n','t','i','f','i','e','r','\0'};
	char Timeticks[]={'T','i','m','e','t','i','c','k','s','\0'};
	char Error[]={'E','r','r','o','r','\0'};
	switch (data_type) {
	case 0x02:
		{
			data_type_string=calloc(1,strlen((char*)Integer)+1);
			strcpy(data_type_string,Integer);
		}
		break;
	case 0x04:
		{
			data_type_string=calloc(1,strlen((char*)OctetString)+1);
			strcpy(data_type_string,OctetString);
		}
		break;
	case 0x05:
		{
			data_type_string=calloc(1,strlen(Null)+1);
			strcpy(data_type_string,Null);
		}
		break;
	case 0x06:
		{
			data_type_string=calloc(1,strlen((char*)ObjectIdentifier)+1);
			strcpy(data_type_string,ObjectIdentifier);
		}
		break;
	case 0x43:
		{
			data_type_string=calloc(1,strlen((char*)Integer)+1);
			strcpy(data_type_string,Timeticks);
		}
		break;
	default:
		{
			data_type_string=calloc(1,strlen((char*)Error)+1);
			strcpy(data_type_string,Error);
		}
		break;
	}
	return (unsigned char*)data_type_string;
}

void disp_snmp_message_rx(struct snmp_message_rx* snmp_msg){
	printf("***SNMP MESSAGE***\n");
	printf("SNMP Message Length: %i\n",snmp_msg->snmp_message_length);
	printf("SNMP Version: %d\n",snmp_msg->version);
	printf("Community String: %s\n",snmp_msg->community);
	printf("SNMP PDU Length: %i\n",snmp_msg->snmp_pdu_length);
	debugg(snmp_msg->snmp_pdu,snmp_msg->snmp_pdu_length+2);
}

void clr_snmp_message_rx(struct snmp_message_rx* snmp_msg){
	free(snmp_msg->snmp_pdu);
	free(snmp_msg->community);
	free(snmp_msg);
}

struct snmp_message_rx* create_snmp_message_rx(unsigned char* udp_received){
	unsigned int pointer=0;
	unsigned int buffer=0;
	struct snmp_message_rx* snmp_msg=NULL;
	if(udp_received[pointer]==0x30){
		snmp_msg=(struct snmp_message_rx*)calloc(1,sizeof(struct snmp_message_rx));
		pointer++;
		snmp_msg->snmp_message_length=udp_received[pointer];
		pointer++;
		if(udp_received[pointer]==0x02){
			buffer=decode_integer(&udp_received[0], &pointer);
			if(!buffer){snmp_msg->version=++buffer;}
			else{snmp_msg->version=0xff;}
		}
		if(udp_received[pointer]==0x04){
			snmp_msg->community=decode_string(&udp_received[0], &pointer);
		}
		if((udp_received[pointer]==0xa0)||(udp_received[pointer]==0xa1)||(udp_received[pointer]==0xa2)||(udp_received[pointer]==0xa3||(udp_received[pointer]==0xa5))){
			pointer++;
			snmp_msg->snmp_pdu_length=udp_received[pointer];
			snmp_msg->snmp_pdu=(unsigned char*)calloc(1,(snmp_msg->snmp_pdu_length)+1+2);
			memcpy(snmp_msg->snmp_pdu,&udp_received[pointer-1] ,snmp_msg->snmp_pdu_length+2);
		}
	}
	if(snmp_msg->version!=1){
		clr_snmp_message_rx(snmp_msg);
		printf("wrong protocol version\n");
		snmp_msg=NULL;
	}
	return snmp_msg;
}

void clr_snmp_pdu_rx(struct snmp_pdu_rx* snmp_pdu){
	free(snmp_pdu->varbindings);
	free(snmp_pdu);
}

struct snmp_pdu_rx* create_snmp_pdu_rx(unsigned char* pdu){
	unsigned int pointer=0;
	struct snmp_pdu_rx* snmp_pdu=NULL;
	if((pdu[pointer]==0xa0)||(pdu[pointer]==0xa1)||(pdu[pointer]==0xa2)||(pdu[pointer]==0xa3)){
		snmp_pdu=(struct snmp_pdu_rx*)calloc(1,sizeof(struct snmp_pdu_rx));
		snmp_pdu->snmp_pdu_type=pdu[pointer];
		pointer++;
		snmp_pdu->snmp_pdu_length=pdu[pointer];
		pointer++;
		if(pdu[pointer]==0x02){
			snmp_pdu->request_id=decode_integer(&pdu[0], &pointer);
		}
		if(pdu[pointer]==0x02){
			snmp_pdu->error=decode_integer(&pdu[0], &pointer);
		}
		if(pdu[pointer]==0x02){
			snmp_pdu->error_index=decode_integer(&pdu[0], &pointer);
		}
		if(pdu[pointer]==0x30){
			pointer++;
			snmp_pdu->varbindings_length=pdu[pointer];
			snmp_pdu->varbindings=(unsigned char*)calloc(1,(snmp_pdu->varbindings_length)+1+2);
			memcpy(snmp_pdu->varbindings,&pdu[pointer-1] ,snmp_pdu->varbindings_length+2);
		}
	}else{
		printf("wrong pdu\n");
	}
	return snmp_pdu;
}

void disp_snmp_pdu_rx(struct snmp_pdu_rx* snmp_pdu){
	unsigned char* pdu_type=return_pdu_type_string(snmp_pdu->snmp_pdu_type);
	printf("***SNMP PDU***\n");
	printf("SNMP PDU Type: %d (%s)\n",snmp_pdu->snmp_pdu_type,pdu_type);
	printf("SNMP PDU Length: %i\n",snmp_pdu->snmp_pdu_length);
	printf("Request ID: %d\n",snmp_pdu->request_id);
	printf("Error: %d\n",snmp_pdu->error);
	printf("Error Index: %d\n",snmp_pdu->error_index);
	printf("Varbindings Length: %i\n",snmp_pdu->varbindings_length);
	debugg(snmp_pdu->varbindings,snmp_pdu->varbindings_length+2);
	free(pdu_type);
}

struct varbind_list_rx* create_varbind_list_rx(unsigned char* varbindings){
	unsigned int pointer=0;
	int length=0;
	struct varbind_list_rx* varbind_list=NULL;
	if((varbindings[pointer]==0x30)){
		varbind_list=(struct varbind_list_rx*)calloc(1,sizeof(struct varbind_list_rx));
		varbind_list->varbind_idx=0;
		pointer++;
		varbind_list->varbind_list_length=varbindings[pointer];
		pointer++;
		while(length<varbind_list->varbind_list_length){
			if((varbindings[pointer]==0x30)){
				pointer++;
				length+=((varbindings[pointer])+2);
				pointer++;
				if((varbindings[pointer]==0x06)){		
					varbind_list->varbind_idx++;
					varbind_list->varbind_list=(struct varbind**)realloc(varbind_list->varbind_list,(varbind_list->varbind_idx)*sizeof(struct varbind*));
					varbind_list->varbind_list[(varbind_list->varbind_idx)-1]=(struct varbind*)calloc(1,sizeof(struct varbind));
					varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->oid=decode_oid(&varbindings[0], &pointer);
				}
				switch (varbindings[pointer]) {
				case 0x02:
					{
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->data_type=varbindings[pointer];
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->value=(unsigned int*)calloc(1, sizeof(unsigned int));
						*(unsigned int*)varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->value=decode_integer(&varbindings[0], &pointer);
					}
					break;
				case 0x04:
					{
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->data_type=varbindings[pointer];
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->value=decode_string(&varbindings[0], &pointer);
					}
					break;
				case 0x05:
					{
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->data_type=varbindings[pointer];
						pointer++;
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->value=(unsigned int*)calloc(1, sizeof(unsigned int));
						pointer++;
						*(unsigned int*)varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->value=0x00;
					}
					break;
				case 0x06:
					{
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->data_type=varbindings[pointer];
						varbind_list->varbind_list[(varbind_list->varbind_idx)-1]->value=decode_oid(&varbindings[0], &pointer);
					}
					break;
				default:
					{
					}
					break;
				}
			}
		}
	}
	return varbind_list;
}

void disp_varbind_list_rx(struct varbind_list_rx* varbind_list){
	unsigned int i;
	printf("***Varbind List***\n");
	printf("Varbind List Length: %i\n",varbind_list->varbind_list_length);
	for(i=0;i<varbind_list->varbind_idx;i++){
		printf("OID: %s, ",varbind_list->varbind_list[i]->oid);
		switch (varbind_list->varbind_list[i]->data_type) {
		case 0x02:
			{
				printf("Data Type: Integer, Value: %d\n",*(unsigned int*)varbind_list->varbind_list[i]->value);
			}
			break;
		case 0x04:
			{
				printf("Data Type: Octet String, Value: %s\n",(unsigned char*)varbind_list->varbind_list[i]->value);
			}
			break;
		case 0x05:
			{
				printf("Data Type: Null, Value: %d\n",*(unsigned int*)varbind_list->varbind_list[i]->value);
			}
			break;
		case 0x06:
			{
				printf("Data Type: Object Identifier, Value: %s\n",(unsigned char*)varbind_list->varbind_list[i]->value);
			}
			break;
		case 0x43:
			{
				printf("Data Type: Timeticks, Value: %d\n",*(unsigned int*)varbind_list->varbind_list[i]->value);
			}
			break;				
		default:
			{
			}
			break;
		}
	}
}

void clr_varbind_list_rx(struct varbind_list_rx* varbind_list){
	unsigned int i;
	for(i=(varbind_list->varbind_idx);i>0;i--){
		free(varbind_list->varbind_list[i-1]->value);
		free(varbind_list->varbind_list[i-1]);
	}
	free(varbind_list->varbind_list);
	free(varbind_list);
}

struct varbind* create_varbind(unsigned char* oid, unsigned char data_type, void* value){
	struct varbind* varbind=NULL;
	varbind=(struct varbind*)calloc(1,sizeof(struct varbind));
	varbind->oid=(unsigned char*)calloc(1,strlen((char*)oid));
	strcpy((char*)varbind->oid,(char*)oid);
	varbind->data_type=data_type;
	switch (data_type) {
	case 0x02:
		{
			varbind->value=(unsigned int*)calloc(1, sizeof(unsigned int));
			*(unsigned int*)varbind->value=*(unsigned int*)value;
		}
		break;
	case 0x04:
		{
			varbind->value=(unsigned char*)calloc(1, strlen((char*)value)+1);
			strcpy((char*)varbind->value,(char*)value);
		}
		break;
	case 0x05:
		{
			varbind->value=(unsigned int*)calloc(1, sizeof(unsigned int));
			*(unsigned int*)varbind->value=0x00;
		}
		break;
	case 0x06:
		{
			varbind->value=(unsigned char*)calloc(1, strlen((char*)value)+1);
			strcpy((char*)varbind->value,(char*)value);
		}
	case 0x043:
		{
			varbind->value=(unsigned int*)calloc(1, sizeof(unsigned int));
			*(unsigned int*)varbind->value=*(unsigned int*)value;
		}
		break;
	default:
		{
		}
		break;
	}
	return varbind;
}

void update_varbind(struct varbind* varbind, unsigned char data_type, void* value){
	free(varbind->value);
	varbind->data_type=data_type;
	switch (data_type) {
	case 0x02:
		{
			varbind->value=(unsigned int*)calloc(1, sizeof(unsigned int));
			*(unsigned int*)varbind->value=*(unsigned int*)value;
		}
		break;
	case 0x04:
		{
			varbind->value=(unsigned char*)calloc(1, strlen((char*)value)+1);
			strcpy((char*)varbind->value,(char*)value);
		}
		break;
	case 0x05:
		{
			varbind->value=(unsigned int*)calloc(1, sizeof(unsigned int));
			*(unsigned int*)varbind->value=0x00;
		}
		break;
	case 0x06:
		{
			varbind->value=(unsigned char*)calloc(1, strlen((char*)value)+1);
			strcpy((char*)varbind->value,(char*)value);
		}
		break;
	case 0x43:		
		{
			varbind->value=(unsigned int*)calloc(1, sizeof(unsigned int));
			*(unsigned int*)varbind->value=*(unsigned int*)value;
		}
	default:
		{
		}
		break;
	}
}

void disp_varbind(struct varbind* varbind){
	unsigned char* data_type=return_data_type_string(varbind->data_type);
	printf("***Varbind***\n");
	printf("OID: %s\n",varbind->oid);
	printf("Data Type: %i (%s)\n",varbind->data_type,data_type);
	switch (varbind->data_type) {
	case 0x02:
		{
			printf("Value: %i\n",*(unsigned int*)varbind->value);
		}
		break;
	case 0x04:
		{
			printf("Value: %s\n",(unsigned char*)varbind->value);
		}
		break;
	case 0x05:
		{
			printf("Value: %i\n",*(unsigned int*)varbind->value);
		}
		break;
	case 0x06:
		{
			printf("Value: %s\n",(unsigned char*)varbind->value);
		}
		break;
	case 0x43:
		{
			printf("Value: %i\n",*(unsigned int*)varbind->value);
		}
		break;
	default:
		{
		}
		break;
	}
	free(data_type);
}

void clr_varbind(struct varbind* varbind){
	free(varbind->value);
	free(varbind->oid);
	free(varbind);
}

struct varbind_list_tx* create_varbind_list_tx(struct varbind_list_rx* varbinds_to_send){
	struct varbind_list_tx* varbind_list=(struct varbind_list_tx*)calloc(1,sizeof(struct varbind_list_tx));
	unsigned int i;
	unsigned int pointer=4;
	unsigned int len_pointer=2;
	unsigned char* data_buffer=NULL;
	unsigned char* varbind_list_buffer=(unsigned char*) calloc(200,sizeof(unsigned char));
	for (i=0;i<varbinds_to_send->varbind_idx;i++){
		switch (varbinds_to_send->varbind_list[i]->data_type) {
		case 0x02:
			{
				data_buffer=encode_oid(varbinds_to_send->varbind_list[i]->oid);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+2;
				free(data_buffer);
				data_buffer=encode_integer(*(unsigned int*)varbinds_to_send->varbind_list[i]->value);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+4;
				free(data_buffer);
				varbind_list_buffer[len_pointer]=0x30;
				varbind_list_buffer[len_pointer+1]=pointer-len_pointer-4;
				len_pointer=pointer-2;
			}
			break;
		case 0x04:
			{
				data_buffer=encode_oid(varbinds_to_send->varbind_list[i]->oid);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+2;
				free(data_buffer);
				data_buffer=encode_string((unsigned char*)varbinds_to_send->varbind_list[i]->value);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+4;
				free(data_buffer);
				varbind_list_buffer[len_pointer]=0x30;
				varbind_list_buffer[len_pointer+1]=pointer-len_pointer-4;
				len_pointer=pointer-2;
			}
			break;
		case 0x05:
			{
				data_buffer=encode_oid(varbinds_to_send->varbind_list[i]->oid);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+2;
				free(data_buffer);
				varbind_list_buffer[pointer]=0x05;
				pointer++;
				varbind_list_buffer[pointer]=0x00;
				pointer+=3;
				varbind_list_buffer[len_pointer]=0x30;
				varbind_list_buffer[len_pointer+1]=pointer-len_pointer-4;
				len_pointer=pointer-2;
			}
			break;
		case 0x06:
			{
				data_buffer=encode_oid(varbinds_to_send->varbind_list[i]->oid);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+2;
				free(data_buffer);
				data_buffer=encode_oid((unsigned char*)varbinds_to_send->varbind_list[i]->value);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+4;
				free(data_buffer);
				varbind_list_buffer[len_pointer]=0x30;
				varbind_list_buffer[len_pointer+1]=pointer-len_pointer-4;
				len_pointer=pointer-2;
			}
			break;
		case 0x43:
			{
				data_buffer=encode_oid(varbinds_to_send->varbind_list[i]->oid);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+2;
				free(data_buffer);
				data_buffer=encode_integer_by_length(*(unsigned int*)varbinds_to_send->varbind_list[i]->value,4,0x43);
				memcpy(&varbind_list_buffer[pointer], data_buffer,data_buffer[1]+2);
				pointer+=data_buffer[1]+4;
				free(data_buffer);
				varbind_list_buffer[len_pointer]=0x30;
				varbind_list_buffer[len_pointer+1]=pointer-len_pointer-4;
				len_pointer=pointer-2;
			}
			break;
		default:
			{
			}
			break;
		}
	}
	varbind_list_buffer[0]=0x30;
	varbind_list_buffer[1]=pointer-4;
	varbind_list->varbind_list_len=pointer-2;
	varbind_list->varbind_list=(unsigned char*)calloc(1,varbind_list->varbind_list_len);
	memcpy(varbind_list->varbind_list,&varbind_list_buffer[0],varbind_list->varbind_list_len);
	free(varbind_list_buffer);
	return varbind_list;
}

void clr_varbind_list_tx(struct varbind_list_tx* varbind_list){
	free(varbind_list->varbind_list);
	free(varbind_list);
}

struct snmp_pdu_tx* create_snmp_pdu_tx(unsigned char pdu_type,unsigned int request_id,unsigned char error,unsigned char error_index, struct varbind_list_tx* varbind_list){
	struct snmp_pdu_tx* snmp_pdu=(struct snmp_pdu_tx*)calloc(1,sizeof(struct snmp_pdu_tx));
	unsigned int pointer=2;
	unsigned char* pdu_buffer=(unsigned char*)calloc(300,sizeof(unsigned char));
	unsigned char* data_buffer;
	data_buffer=encode_integer_by_length(request_id,4,0x02);
	memcpy(&pdu_buffer[pointer], data_buffer,data_buffer[1]+2);
	pointer+=data_buffer[1]+2;
	free(data_buffer);
	data_buffer=encode_integer((unsigned int)error);
	memcpy(&pdu_buffer[pointer], data_buffer,data_buffer[1]+2);
	pointer+=data_buffer[1]+2;
	free(data_buffer);
	data_buffer=encode_integer((unsigned int)error_index);
	memcpy(&pdu_buffer[pointer], data_buffer,data_buffer[1]+2);
	pointer+=data_buffer[1]+2;
	free(data_buffer);
	memcpy(&pdu_buffer[pointer], varbind_list->varbind_list,varbind_list->varbind_list_len);
	pointer+=varbind_list->varbind_list_len;
	snmp_pdu->snmp_pdu_len=pointer;
	pdu_buffer[0]=pdu_type;
	pdu_buffer[1]=pointer-2;	
	snmp_pdu->snmp_pdu=(unsigned char*)calloc(1,snmp_pdu->snmp_pdu_len);
	memcpy(snmp_pdu->snmp_pdu,&pdu_buffer[0],snmp_pdu->snmp_pdu_len);
	free(pdu_buffer);
	return snmp_pdu;
}

void clr_snmp_pdu_tx(struct snmp_pdu_tx* snmp_pdu){
	free(snmp_pdu->snmp_pdu);
	free(snmp_pdu);
}

struct snmp_message_tx* create_snmp_message_tx(unsigned char* community, struct snmp_pdu_tx* snmp_pdu){
	struct snmp_message_tx* snmp_msg=(struct snmp_message_tx*)calloc(1,sizeof(struct snmp_message_tx));
	unsigned int pointer=2;
	unsigned char* snmp_msg_buffer=(unsigned char*)calloc(500,sizeof(unsigned char));
	unsigned char* data_buffer;
	data_buffer=encode_integer((unsigned int)0x00);
	memcpy(&snmp_msg_buffer[pointer], data_buffer,data_buffer[1]+2);
	pointer+=data_buffer[1]+2;
	free(data_buffer);
	data_buffer=encode_string(community);
	memcpy(&snmp_msg_buffer[pointer], data_buffer,data_buffer[1]+2);
	pointer+=data_buffer[1]+2;
	free(data_buffer);
	memcpy(&snmp_msg_buffer[pointer], snmp_pdu->snmp_pdu,snmp_pdu->snmp_pdu_len);
	pointer+=snmp_pdu->snmp_pdu_len;
	snmp_msg->snmp_message_len=pointer;
	snmp_msg_buffer[0]=0x30;
	snmp_msg_buffer[1]=pointer-2;
	snmp_msg->snmp_message=(unsigned char*)calloc(1,snmp_msg->snmp_message_len);
	memcpy(snmp_msg->snmp_message,&snmp_msg_buffer[0],snmp_msg->snmp_message_len);
	free(snmp_msg_buffer);
	return snmp_msg;
}

void clr_snmp_message_tx(struct snmp_message_tx* snmp_msg){
	free(snmp_msg->snmp_message);
	free(snmp_msg);
}
