/*********************************************************************
 *                
 * File path:     kdb/generic/tracebuffer.cc
 * Description:   tracebuffer KDB interface
 *
 * Australian Public Licence B (OZPLB)
 * 
 * Version 1-0
 * 
 * Copyright (c) 2005 National ICT Australia
 * 
 * All rights reserved. 
 * 
 * Developed by: Embedded, Real-time and Operating Systems Program (ERTOS)
 *               National ICT Australia
 *               http://www.ertos.nicta.com.au
 * 
 * Permission is granted by National ICT Australia, free of charge, to
 * any person obtaining a copy of this software and any associated
 * documentation files (the "Software") to deal with the Software without
 * restriction, including (without limitation) the rights to use, copy,
 * modify, adapt, merge, publish, distribute, communicate to the public,
 * sublicense, and/or sell, lend or rent out copies of the Software, and
 * to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimers.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimers in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of National ICT Australia, nor the names of its
 *       contributors, may be used to endorse or promote products derived
 *       from this Software without specific prior written permission.
 * 
 * EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT
 * PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND
 * NATIONAL ICT AUSTRALIA AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS,
 * WARRANTIES OR CONDITIONS OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO ANY REPRESENTATIONS, WARRANTIES OR CONDITIONS
 * REGARDING THE CONTENTS OR ACCURACY OF THE SOFTWARE, OR OF TITLE,
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,
 * THE ABSENCE OF LATENT OR OTHER DEFECTS, OR THE PRESENCE OR ABSENCE OF
 * ERRORS, WHETHER OR NOT DISCOVERABLE.
 * 
 * TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
 * NATIONAL ICT AUSTRALIA OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL
 * THEORY (INCLUDING, WITHOUT LIMITATION, IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHERWISE) FOR ANY CLAIM, LOSS, DAMAGES OR OTHER
 * LIABILITY, INCLUDING (WITHOUT LIMITATION) LOSS OF PRODUCTION OR
 * OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA OR RECORDS; OR LOSS
 * OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT OR GOODWILL, OR
 * OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL, INDIRECT,
 * CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF OR IN
 * CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS WITH THE SOFTWARE, EVEN IF NATIONAL ICT AUSTRALIA OR ITS
 * CONTRIBUTORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS,
 * DAMAGES OR OTHER LIABILITY.
 * 
 * If applicable legislation implies representations, warranties, or
 * conditions, or imposes obligations or liability on National ICT
 * Australia or one of its contributors in respect of the Software that
 * cannot be wholly or partly excluded, restricted or modified, the
 * liability of National ICT Australia or the contributor is limited, to
 * the full extent permitted by the applicable legislation, at its
 * option, to:
 * a.  in the case of goods, any one or more of the following:
 * i.  the replacement of the goods or the supply of equivalent goods;
 * ii.  the repair of the goods;
 * iii. the payment of the cost of replacing the goods or of acquiring
 *  equivalent goods;
 * iv.  the payment of the cost of having the goods repaired; or
 * b.  in the case of services:
 * i.  the supplying of the services again; or
 * ii.  the payment of the cost of having the services supplied again.
 * 
 * The construction, validity and performance of this licence is governed
 * by the laws in force in New South Wales, Australia.
 ********************************************************************/

#include <debug.h>
#include <kdb/kdb.h>
#include <kdb/input.h>
#include <kdb/tracepoints.h>

#ifdef CONFIG_TRACEBUFFER

DECLARE_CMD_GROUP (tracebuf);

DECLARE_CMD (cmd_tracebuffer, root, 'b', "tracebuffer", "tracebuffer menu");

CMD(cmd_tracebuffer, cg)
{
    return tracebuf.interact (cg, "tracebuffer");
}

DECLARE_CMD (cmd_tb_info, tracebuf, 'i', "info", "show tracebuffer info");

CMD(cmd_tb_info, cg)
{
    if (!trace_buffer)
    {
	printf("Invalid trace_buffer (NULL)\n");
	return CMD_NOQUIT;
    }
	
    printf("=== Tracebuffer @ %p, magic = %p ===\n", trace_buffer, (void*)trace_buffer->magic);
    printf("version: %8x, bufid:         %8x\n", trace_buffer->version, trace_buffer->bufid);
    printf("buffers: %8d, active_buffer: %8d\n", trace_buffer->buffers, trace_buffer->active_buffer);
    printf("bufsize: %8d, empty mask     %8x\n", trace_buffer->buffer_size, trace_buffer->buffer_empty);
    printf("logmask: %8x\n", trace_buffer->log_mask);
    for (int i = 0; i < (int)trace_buffer->buffers; i++)
    {
	printf("  buffer %d: offset: %8d,  head: %8d\n", i,
		trace_buffer->buffer_offset[i], trace_buffer->buffer_head[i]);
    }
    return CMD_NOQUIT;
}

DECLARE_CMD (cmd_tb_logmask, tracebuf, 'l', "logmask", "change logmask");

CMD(cmd_tb_logmask, cg)
{
    word_t logmask = get_hex ("Log Mask", trace_buffer->log_mask);

    if (logmask == ABORT_MAGIC)
	return CMD_NOQUIT;

    trace_buffer->log_mask = logmask;

    return CMD_NOQUIT;
}

DECLARE_CMD (cmd_tb_dump, tracebuf, 'd', "dump", "dump current buffer");

CMD(cmd_tb_dump, cg)
{
    word_t buffer = trace_buffer->active_buffer;
    word_t head = trace_buffer->buffer_head[buffer];
    word_t entry = 0;

    if (buffer == ~(0ul))
    {
	printf("No current active buffer!\n");
	return CMD_NOQUIT;
    }

    while (entry < head)
    {
	trace_entry_t * tbe = (trace_entry_t*)((word_t)trace_buffer +
		trace_buffer->buffer_offset[buffer] + entry);

	printf("%6d: %9m ", entry, tbe->timestamp);

	switch (tbe->hdr.x.id) {
	case EVENT:
	    printf("(%d, %d, %d) %s", tbe->hdr.x.major, tbe->hdr.x.minor,
		    tbe->hdr.x.user, trace_names[tbe->data[0]]);
	    break;
	case STRING:
	    printf("(%d, %d, %d) \"%s\"", tbe->hdr.x.major, tbe->hdr.x.minor,
		    tbe->hdr.x.user, (char*)tbe->data);
	    break;
	default:
	    printf("(%d, %d, %d) %s: ",
		    tbe->hdr.x.major, tbe->hdr.x.minor, tbe->hdr.x.user,
		    trace_names[tbe->hdr.x.id]);

	    for ( int i = 0; i < (int)tbe->hdr.x.args; i++ )
		printf("%p, ", (void*)tbe->data[i]);
	    break;
	}

	printf("\n");

	entry = entry + (tbe->hdr.x.reclen * sizeof(word_t));
    }
    
    return CMD_NOQUIT;
}

#endif
