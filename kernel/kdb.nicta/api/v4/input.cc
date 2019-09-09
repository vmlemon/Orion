/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 *                
 * File path:     kdb/api/v4/input.cc
 * Description:   Version 4 specific input functions
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
 * $Id: input.cc,v 1.10 2004/12/09 01:29:37 cvansch Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include <kdb/input.h>
#include <kdb/kdb.h>
#include <kdb/console.h>
#include INC_API(space.h)
#include INC_API(tcb.h)


static int
strncmp(const char *s1, const char *s2, int n)
{
	int          i;
	int             diff;

	for (i = 0; i < n; i++) {
		diff =
		    ((unsigned char *) s1)[i] - ((unsigned char *) s2)[i];
		if (diff != 0 || s1[i] == '\0')
			return diff;
	}
	return 0;
}

static inline bool str2int(const char *str, int size, word_t *value) {
	bool is_num = true;
	word_t val = 0;
	for (int i=0; i < size; i++) {
		switch(str[i]) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			val *= 16;
			val += str[i] - '0';
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			val *= 16;
			val += 10 + str[i] - 'a';
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			val *= 16;
			val += 10 + str[i] - 'A';
		default:
			is_num = false;
			break;
		}
	}
	*value = val;
	return is_num;
}

static inline int thread_match(const char * str, word_t size, tcb_t **tcb)
{
    int found = 0;
    space_t * dummy = kdb.kdb_current->get_space ();
    dummy = dummy ? dummy : get_kernel_space();
    word_t val;
    bool is_num = false;

    *tcb = NULL;

    /* Maybe it is an id (either TID or TCB) */
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
	is_num = str2int(&str[2], size-2, &val);
    } else {
	is_num = str2int(str, size, &val);
    }

    if (is_num) {
	if (dummy->is_tcb_area ((addr_t) val)) {
	    *tcb = addr_to_tcb ((addr_t) val);
	    found = 1;
	} else {
	    *tcb = dummy->get_tcb (threadid (val));

	    if ((*tcb)->myself_global == threadid ((word_t) val)) {
		found = 1;
	    } else {
		if (dummy->is_user_area ((addr_t)val))
		{
		    // Pointer in lower memory area.  Probably a physical address.
		    val = (word_t)phys_to_virt ((addr_t)val);
		}
		*tcb = (tcb_t *)val;
	    }
	}
    } else {
	present_list_lock.lock();
	tcb_t* walk = global_present_list;

	do {
	    /* Only compare first size characters */
	    if (strncmp(str, walk->debug_name, size) == 0) {
		found = 1;
		*tcb = walk;
		break;
	    }
	    walk = walk->present_list.next;
	} while (walk != global_present_list);
	present_list_lock.unlock();
    }
    return found;
}


/**
 * Prompt for an address space using PROMPT and return the space
 * pointer.  Input value can be a TCB address or a physical or virtual
 * space pointer.

 * @param prompt	prompt
 *
 * @return pointer to space
 */
space_t SECTION(SEC_KDEBUG) * get_space (const char * prompt)
{
    char buf[20];
    bool valid_id = 0;
    tcb_t *tcb;
    space_t * space = NULL;

    int cnt = 0;
    char c;
    /* Zero out string */
    for (int i=0; i < 20; i++) {
	buf[i] = '\0';
    }
    printf ("%s [current]: ", prompt);

    /* Get a string */
    while (1) {
	c = getc();
	if (c == KEY_BS) {
	    if (cnt > 0) {
		putc(c);
		putc(' ');
		putc(c);
		buf[cnt--] = '\0';
	    }
	} else if (c == KEY_RETURN) {
	    break;
	} else {
	    buf[cnt++] = c;
	    putc(c);

	    if (cnt == 20)
		break;
	}
    }

    if (cnt == 0) {
	/* Just hit enter -- current */
	printf ("current");
	tcb = kdb.kdb_current;
	valid_id = 1;
    } else {
	/* Check valid thread name */
	if (thread_match(buf, cnt, &tcb)) {
	    valid_id = 1;
	}
    }

    if (valid_id == 1) {
	space = tcb->get_space ();
    } else {
	/* Hope it is a space */
	if (tcb)
	    space = (space_t *)tcb;
    }

    printf ("\n");
    return space ? space : get_kernel_space();
}


/**
 * Prompt for a thread using PROMPT and return the tcb pointer.  Input
 * value can be a TCB address od a thread id.
 *
 * @param prompt	prompt
 *
 * @return pointer to tcb
 */
tcb_t SECTION (SEC_KDEBUG) * get_thread (const char * prompt)
{
    char buf[20];
    bool valid_id = 0;
    tcb_t *tcb;

    int cnt = 0;
    char c;
    /* Zero out string */
    for (int i=0; i < 20; i++) {
	buf[i] = '\0';
    }
    printf ("%s [current]: ", prompt);

    /* Get a string */
    while (1) {
	c = getc();
	if (c == KEY_BS) {
	    if (cnt > 0) {
		putc(c);
		putc(' ');
		putc(c);
		buf[cnt--] = '\0';
	    }
	} else if (c == KEY_RETURN) {
	    break;
	} else {
	    buf[cnt++] = c;
	    putc(c);

	    if (cnt == 20)
		break;
	}
    }

    if (cnt == 0) {
	/* Just hit enter -- current */
	printf ("current");
	tcb = kdb.kdb_current;
	valid_id = 1;
    } else {
	/* Check valid thread name */
	if (thread_match(buf, cnt, &tcb)) {
	    valid_id = 1;
	}
    }

    if (valid_id == 0) {
	printf("\nNo thread named: %s\n", buf);
	return NULL;
    }

    printf ("\n");
    return tcb;
}
