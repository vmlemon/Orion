/*********************************************************************
 *                
 * File path:     src/generic/tracebuffer.cc
 * Description:   tracebuffer interface
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
#include <stdarg.h>	/* for va_list, ... comes with gcc */
#include <debug.h>
#include <kmemory.h>
#include INC_API(kernelinterface.h)
#include INC_API(schedule.h)
#include INC_API(interrupt.h)
#include INC_GLUE(hwspace.h)

void handle_interrupt(word_t irq);

#if defined(CONFIG_TRACEBUFFER)

long tb_irq = -1;
bool tb_irq_masked = true;
bool tb_irq_pending = false;

DECLARE_KMEM_GROUP (kmem_trace);

trace_buffer_t * trace_buffer = NULL;
spinlock_t trace_lock;

SECTION(".init") void init_tracebuffer()
{
    word_t buffer_first;

    trace_buffer = (trace_buffer_t*) kmem.alloc( kmem_trace, TBUFF_SIZE );
    ASSERT(ALWAYS, trace_buffer);

    /* Setup tracebuffer header */
    trace_buffer->magic	    = (word_t)TBUF_MAGIC;
    trace_buffer->version   = TBUF_VERSION;
    trace_buffer->bufid	    = TBUF_ID;
    trace_buffer->buffers   = TBUF_BUFFERS;

    /* Calculate buffer size */
    buffer_first = sizeof(trace_buffer_t) + 7 & (~7ul);
    trace_buffer->buffer_size =
	((TBUFF_SIZE - buffer_first) / TBUF_BUFFERS) & (~7ul);

    /* Setup buffer offsets + head pointers */
    for (int i = 0; i < TBUF_BUFFERS; i ++ )
    {
	trace_buffer->buffer_head[i] = 0;
	trace_buffer->buffer_offset[i] = buffer_first + (i * trace_buffer->buffer_size);
    }

    /* Set active buffer = 0 and set rest to emtpy */
    trace_buffer->active_buffer	= 0;
    trace_buffer->buffer_empty = ((1ul << TBUF_BUFFERS) - 1) &
	(~(1ul << trace_buffer->active_buffer));

    /* By default, enable all traces */
    trace_buffer->log_mask = ~(0UL);

    get_kip()->memory_info.insert(TBUF_MEMDESC_TYPE, false,
	    virt_to_phys(trace_buffer),
	    virt_to_phys((addr_t)((word_t)trace_buffer + TBUFF_SIZE)));

    TRACE_INIT("Initialized tracebuffer @ %p\n", trace_buffer);
}

/*
 * flip to the next available tracebuffer + do virtual interrupt
 * note: tracebuffer lock is held here
 */
static word_t
tb_switch_buffers()
{
    word_t orig_buffer = trace_buffer->active_buffer;
    word_t next_buffer = trace_buffer->active_buffer;
    word_t empty_bitmask = trace_buffer->buffer_empty;
    word_t nbufs = trace_buffer->buffers;

    do {
	next_buffer++;
	if (next_buffer >= nbufs)
	    next_buffer = 0;

	if ((1ul << next_buffer) & empty_bitmask)
	{
	    /* Set next active buffer, mark it as not empty */
	    //printf("switch buffer -> %d\n", next_buffer);
	    trace_buffer->active_buffer = next_buffer;
	    trace_buffer->buffer_empty = empty_bitmask & (~(1ul << next_buffer));
	    trace_buffer->buffer_head[next_buffer] = 0;

	    goto switch_return;
	}
    } while (next_buffer != orig_buffer);

    ASSERT(DEBUG, !"no free buffers");
    trace_buffer->active_buffer = next_buffer = ~(0ul);

switch_return:
    /* Send tracebuffer interrupt */
    if (!tb_irq_masked)
    {
	tb_irq_masked = true;
	handle_interrupt(tb_irq);
    }
    else
	tb_irq_pending = true;
    return next_buffer;
}

