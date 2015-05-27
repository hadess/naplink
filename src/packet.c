/*
 * NapLink Linux Client v1.0.0 by AndrewK of Napalm
 *
 * Copyright (c) Andrew Kieschnick.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author(s) may not be used to endorse or promote products 
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#include "packet.h"
#include "pl2301.h"

extern uid_t superuser;
extern uid_t loseruser;

/* #define DEBUG */

/* receive a packet */
void recv_packet(libusb_device_handle *hnd, packet_header_t *ph, void **pb)
{
    int transferred;
#ifdef DEBUG
    printf("Waiting for PS2 to set TX_REQ...\n");
#endif

    /* wait for TX_REQ */
    while(CHECK_QLF(hnd, TX_REQ) == 0);

    /* clear TX_REQ */
    CLEAR_QLF(hnd, TX_REQ);

#ifdef DEBUG
    printf("Receiving packet from PS2...\n");            
#endif

    seteuid(superuser);
    /* receive packet header */
    libusb_bulk_transfer(hnd, 0x83, (unsigned char *)ph, 8, &transferred, 10000);

    /* receive packet body */
    if (ph->size) {
	*pb = malloc(ph->size);
	libusb_bulk_transfer(hnd, 0x83, *pb, ph->size, &transferred, 10000);
    }	
    seteuid(loseruser);

#ifdef DEBUG
    printf("Waiting for TX_C...\n");
#endif

    /* wait for TX_C */
    while(CHECK_QLF(hnd, TX_C) == 0);

    /* clear TX_C */
    CLEAR_QLF(hnd, TX_C);

#ifdef DEBUG
    printf("Packet received from PS2!\n");
#endif
}

/* send a packet */
void send_packet(libusb_device_handle *hnd, packet_header_t *ph, void *pb)
{
    int transferred;

#ifdef DEBUG
    printf("Waiting for TX_RDY...\n");
#endif

    /* wait for TX_RDY */
    while(CHECK_QLF(hnd, TX_RDY) == 0);

    /* set TX_REQ */
    SET_QLF(hnd, TX_REQ);

#ifdef DEBUG
    printf("Waiting for TX_REQ to clear...\n");
#endif

    /* wait for peer to clear TX_REQ */
    while(CHECK_QLF(hnd, TX_REQ));

#ifdef DEBUG
    printf("Sending packet to PS2...\n");                
#endif

    seteuid(superuser);
    /* send bulk */
    libusb_bulk_transfer(hnd, 0x02, (unsigned char *)ph, 8, &transferred, 10000);

    if (ph->size)
	libusb_bulk_transfer(hnd, 0x02, pb, ph->size, &transferred, 10000);
    seteuid(loseruser);

    /* set TX_C */
    SET_QLF(hnd, TX_C);
/*
#ifdef DEBUG
    printf("Waiting for TX_C to clear...\n");
#endif
*/
    /* wait for peer to clear TX_C */
//    while(CHECK_QLF(hnd, TX_C));

#ifdef DEBUG
    printf("Packet sent to PS2!\n");
#endif
}

