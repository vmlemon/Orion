/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     glue/v4-amd64/init.cc
 * Description:   
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
 * $Id: 
 *                
 ********************************************************************/
#include <init.h>
#include <kmemory.h>
#include <mapping.h>
#include <ctors.h>

#include INC_API(kernelinterface.h)
#include INC_API(types.h)
#include INC_API(processor.h)
#include INC_API(schedule.h)
#include INC_ARCH(cpuid.h)
#include INC_ARCH(descreg.h)
#include INC_ARCH(hwcr.h)
#include INC_ARCH(segdesc.h)
#include INC_ARCH(tss.h)
#include INC_GLUE(intctrl.h)
#include INC_PLAT(rtc.h)
#include INC_GLUE(timer.h)
#include INC_GLUE(idt.h)
#include INC_GLUE(memory.h)

amd64_cpu_features_t boot_cpu_ft UNIT("amd64.cpulocal") CTORPRIO(CTORPRIO_GLOBAL, 1);
amd64_tss_t tss UNIT("amd64.cpulocal") CTORPRIO(CTORPRIO_GLOBAL, 2);
bool tracebuffer_initialized UNIT("amd64.cpulocal");

struct{
    amd64_segdesc_t segdsc[GDT_SIZE - 2];	/* 6 entries a  8 byte */
    amd64_tssdesc_t tssdsc;			/* 1 entries a 16 byte */
} gdt UNIT("amd64.cpulocal");

u8_t amd64_cache_line_size;


/**
 * Clear BSS
 * 
 */
static void  SECTION(SEC_INIT) clear_bss()
{
    extern u8_t _start_bss[];
    extern u8_t _end_bss[];
    for (u8_t* p = _start_bss; p < _end_bss; p++)
	*p = 0;
}


/**
 * Check CPU features 
 * 
 */

static void SECTION(SEC_INIT) check_cpu_features(){

#if 0
    boot_cpu_ft.dump_features();
#endif
    amd64_cache_line_size = boot_cpu_ft.get_l1_cache().d.dcache.l_size;
    
}

/**
 * Initialize boot memory 
 * 
 */

static void SECTION(SEC_INIT) init_bootmem(){
    
    extern u8_t _start_bootmem[];
    extern u8_t _end_bootmem[];
    
    
    for (u8_t * p = _start_bootmem; p < _end_bootmem; p++){
	*p = 0;
    }

    kmem.init(start_bootmem, end_bootmem); 
}

static void add_more_kmem()
{

    /* 
     * Scan memory descriptors for a block of reserved physical memory
     * that is within the range of (to be) contiguously mapped
     * physical memory.  If there's one, it has been set up by the
     * boot loader for us. 
     */
    bool found = false;

    for (word_t i = 0;
         i < get_kip()->memory_info.get_num_descriptors();
         i++)
    {
        memdesc_t* md = get_kip()->memory_info.get_memdesc(i);

        if (!md->is_virtual() &&
            (md->type() == memdesc_t::reserved) &&
            (word_t) md->high() <= KERNEL_AREA_END)
        {
	    TRACE_INIT("Remapping  suitable Region %x size %x:\n", md->low(), md->size());
            // Map region kernel writable 
            get_kernel_space()->remap_area(
                phys_to_virt(md->low()), md->low(),
                (KERNEL_PAGE_SIZE == AMD64_2MPAGE_SIZE)
                 ? pgent_t::size_2m
                 : pgent_t::size_4k,
                (md->size() + (KERNEL_PAGE_SIZE-1)) & ~(KERNEL_PAGE_SIZE-1),
                true, true, true);

            // Add it to allocator
	    TRACE_INIT("Adding  Region %x size %x:\n", phys_to_virt(md->low()), md->size());
            kmem.add(phys_to_virt(md->low()),
                     md->size());
	    found = true;
        }
    }
    if (!found)
	TRACE_INIT("Did not find any suitable Region\n");
    
}

static void SECTION(SEC_INIT) init_meminfo(){

    extern word_t _memory_descriptors_size[];

    if (get_kip()->memory_info.get_num_descriptors() == (word_t) &_memory_descriptors_size){
	TRACE_INIT("\tBootloader did not patch memory info...\n");
	get_kip()->memory_info.n = 0;
    }
    /* 
     * reserve ourselves
     */
    
    get_kip()->memory_info.insert(memdesc_t::reserved, false,
				  start_text_phys, end_text_phys);

    get_kip()->memory_info.insert(memdesc_t::reserved, false,
				  start_bootmem_phys, end_bootmem_phys);

    get_kip()->memory_info.insert(memdesc_t::reserved, false,
				  start_syscalls_phys, end_syscalls_phys);

    /* 
     * add user area
     */

    get_kip()->memory_info.insert(memdesc_t::conventional, true,
				  (void *) USER_AREA_START, (void *) USER_AREA_END);
    

    /* 
     * dump reserved memory regions
     */
    
    //TRACE_INIT("Memory descriptors (p=%p/n=%d):\n",
    //       get_kip()->memory_info.get_memdesc(0),
    //     get_kip()->memory_info.get_num_descriptors());
    //for (word_t i=0; i < get_kip()->memory_info.get_num_descriptors(); i++){
	//memdesc_t *m = get_kip()->memory_info.get_memdesc(i);
	//TRACE_INIT("\t#%d, low=%x, high=%x, type=%x\n", i, 
	//   m->low(),  m->high(),  m->type());
    //}



}