/* Alocate space in the tracebuffer */
static word_t
tb_allocate(word_t entry_size, word_t *buffer)
{
    word_t buf;
    word_t entry;

    /* try allocate space in the tracebuffer */
    trace_lock.lock();
    {
	buf = trace_buffer->active_buffer;

	/* no active buffer? */
	if (buf == ~(0ul))
	    goto alloc_error;

	word_t new_head = trace_buffer->buffer_head[buf] + entry_size;

	/* test for enough space for this entry and buffer terminating entry */
	if (new_head >= trace_buffer->buffer_size)
	{
	    buf = tb_switch_buffers();
	    if (buf == ~(0ul))
		goto alloc_error;
	    new_head = trace_buffer->buffer_head[buf] + entry_size;
	}
	/* enough space, save current position and increment head */
	entry = trace_buffer->buffer_head[buf];
	trace_buffer->buffer_head[buf] = new_head;
    }
    trace_lock.unlock();

    *buffer = buf;
    return entry;

alloc_error:
    trace_lock.unlock();
    return ~(0ul);
}

/* Log an event */
void tb_log_event(word_t traceid)
{
    word_t entry_size = sizeof(trace_entry_t);
    word_t entry, buffer;

    /* try allocate space in the tracebuffer */
    entry = tb_allocate(entry_size, &buffer);
    //printf("event: (%d, %d, %d) %d\n", buffer, entry_size, entry, traceid);

    if (entry == ~(0ul))
	return;

    /* get the tracebuffer entry */
    trace_entry_t * tbe = (trace_entry_t*)((word_t)trace_buffer +
	    trace_buffer->buffer_offset[buffer] + entry);

    /* Write out tracebuffer event entry */
    tbe->timestamp = get_current_time();
    tbe->hdr.x.id = EVENT;
    tbe->hdr.x.user = 0;
    tbe->hdr.x.reclen = entry_size/sizeof(word_t);
    tbe->hdr.x.args = 1;
    tbe->hdr.x.major = EVENT_major;
    tbe->hdr.x.minor = 0;

    tbe->data[0] = traceid;
}

/* Log a full tracebuffer entry */
void tb_log_trace(word_t data, char *str, ...)
{
    va_list args;
    word_t major = data >> 24;
    word_t traceid = (data >> 8 & 0xffff);
    int nargs = (data & 0xf);

    /* calc entry_size, works for nargs = 0 due to data[1] in trace_entry_t */
    word_t entry_size = sizeof(trace_entry_t) + (sizeof(word_t)*(nargs-1));
    word_t entry, buffer;

    /* try allocate space in the tracebuffer */
    entry = tb_allocate(entry_size, &buffer);

    //printf("entry: (%d, %d, %d) (%d, %d, %d)\n", buffer, entry_size, entry, major, traceid, nargs);

    if (entry == ~(0ul))
	return;

    /* get the tracebuffer entry */
    trace_entry_t * tbe = (trace_entry_t*)((word_t)trace_buffer +
	    trace_buffer->buffer_offset[buffer] + entry);

    /* Write out tracebuffer event entry */
    tbe->timestamp = get_current_time();
    tbe->hdr.x.id = traceid;
    tbe->hdr.x.user = 0;
    tbe->hdr.x.reclen = entry_size/sizeof(word_t);
    tbe->hdr.x.args = nargs;
    tbe->hdr.x.major = major;
    tbe->hdr.x.minor = 0;

    int i = 0;
    va_start(args, str);
    while (i < nargs) {
	tbe->data[i++] = va_arg(args, word_t);;
    }
    va_end(args);
}

/* Virtual tracebuffer interrupt handling */
bool tb_reply_irq()
{
    if (tb_irq_pending)
    {
	tb_irq_pending = false;
	return true;
    }

    tb_irq_masked = false;
    return false;
}

void tb_irq_control(bool enable)
{
    tb_irq_masked = !enable;
}

#endif /* CONFIG_TRACEBUFFER */
