/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia
 *                
 * File path:     kdb/tracepoints.h
 * Description:   Tracepoint interface
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
 * $Id: tracepoints.h,v 1.12 2003/09/24 19:04:54 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __KDB__TRACEPOINTS_H__
#define __KDB__TRACEPOINTS_H__

#include <debug.h>
#include <kdb/linker_set.h>
#include <generic/traceids.h>

// avoid including api/smp.h for non-SMP case
#ifndef CONFIG_SMP
# define TP_CPU 0
#else
# include INC_API(smp.h)
# if defined(CONFIG_ARCH_IA64)
#   define TP_CPU 0
# else
#   define TP_CPU get_current_cpu()
# endif
#endif

extern bool tracepoint_count;

class tracepoint_t
{
public:
    word_t	id;
    word_t	enabled;
    word_t	enter_kdb;
    word_t	counter[CONFIG_SMP_MAX_CPUS];

public:
    void reset_counter ()
	{ for (int cpu = 0; cpu < CONFIG_SMP_MAX_CPUS; counter[cpu++] = 0); }
};

#define EXTERN_TRACEPOINT(tp)				\
    extern tracepoint_t __tracepoint_##tp

#if defined(CONFIG_TRACEPOINTS)

/*
 * Wrapper class for accessing tracepoint set.
 */

class tracepoint_list_t
{
public:
    linker_set_t	*tp_set;

    inline void reset (void)
	{ tp_set->reset (); }

    inline tracepoint_t * next (void)
	{ return (tracepoint_t *) tp_set->next (); }

    inline word_t size (void)
	{ return tp_set->size (); }

    inline tracepoint_t * get (word_t n)
	{ return (tracepoint_t *) tp_set->get (n); }
};

extern tracepoint_list_t tp_list;

#define DECLARE_TRACEPOINT(tp)					\
    tracepoint_t __tracepoint_##tp = { tp, 0, 0, { 0, } };	\
    PUT_SET (tracepoint_set, __tracepoint_##tp)

#define TRACEPOINT(tp, code...)					\
do {								\
    TBUF_RECORD_EVENT (tp);					\
    if (tracepoint_count) __tracepoint_##tp.counter[TP_CPU]++;	\
    if (__tracepoint_##tp.enabled & (1UL << TP_CPU))		\
    {								\
	{code;}							\
	if (__tracepoint_##tp.enter_kdb & (1UL << TP_CPU))	\
	    enter_kdebug (#tp);					\
    }								\
} while (0)

#define TRACEPOINT_TB(tp, code, tbs, tba...)			\
do {								\
    TBUF_RECORD (tp, tbs, tba);					\
    if (tracepoint_count) __tracepoint_##tp.counter[TP_CPU]++;	\
    if (__tracepoint_##tp.enabled & (1UL << TP_CPU))		\
    {								\
	{code;}							\
	if (__tracepoint_##tp.enter_kdb & (1UL << TP_CPU))	\
	    enter_kdebug (#tp);					\
    }								\
} while (0)

#define ENABLE_TRACEPOINT(tp, kdb)		\
do {						\
    __tracepoint_##tp.enabled = ~0UL;		\
    __tracepoint_##tp.enter_kdb = kdb;		\
} while (0)

#define TRACEPOINT_ENTERS_KDB(tp)		\
   (__tracepoint_##tp.enter_kdb)


#else /* !CONFIG_TRACEPOINTS */

#define DECLARE_TRACEPOINT(tp)				\
    tracepoint_t __tracepoint_##tp = { tp, 0, 0, { 0, } };

#define TRACEPOINT(tp, code...)			\
do {						\
    TBUF_RECORD_EVENT (tp);			\
} while (0)

#define TRACEPOINT_TB(tp, code, tbs, tba...)	\
do {						\
    TBUF_RECORD (tp, tbs, tba);			\
} while (0)

#define ENABLE_TRACEPOINT(tp, kdb)
#define TRACEPOINT_ENTERS_KDB(tp) (0)

#endif


#endif /* !__KDB__TRACEPOINTS_H__ */
