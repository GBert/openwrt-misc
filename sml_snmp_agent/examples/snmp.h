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
//  Module:			snmp.h
//  Author:		Andreas Bartusch <xelean@googlemail.com>
//  Contact@FOKUS:	bjoern.riemer@fokus.fraunhofer.de
//  Date:			22.02.2011
////////////////////////////////////////////////////////////////////////

#ifndef SNMP_H_INCLUDED
#define SNMP_H_INCLUDED

struct snmp_message_rx{
	unsigned int snmp_message_length;
	unsigned char version;
	unsigned char* community;
	unsigned char* snmp_pdu;
	unsigned int snmp_pdu_length;
};

struct snmp_pdu_rx{
	unsigned char snmp_pdu_type;
	unsigned int snmp_pdu_length;
	unsigned int request_id;
	unsigned char error;
	unsigned char error_index;
	unsigned char* varbindings;
	unsigned int varbindings_length;
};

struct varbind{
	unsigned char* oid;
	unsigned char data_type;
	void* value;
};

struct varbind_list_rx{
	unsigned int varbind_list_length;
	unsigned char varbind_idx;
	struct varbind** varbind_list;
};

struct varbind_list_tx{
	unsigned int varbind_list_len;
	unsigned char* varbind_list;
};

struct snmp_pdu_tx{
	unsigned int snmp_pdu_len;
	unsigned char* snmp_pdu;
};

struct snmp_message_tx{
	unsigned int snmp_message_len;
	unsigned char* snmp_message;
};

int decode_integer(unsigned char*, unsigned int*);

unsigned char* decode_string(unsigned char*, unsigned int*);

unsigned char* decode_oid(unsigned char*, unsigned int*);

unsigned char* encode_oid(unsigned char*);

unsigned char* encode_string(unsigned char*);

unsigned char* encode_integer(unsigned int);

unsigned char* encode_integer_by_length(unsigned int, unsigned char, unsigned char);

unsigned char* return_pdu_type_string(unsigned char);

unsigned char* return_data_type_string(unsigned char);

void disp_snmp_message_rx(struct snmp_message_rx*);

void disp_snmp_pdu_rx(struct snmp_pdu_rx*);

void disp_varbind_list_rx(struct varbind_list_rx*);

void disp_varbind(struct varbind*);

struct snmp_message_rx* create_snmp_message_rx(unsigned char*);

struct snmp_pdu_rx* create_snmp_pdu_rx(unsigned char*);

struct varbind_list_rx* create_varbind_list_rx(unsigned char*);

struct varbind* create_varbind(unsigned char*, unsigned char, void*);

struct varbind_list_tx* create_varbind_list_tx(struct varbind_list_rx*);

struct snmp_pdu_tx* create_snmp_pdu_tx(unsigned char,unsigned int,unsigned char,unsigned char, struct varbind_list_tx*);

struct snmp_message_tx* create_snmp_message_tx(unsigned char*, struct snmp_pdu_tx*);

void update_varbind(struct varbind*, unsigned char, void*);

void clr_snmp_message_rx(struct snmp_message_rx*);

void clr_snmp_pdu_rx(struct snmp_pdu_rx*);

void clr_varbind_list_rx(struct varbind_list_rx*);

void clr_varbind(struct varbind*);

void clr_varbind_list_tx(struct varbind_list_tx*);

void clr_snmp_pdu_tx(struct snmp_pdu_tx*);

void clr_snmp_message_tx(struct snmp_message_tx*);

#endif