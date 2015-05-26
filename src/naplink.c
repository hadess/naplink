/*
 * NapLink Linux Client v1.0.1 by AndrewK of Napalm
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <usb.h>

#include "packet.h"
#include "pl2301.h"

/* #define DEBUG */

uid_t superuser;
uid_t loseruser;

int commandWaiting = 0;
char *commandBuffer = 0;
int bail = 0;
int benchmarking = 0;

/* get return value from target */
int get_return(usb_dev_handle *hnd, void *data)
{
    packet_header_t ph;
    return_data_t *pb = 0;
    int rv;

    recv_packet(hnd, &ph, (void *)&pb); 

    if (ph.type != PACKET_RETURN) {
	printf("Was expecting PACKET_RETURN, but got 0x%x\n", ph.type);
	return -1;
    }

    if ((ph.size > 4)&&(data))
	memcpy(data, pb->data, ph.size - 4);

    rv = pb->rv;
    
    if (pb)
	free(pb);

    return rv;
}

/* quit */
int do_quit(usb_dev_handle *hnd)
{
    packet_header_t ph;
    
    ph.type = PACKET_QUIT;
    ph.size = 0;

    send_packet(hnd, &ph, NULL);

    bail = 1;

    return get_return(hnd, NULL);

}

/* reset target */
int do_reset(usb_dev_handle *hnd)
{
    packet_header_t ph;
    
    ph.type = PACKET_RESET;
    ph.size = 0;

    send_packet(hnd, &ph, NULL);

    return get_return(hnd, NULL);

}

/* execute ee elf on target */
int do_execps2(usb_dev_handle *hnd, char *filename)
{
    packet_header_t ph;

    if (filename) {

	ph.type = PACKET_EXECPS2;
	ph.size = strlen(filename) + 1;

	send_packet(hnd, &ph, filename);

	return get_return(hnd, NULL);
    } else
	return -1;
}

/* execute iop irx on target */
int do_execiop(usb_dev_handle *hnd, char *filename)
{
    packet_header_t ph;

    if (filename) {

	ph.type = PACKET_EXECIOP;
	ph.size = strlen(filename) + 1;
	
	send_packet(hnd, &ph, filename);
	
	return get_return(hnd, NULL);
    } else
	return -1;
}

/* send return value packet to target */
int do_return(usb_dev_handle *hnd, int rv, void *data, int count)
{
    packet_header_t ph;
    return_data_t *pb;

    if (count < 0)
	count = 0;

    pb = malloc(4 + count);

    ph.type = PACKET_RETURN;
    ph.size = 4 + count;
    
    pb->rv = rv;
    if (count)
	memcpy(pb->data, data, count);

    send_packet(hnd, &ph, pb);
}

/* perform a transfer rate test */
int do_benchmark(usb_dev_handle *hnd)
{
    int i = 0;
    packet_header_t ph;
    struct timeval start, end;
    float bps;
    
    ph.type = PACKET_BENCHMARK;
    ph.size = 0;

    send_packet(hnd, &ph, NULL);

    printf("benchmarking..."); fflush(stdout);

    benchmarking = 1;

    gettimeofday(&start, NULL);

    while(i != PACKET_RETURN)
	i = recv_and_process_packet(hnd);

    gettimeofday(&end, NULL);

    benchmarking = 0;
    
    /* test is 32 reads of 64kbyte plus 32 writes of 64kbyte */
    bps = (65536 * 64) / ((end.tv_sec + end.tv_usec / 1000000.0) -(start.tv_sec + start.tv_usec / 1000000.0));
	
    printf(" %.0f bytes/second average\n", bps);
}