/**********************************************************************
 *
 *  processor local initialization, performed by all IA32 CPUs
 *
 **********************************************************************/


/**
 * Setup global descriptor table 
 * 
 */

static void SECTION(SEC_INIT) init_gdt(amd64_tss_t &tss, cpuid_t cpuid){

    /* Initialize GDT */
    gdt.segdsc[GDT_IDX(AMD64_INVS)].set_seg((u64_t) 0, amd64_segdesc_t::inv, 0, amd64_segdesc_t::m_long);
    gdt.segdsc[GDT_IDX(AMD64_KCS)].set_seg((u64_t) 0, amd64_segdesc_t::code, 0, amd64_segdesc_t::m_long);
    gdt.segdsc[GDT_IDX(AMD64_KDS)].set_seg((u64_t) 0, amd64_segdesc_t::data, 0, amd64_segdesc_t::m_long);
    gdt.segdsc[GDT_IDX(AMD64_UCS)].set_seg((u64_t) 0, amd64_segdesc_t::code, 3, amd64_segdesc_t::m_long);
    gdt.segdsc[GDT_IDX(AMD64_UDS)].set_seg((u64_t) 0, amd64_segdesc_t::data, 3, amd64_segdesc_t::m_long);

    /* TODO: Assertion correct ? */
    ASSERT(unsigned(cpuid * AMD64_CACHE_LINE_SIZE) < AMD64_2MPAGE_SIZE);
    
    /* Set TSS */
    gdt.tssdsc.set_seg((u64_t) &tss, sizeof(tss) - 1);


    /* Load descriptor registers */
    amd64_descreg_t::setdescreg(amd64_descreg_t::gdtr, (u64_t) &gdt, sizeof(gdt));
    amd64_descreg_t::setdescreg(amd64_descreg_t::tr, AMD64_TSS);


    /*
     * As reloading fs/gs clobbers the upper 32bit of the segment descriptor 
     * registers, we have to set them twice:
     * - before loading the segment selectors (otherwise #GP because of invalid segment)
     * - after reloading the segment selectors  (otherwise upper 32 bits = 0)
     * registers 
     */ 
	
    gdt.segdsc[GDT_IDX(AMD64_UTCBS)].set_seg(UTCB_MAPPING + (cpuid * AMD64_CACHE_LINE_SIZE),
					     amd64_segdesc_t::data,
					     3, 
					     amd64_segdesc_t::m_long,
					     amd64_segdesc_t::msr_gs);


    /* Load segment registers */
    asm("mov  %0, %%ds		\n\t"		// load data  segment (DS)
	"mov  %0, %%es		\n\t"		// load extra segment (ES)
	"mov  %0, %%ss		\n\t"		// load stack segment (SS)
	"mov  %1, %%gs		\n\t"		// load UTCB segment  (GS)
	"mov  %0, %%fs		\n\t"	        // no tracebuffer
 	"pushq  %3	      	\n\t"		// new CS
 	"pushq $1f		\n\t"		// new IP		
 	"lretq			\n\t"
 	"1:			\n\t"	
	: /* No Output */ : "r" (0), "r" (AMD64_UTCBS), "r" (AMD64_TBS), "r" ((u64_t) AMD64_KCS)
	);
    
    
    gdt.segdsc[GDT_IDX(AMD64_UTCBS)].set_seg(UTCB_MAPPING + (cpuid * AMD64_CACHE_LINE_SIZE),
					     amd64_segdesc_t::data,
					     3, 
					     amd64_segdesc_t::m_long,
					     amd64_segdesc_t::msr_gs);
    
}
/**
 * setup_msrs: initializes all model specific registers for CPU
 */
static void setup_msrs()
{
    
    /* sysret (63..48) / syscall (47..32)  CS/SS MSR */
    amd64_wrmsr(AMD64_STAR_MSR, ((AMD64_SYSRETCS << 48) | (AMD64_SYSCALLCS << 32)));
    
    /* long mode syscalls MSR */
    amd64_wrmsr(AMD64_LSTAR_MSR, (u64_t)(syscall_entry));

    /* long mode syscall RFLAGS MASK  */
    amd64_wrmsr(AMD64_SFMASK_MSR, (u64_t)(AMD64_SYSCALL_FLAGMASK));

    /* enable syscall/sysret in EFER */
    word_t efer = amd64_rdmsr(AMD64_EFER_MSR);
    efer |= AMD64_EFER_SCE;
    amd64_wrmsr(AMD64_EFER_MSR, efer);
    


}

