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

#ifndef __PL2301_H__
#define __PL2301_H__

#include <sys/types.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define PROLIFIC_VENDOR_ID          0x067b
#define PL2301_DEVICE_ID            0x0000
#define PL2302_DEVICE_ID            0x0001
#define BELKIN_VENDOR_ID            0x050d
#define BELKIN_VISTA_DEVICE_ID      0x258a

#define PEER_E    0x01 /* bit0 - Peer Exists */
#define TX_REQ    0x02 /* bit1 - Transfer Request */
#define TX_C      0x04 /* bit2 - Transfer Complete */
#define RESET_I   0x08 /* bit3 - Reset Input Pipe */
#define RESET_O   0x10 /* bit4 - Reset Ouput Pipe */
#define TX_RDY    0x20 /* bit5 - Transmit Request Acknowledge */
#define RESERVED  0x40 /* bit6 - Transmit Complete Acknowledge */
#define S_EN      0x80 /* bit7 - Suspend Enable */

/* clear a feature in quicklink features */
#define CLEAR_QLF(hnd, feature) while(libusb_control_transfer(hnd, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, LIBUSB_REQUEST_CLEAR_FEATURE, feature, 0, 0, 0, 10000) < 0);

/* set a feature in quicklink features */
#define SET_QLF(hnd, feature) while(libusb_control_transfer(hnd, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, LIBUSB_REQUEST_SET_FEATURE, feature, 0, 0, 0, 10000) < 0);

/* check status of a feature in quicklink features */
unsigned char CHECK_QLF(libusb_device_handle *hnd, int feature);

#endif