/* receive and process a packet from target */
int recv_and_process_packet(usb_dev_handle *hnd)
{
    char *buffer = 0;
    packet_header_t packet;
    int rv;
    char *buffer2;
    int flags = 0;

    /* receive packet */
    recv_packet(hnd, &packet, (void *)&buffer);

    /* process packet */
    switch(packet.type) {
    case PACKET_OPEN:
#ifdef DEBUG
	printf("PACKET_OPEN\n");
	printf("pathname = %s\n", ((open_data_t *)buffer)->pathname);
	printf("ps2 flags = %d\n", ((open_data_t *)buffer)->flags);
#endif
	if (!(strcmp( ((open_data_t *)buffer)->pathname, "/dev/tty"))) {
	    rv = 1;
	} else {
	    if (((open_data_t *)buffer)->flags & 1)
		flags |= O_RDONLY;
	    if (((open_data_t *)buffer)->flags & 2)
		flags |= O_WRONLY;
	    if (((open_data_t *)buffer)->flags & 0x100)
		flags |= O_APPEND;
	    if (((open_data_t *)buffer)->flags & 0x200)
		flags |= O_CREAT;
	    if (((open_data_t *)buffer)->flags & 0x400)
		flags |= O_TRUNC;
#ifdef DEBUG
	printf("pc flags = %d\n", flags);
#endif
	    rv = open( ((open_data_t *)buffer)->pathname, flags);
	}
	do_return(hnd, rv, NULL, 0);
	break;
    case PACKET_CLOSE:
#ifdef DEBUG
	printf("PACKET_CLOSE\n");
	printf("fd = %d\n", ((close_data_t *)buffer)->fd);
#endif
	if ( ((close_data_t *)buffer)->fd <= 2)
	    rv = 0;
	else
	    rv = close ( ((close_data_t *)buffer)->fd );
	do_return(hnd, rv, NULL, 0);
	break;
    case PACKET_READ:
#ifdef DEBUG
	printf("PACKET_READ\n");
	printf("fd = %d\n", ((read_write_data_t *)buffer)->fd);
	printf("count = %d\n", ((read_write_data_t *)buffer)->count);
#endif
	buffer2 = malloc(((read_write_data_t *)buffer)->count);
	if (!benchmarking)
	    rv = read( ((read_write_data_t *)buffer)->fd, buffer2, ((read_write_data_t *)buffer)->count );
	else
	    rv = ((read_write_data_t *)buffer)->count;
	do_return(hnd, rv, buffer2, rv );
	free(buffer2);
	break;
    case PACKET_WRITE:
#ifdef DEBUG
	printf("PACKET_WRITE\n");
	printf("fd = %d\n", ((read_write_data_t *)buffer)->fd);
	printf("count = %d\n", ((read_write_data_t *)buffer)->count);
#endif
	if (!benchmarking)
	    rv = write( ((read_write_data_t *)buffer)->fd, ((read_write_data_t *)buffer)->data, ((read_write_data_t *)buffer)->count );
	else
	    rv = ((read_write_data_t *)buffer)->count;
	do_return(hnd, rv, NULL, 0);
	break;
    case PACKET_LSEEK:
#ifdef DEBUG
	printf("PACKET_LSEEK\n");
	printf("fd = %d\n", ((lseek_data_t *)buffer)->fd);
	printf("offset = %d\n", ((lseek_data_t *)buffer)->offset);
	printf("whence = %d\n", ((lseek_data_t *)buffer)->whence);
#endif
	rv = lseek( ((lseek_data_t *)buffer)->fd, ((lseek_data_t *)buffer)->offset, ((lseek_data_t *)buffer)->whence );
	do_return(hnd, rv, NULL, 0);
	break;
    case PACKET_WAZZUP:
#ifdef DEBUG
	printf("PACKET_WAZZUP\n");
#endif
	switch (commandWaiting) {
	case 0:
	    rv = 0;
	    do_return(hnd, rv, NULL, 0);
	    break;
	case PACKET_RESET:
	    rv = do_reset(hnd);
	    printf("rv = %d\n", rv);
	    break;
	case PACKET_EXECPS2:
	    rv = do_execps2(hnd, commandBuffer);
	    printf("rv = %d\n", rv);
	    break;
	case PACKET_EXECIOP:
	    rv = do_execiop(hnd, commandBuffer);
	    printf("rv = %d\n", rv);
	    break;
	case PACKET_QUIT:
	    rv = do_quit(hnd);
	    printf("rv = %d\n", rv);
	    break;
	case PACKET_BENCHMARK:
	    do_benchmark(hnd);
	    break;
	default:
	    break;
	}
	commandWaiting = 0;
	break;
    case PACKET_RETURN:
	break;
    default:
	printf("BAD PACKET 0x%x\n",packet.type);
	break;
    }

    /* free buffer */
    if (buffer)
	free(buffer);

    return packet.type;
}

/* extract filename from a command string */
char *find_filename(char *buf)
{
    int i;

    for(i = 0; i <= strlen(buf); i++)
	if (buf[i] == ' ')
	    return &buf[i+1];
    return 0;
}