static cpuid_t SECTION(".init.cpu") init_cpu()
{
    cpuid_t cpuid = 0;

     /* Allow performance counters for users */
#if defined(CONFIG_PERFMON)
    amd64_cr4_set(AMD64_CR4_PCE);  
#endif

    TRACE_INIT("Enabling global pages (CPU %d)\n", cpuid);
    amd64_mmu_t::enable_global_pages();

#if defined(CONFIG_CPU_AMD64_OPTERON)
    TRACE_INIT("Enabling flush filter (CPU %d)\n", cpuid);
    amd64_hwcr_t::enable_flushfilter();
#endif
    /* configure IRQ hardware - local part */
    get_interrupt_ctrl()->init_cpu();

    /* set up task state segment */
    TRACE_INIT("Activating TSS (CPU %d)\n", cpuid);
    tss.init();

    /* set up global descriptor table */
    TRACE_INIT("Initializing GDT (CPU %d)\n", cpuid);
    init_gdt(tss, cpuid);
    
    /* activate idt */
    TRACE_INIT("Activating IDT (CPU %d)\n", cpuid);;
    idt.activate();
    
    /* activate msrs */
    TRACE_INIT("Activating MSRS (CPU %d)\n", cpuid);
    setup_msrs();
	
    /* initialize the kernel's timer source - per CPU part*/
    TRACE_INIT("Initializing Timer (CPU %d)\n", cpuid);
    get_timer()->init_cpu();

    /* initialize V4 processor info */
    TRACE_INIT("Initializing Processor (CPU %d)\n", cpuid);
    init_processor (cpuid, get_timer()->get_bus_freq(), 
		    get_timer()->get_proc_freq());
    


    return cpuid;
}     

/**
 * Entry Point into C Kernel
 * 
 * Precondition: 64bit-mode enabled
 *		 idempotent mapping and mapping at KERNEL_OFFSET
 *   
 */

extern "C" void SECTION(".init.init64") startup_system()
{

    /* init console */
    init_console(); 

    
    /* clear bss */
    clear_bss();
    
    /* call global constructors */
    call_global_ctors();

    /* call node constructors */
    call_node_ctors();

    /* say hello ... */ 
    init_hello();
   
    /* Get CPU Features */
    TRACE_INIT("Checking CPU features\n");
    check_cpu_features();

    /* feed the kernel memory allocator */
    TRACE_INIT("Initializing boot memory (%p - %p)\n",
		start_bootmem, end_bootmem);
    init_bootmem();


     /* initialize kernel space */
     TRACE_INIT("Initializing kernel space\n");
     init_kernel_space();

     {
	 /* copied here to catch errors early */
	 TRACE_INIT("Activating TSS (Preliminarily)\n");
	 tss.init();
	 /* set up global descriptor table */
	 TRACE_INIT("Initializing GDT (Preliminarily)\n");
	 init_gdt(tss, 0);
	 
	 /* activate idt */
	 TRACE_INIT("Activating IDT (Preliminarily)\n");
	 idt.activate();
     }

    /* initialize kernel interface page */
    TRACE_INIT("Initializing kernel interface page(%p)\n",
           get_kip());
    get_kip()->init();

 
    /* add additionall kernel memory */
    TRACE_INIT("Add more Kernel Memory\n");
    add_more_kmem();  
    
    /* init memory info */
    TRACE_INIT("Initializing memory info\n");
    init_meminfo(); 

    
     /* initialize mapping database */
    TRACE_INIT("Initializing mapping database\n");
    init_mdb ();
    
    /* initialize kernel debugger if any */
    TRACE_INIT("Initializing kernel debugger\n");
    if (get_kip()->kdebug_init)
	get_kip()->kdebug_init();
    
    /* configure IRQ hardware - global part */
    TRACE_INIT("Initializing IRQ hardware\n");
    get_interrupt_ctrl()->init_arch();

    /* initialize the kernel's timer source */
    TRACE_INIT("Initializing Timer\n");
    get_timer()->init_global();

    /* Initialize CPU */
    TRACE_INIT("Initializing CPU\n");
    cpuid_t cpuid = init_cpu();

    /* initialize the scheduler */
    get_current_scheduler()->init();
    /* get the thing going - we should never return */
    get_current_scheduler()->start(cpuid);
    
    /* make sure we don't fall off the edge */
    spin_forever(1);
}

