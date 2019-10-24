/****************************************************************************
 *
 * Copyright (C) 2004,  National ICT Australia (NICTA)
 *
 * File path:	platform/csb337/console.cc
 * Description:	Cogent CSB337 Console
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: console.cc,v 1.1 2004/08/12 12:00:34 cvansch Exp $
 *
 ***************************************************************************/
#include <l4.h>
#include <debug.h>

#include INC_GLUE(config.h)
#include INC_PLAT(console.h)
#include <kdb/console.h>

#define SERIAL_NAME	"serial"

#define rULCON0     (*(volatile unsigned *)CONSOLE_VADDR) //UART 0 Line control
#define rUCON0      (*(volatile unsigned *)(CONSOLE_VADDR + 0x4)) //UART 0 Control
#define rUFCON0     (*(volatile unsigned *)(CONSOLE_VADDR + 0x8)) //UART 0 FIFO control
#define rUMCON0     (*(volatile unsigned *)(CONSOLE_VADDR + 0xc)) //UART 0 Modem control
#define rUTRSTAT0   (*(volatile unsigned *)(CONSOLE_VADDR + 0x10)) //UART 0 Tx/Rx status
#define rUERSTAT0   (*(volatile unsigned *)(CONSOLE_VADDR + 0x14)) //UART 0 Rx error status
#define rUFSTAT0    (*(volatile unsigned *)(CONSOLE_VADDR + 0x18)) //UART 0 FIFO status
#define rUMSTAT0    (*(volatile unsigned *)(CONSOLE_VADDR + 0x1c)) //UART 0 Modem status
#define rUTXH0      (*(volatile unsigned *)(CONSOLE_VADDR + 0x20)) //UART 0 Transmission Hold
#define rURXH0 (*(volatile unsigned char *)(CONSOLE_VADDR + 0x24)) //UART 0 Receive buffer
#define rUBRDIV0    (*(volatile unsigned *)(CONSOLE_VADDR + 0x28)) //UART 0 Baud rate divisor

static void putc_serial( char c )
{
    if (c == '\n')
	putc_serial('\r');
    rUTXH0=c;	
    while(!(rUTRSTAT0 & 0x4)); 			// Wait until THR is empty.
}

static char getc_serial( bool block )
{
    if (block) {
	while(!(rUTRSTAT0 & 0x1)); 			// Receive data read
	return rURXH0;
    } else {
	if(!(rUTRSTAT0 & 0x1)) return 0;			// Receive data read
	else return rURXH0;
    }
}


#if defined(CONFIG_KDB_BREAKIN)
void kdebug_check_breakin (void)
{
}
#endif

static void init(void) 
{
    u32_t freq = 266000000L; /* Hack */
    rUFCON0=0x00;     							// FIFO Disable
    rUMCON0=0x00;    
    rULCON0=0x03;     							// Normal,No parity,1 stop,8 bit
    rUCON0=0x245;    	
    rUBRDIV0=(int)(freq/4/(115200*16)-1);			// freq/4MHz, 115200bps
}

/****************************************************************************
 *
 *    Console registration
 *
 ****************************************************************************/

kdb_console_t kdb_consoles[] = {
    { SERIAL_NAME, init, putc_serial, getc_serial},
    KDB_NULL_CONSOLE
};

word_t kdb_current_console = 0;