/* handle usb i/o and stdin */
void io_loop(usb_dev_handle * hnd)
{
    int rv;
    char buffer[300];

    /* make stdin non-blocking */
    fcntl(0, F_SETFL, O_NONBLOCK); 
    
    while(!bail) {
	/* check if peer is still there */
	if (CHECK_QLF(hnd, PEER_E) == 0) {
	    printf("peer went away\n");
	    
	    seteuid(superuser);
	    
	    usb_reset(hnd);
	    usb_set_configuration(hnd, 1);
	    usb_claim_interface(hnd, 0);
	    
	    seteuid(loseruser);

            /* handshake */
	    CLEAR_QLF(hnd, TX_REQ);
	    CLEAR_QLF(hnd, TX_C);
	    SET_QLF(hnd, PEER_E);
	    
	    printf("waiting for peer\n");
	    while(CHECK_QLF(hnd, PEER_E) == 0);
	    printf("peer is alive\n");
	}	

	/* check for input on usb */
 	if (CHECK_QLF(hnd, TX_REQ))
	    recv_and_process_packet(hnd);
	
	/* check for input on stdin */
	if (!commandWaiting) {
	    rv = read(0, buffer, 300);
	    if (rv > 0) {
		buffer[rv-1] = 0;
		if (!strncmp(buffer, "reset", 5)) {
		    printf("reset requested, ");
		    commandWaiting = PACKET_RESET;
		} else if (!strncmp(buffer, "benchmark", 9)) {
		    printf("benchmark requested.\n");
		    commandWaiting = PACKET_BENCHMARK;
		} else if (!strncmp(buffer, "execee", 6)) {
		    printf("execee requested, ");
		    commandWaiting = PACKET_EXECPS2;
		    commandBuffer = find_filename(buffer);
		} else if (!strncmp(buffer, "execiop", 7)) {
		    printf("execiop requested, ");
		    commandWaiting = PACKET_EXECIOP;
		    commandBuffer = find_filename(buffer);
		} else if (!strncmp(buffer, "quit", 4) || !strncmp(buffer, "exit", 4)) {
		    printf("quit requested, ");
		    commandWaiting = PACKET_QUIT;
		} else if (!strncmp(buffer, "help", 4)) {
		    if (!strncmp(buffer, "help help", 9)) {
			printf("Help On 'help':\n");
			printf("\tAre you stupid or something?!?\n");
		    } else if (!strncmp(buffer, "help benchmark", 14)) {
			printf("Help On 'benchmark':\n");
			printf("\tTests transfer speed.\n");
			printf("\tSyntax: benchmark\n");
		    } else if (!strncmp(buffer, "help reset", 10)) {
			printf("Help On 'reset':\n");
			printf("\tReset PS2 target.\n");
			printf("\tSyntax: reset\n");
		    } else if (!strncmp(buffer, "help execee", 11)) {
			printf("Help On 'execee':\n");
			printf("\tExecute EE ELF on PS2.\n");
			printf("\tSyntax: execee <file>\n");
			printf("\tExample: execee host:/home/me/myapp.elf\n");
		    } else if (!strncmp(buffer, "help execiop", 12)) {
			printf("Help On 'execiop':\n");
			printf("\tExecute IOP IRX on PS2.\n");
			printf("\tSyntax: execiop <file>\n");
			printf("\tExample: execiop host:/home/you/yourapp.irx\n");
		    } else if (!strncmp(buffer, "help exit", 9)) {
			printf("Help On 'exit':\n");
			printf("\tExit NapLink client.\n");
			printf("\tSyntax: exit\n");
		    } else {
			printf("Available Commands:\n");
			printf("\thelp - This help message.\n");
			printf("\treset - Reset the PS2 target.\n");
			printf("\texecee - Execute EE ELF file on PS2.\n");
			printf("\texeciop - Execute IOP IRX file on PS2.\n");
			printf("\tbenchmark - Test transfer speed.");
			printf("\texit - Exit the NapLink client.");
			printf("\ntype help <command> for more help\n");
		    }
		}
	    }
	}
    }
}

int main(void)
{
  struct usb_bus *bus;
  struct usb_device *dev;

  struct usb_bus *pl_bus = NULL;
  struct usb_device *pl_dev = NULL;

  usb_dev_handle * pl_hnd;

  superuser = geteuid();
  loseruser = getuid();

  seteuid(loseruser);

  printf("NapLink Linux Client v1.0.1\n");
  printf("Coded by AndrewK of Napalm\n\n");

  seteuid(superuser);

  usb_init();
  usb_find_busses();
  usb_find_devices();

  seteuid(loseruser);

  for (bus = usb_busses; bus; bus = bus->next) {
      for (dev = bus->devices; dev; dev = dev->next)
	  if (dev->descriptor.idVendor == PROLIFIC_VENDOR_ID)
	      switch (dev->descriptor.idProduct) {
	      case 0:
		  printf("Found a PL2301\n");
		  pl_bus = bus;
		  pl_dev = dev;
		  break;
	      case 1:
		  printf("Found a PL2302\n");
		  pl_bus = bus;
		  pl_dev = dev;
		  break;
	      default:
		  printf("Found an unknown Prolific device (id %d)\n", dev->descriptor.idProduct);
		  break;
	      }
  }
  
  if ((!pl_bus)||(!pl_dev)) {
      printf("No supported devices detected.\n");
      exit(1);
  }

  seteuid(superuser);

/* open device */
  pl_hnd = usb_open(pl_dev);

  usb_reset(pl_hnd);

  usb_set_configuration(pl_hnd, 1);
  usb_claim_interface(pl_hnd, 0);

  seteuid(loseruser);

/* handshake */
  CLEAR_QLF(pl_hnd, TX_REQ);
  CLEAR_QLF(pl_hnd, TX_C);
  SET_QLF(pl_hnd, PEER_E);

  printf("waiting for peer\n");
  while(CHECK_QLF(pl_hnd, PEER_E) == 0);
  printf("peer is alive\n");
  
/* peer is alive and we're ready to go */

  io_loop(pl_hnd);

/* tell peer we're going away */
  CLEAR_QLF(pl_hnd, PEER_E);

  seteuid(superuser);

/* close device */
  usb_release_interface(pl_hnd, 0);
  usb_close(pl_hnd);

  seteuid(loseruser);

  return 0;
}

