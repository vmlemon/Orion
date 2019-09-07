Import("*")

# Get the user and hostname, and current date
import os, sys, time

# Cygwin python seem to have a bug where socket and
# fork don't work so well together.
if sys.platform != "cygwin":
    import socket
    hostname = socket.gethostname()
else:
    hostname = "cygwin"
    
user = os.getenv("USER", "unknown")
(year, month, day) =  time.localtime()[:3]

generic_src_files = ["kernel/src/generic/" + x for x in
                     Split("""lib.cc mapping.cc kmemory.cc
                     linear_ptab_walker.cc mapping_alloc.cc traceids.cc""")]

generic_kdb_files = ["kernel/kdb/generic/" + x for x in
                     Split("""init.cc bootinfo.cc console.cc kmemory.cc linker_set.cc
                     memdump.cc sprintf.cc tracepoints.cc cmd.cc entry.cc
                     input.cc linear_ptab_dump.cc mapping.cc print.cc tid_format.cc""") ]

# Check that machine has valid arch, platform
archs = [os.path.split(arch)[-1] for arch in src_glob("kernel/src/arch/*")]
platforms = [os.path.split(plat)[-1] for plat in src_glob("kernel/src/platform/*")]

arch = getattr(env.machine, "arch", None)
platform = getattr(env.machine, "platform", None)
cpu = getattr(env.machine, "cpu", None)

if arch not in archs:
    raise UserError, "The specified machine doesn't have a valid architecture. Must be one of: %s" % archs

if platform not in platforms:
    raise UserError, "The specified machine platform <%s> isn't valid. Must be one of: %s" % (platform, platforms)

if cpu is None:
    raise UserError, "The specified machine CPU <%s> isn't valid." % cpu

src = [
    "kernel/src/arch/%s/*.cc" % arch,
    "kernel/src/api/v4/*.cc",
    "kernel/src/glue/v4-%s/*.S" % arch,
    "kernel/src/arch/%s/*.S" % arch,
    "kernel/src/api/v4/*.S",
    "kernel/src/generic/*.S",
    ] + generic_src_files

if arch == "ia32":
    src += ["kernel/src/glue/v4-ia32/" + fn for fn in
            ["ctors.cc", "exception.cc", "init.cc", "space.cc",
             "timer.cc", "user.cc", "debug.cc", "idt.cc",
             "resources.cc", "thread.cc"]]
else:
    glue_src = src_glob("kernel/src/glue/v4-%s/*.cc" % arch)
    glue_src = [fn for fn in glue_src if "asmsyms.cc" not in fn]
    src += glue_src


if platform == "pc99":
    src += ["kernel/src/platform/pc99/" + fn for fn in
            ["startup.S", "intctrl-pic.cc"]]
    src += ["kernel/src/platform/generic/intctrl-pic.cc"]
else:
    src += ["kernel/src/platform/%s/*.cc" % platform,
            "kernel/src/platform/%s/*.S" % platform]

kdb_src = [
    "kernel/kdb/api/v4/*.cc",
    "kernel/kdb/platform/%s/*.cc" % platform,
    ] + generic_kdb_files

if arch == "ia32":
    kdb_src += ["kernel/kdb/glue/v4-ia32/" + fn for fn in
            ["prepost.cc", "readmem.cc", "resources.cc",
             "space.cc"]]
    kdb_src += ["kernel/kdb/arch/ia32/" + fn for fn in
            ["x86.cc", "breakpoints.cc", "stepping.cc"]]
else:
    kdb_src += ["kernel/kdb/glue/v4-%s/*.cc" % arch,
            "kernel/kdb/arch/%s/*.cc" % arch]


version_cppdefines = [
    ("SCONS_BUILD", 1),
    ("__USER__", '\\"%s@%s\\"' % (user, hostname) ),
    ("KERNEL_VERSION", 0), ("KERNEL_SUBVERSION", 1),
    ("KERNEL_SUBSUBVERSION", 0), ("KERNEL_GEN_DAY", day),
    ("KERNEL_GEN_MONTH", month), ("KERNEL_GEN_YEAR", year - 2000),
    ]

config_defines = [
    ("CONFIG_DEBUG", 1),
    ("CONFIG_KMEM_TRACE", 1),
    ("CONFIG_TRACEPOINTS", 1),
    ("CONFIG_KDB", 1),
    ("CONFIG_ASSERT_LEVEL", 2),
    ("CONFIG_KDB_CONS_COM", 1),
    ]

cppdefines = [
    ("__API__", "v4"),
    ("__ARCH__", arch),
    ("__CPU__", env.machine.cpu),
    ("__PLATFORM__", platform),
    
    ("CONFIG_ARCH_%s" % arch.upper(), 1),
    ("CONFIG_PLAT_%s" % platform.upper(), 1),
    ("CONFIG_CPU_%s_%s" % (arch.upper(), env.machine.cpu.upper()), 1),

    ] + version_cppdefines + config_defines

if arch == "mips64":
    cppdefines += [("CONFIG_CPU_CLOCK_SPEED", 100000)]
print "arch", arch
if arch in ["mips64"]:
    print "ARCH!!", arch
    cppdefines += [("CONFIG_MAX_NUM_ASIDS", 256)]

if arch in ["mips64", "arm"]:
    cppdefines += [("CONFIG_BOOTMEM_PAGES", args.get("bootmem_pages", 1024))]

if args.get("enter_kdb", False):
    cppdefines += [("CONFIG_KDB_ON_STARTUP", 1)]

if args.get("enable_fastpath", False):
    cppdefines += [("CONFIG_EXCEPTION_FASTPATH", 1),
                   ("CONFIG_IPC_FASTPATH", 1)]

if args.get("verbose_init", True):
    cppdefines += [("CONFIG_VERBOSE_INIT", 1)]

if arch == "arm":
    if args.get("enable_fass", True):
        cppdefines += [("CONFIG_ENABLE_FASS", 1)]

if arch in ["arm", "mips64", "powerpc64", "ia64"]:
    cppdefines += [("CONFIG_HAVE_MEMORY_CONTROL", 1)]

if env.machine.endian == "big":
    cppdefines += [("CONFIG_BIGENDIAN", 1)]

if env.machine.wordsize == 32:
    cppdefines +=  [("CONFIG_IS_32BIT", 1)]
elif env.machine.wordsize == 64:
    cppdefines +=  [("CONFIG_IS_64BIT", 1)]
else:
    raise UserError, "Wordsize: %s is not support. Only 32 and 64-bit wordsize are supported." % env.machine.wordsize

env.Append(CPPPATH=["kernel/include"], CPPDEFINES=cppdefines,
           CXX_WARNINGS=["no-uninitialized"], ASPPFLAGS = ["-DASSEMBLY"])

env.Replace(CCFLAGS=["-finline-limit=999999999", "-g",
                     "-nostdinc",
                     "-fno-builtin", "-fno-exceptions", "-fomit-frame-pointer"],
            CXX_OPTIMISATIONS = [2])


class _genstring:
    def __call__(self, targets, source, env):
        return "=> Generating %s" % " ".join([str(x) for x in targets])
    def __str__(self):
        return "=> Generating $TARGETS"
genstring = _genstring()

# SCons.voodoo
def mk_asmsyms(target, source, env):
    out = open(target[0].abspath, "w")
    f = open(source[0].abspath)

    out.write("/* machine-generated file - do NOT edit */\n")
    out.write("#ifndef __ASMSYMS_H__\n")
    out.write("#define __ASMSYMS_H__\n\n")

    val_low = val_high = 0

    for line in f:
        info = line.strip().split()
        name = "_".join(info[-1].split("_")[:-1])
        if (len(info) == 4):
            val = long(info[1])
        else:
            val = 0
        val /= 32
        thing = info[-1].split("_")[-1]
        if thing == "sign":
            sign = ""
            if (val != 2):
                sign = "-"
            out.write("#define %-25s (%s(0x%x%x))\n" % (name, sign, val_high, val_low))
            val_low = val_high = 0
        else:
            num = long(thing[1])
            if num <= 3:
                val_low += val << (num * 8)
            else:
                val_high += val << ((num-4) * 8)
    out.write("\n#endif /* __ASMSYMS_H__ */\n")

asmsyms = env.StaticObject("kernel/src/glue/v4-%s/asmsyms.cc" % arch, CXX_WARNINGS = [])
asmsyms_nm = env.Command("asymsyms.o.nm", asmsyms, Action("$NM --radix=d -S $SOURCE > $TARGET", genstring))
asmsyms_h = env.Command(["kernel/include/asmsyms.h"], [asmsyms_nm], Action(mk_asmsyms, genstring))

class Values(elf.AIStruct):
	def __init__(self, *args, **kwargs):
		elf.AIStruct.__init__(self, elf.AIStruct.SIZE32)
		self.setup(
			('UINT32', 'value')
		)

        def __str__(self):
            return "0x%x" % self.ai.value.get()

def mk_tcblayout(target, source, env):
    out = open(target[0].abspath, "w")
    elffile = elf.ElfFile.from_file(source[0].abspath)
    offsets = elffile.sheaders[".offsets"].container(Values)
    out.write("""/* machine-generated file - do NOT edit */
#ifndef __TCB_LAYOUT__H__
#define __TCB_LAYOUT__H__
    """)

    for each in elffile.symtable.get_symbols(elffile.sheaders[".offsets"].index):
        if "OFS" in str(each.ai.st_name):
            off = offsets[each.ai.value.get() / 4].ai.value.get()
            name = str(each.ai.st_name).split("__")[-1]
            out.write("#define %-35s 0x%02x /* %4d */\n" % (name, off, off))
    out.write("#endif /* __TCB_LAYOUT__H__ */\n")


#
# Voodoo: This next bit is for generic tcb_layout.h. Doing so requires a few steps
# though
#

def mk_tcblayout_c(target, source, env):
    out = open(target[0].abspath, "w")
    out.write("""
#define BUILD_TCB_LAYOUT
#include <l4.h>
#include INC_API(tcb.h)

#define O(w,x) (u32_t)((char*) &w.x - (char*)(&w))

tcb_t tcb;
utcb_t utcb;

void make_offsets() {
""")
    for fn in source:
        printme = False
        for line in open(fn.abspath).readlines():
            if "utcb" in str(fn):
                struct_name = "utcb"
            else:
                struct_name = "tcb"
            line = line.strip()
            if "TCB_START_MARKER" in line:
                printme = True
                continue
            if "TCB_END_MARKER" in line:
                printme = False
                continue
            if printme:
                line = line.split(';')
                if len(line) > 1:
                    line = line[0] + ";"
                else:
                    line = line[0]
                if line.endswith(';'):
                    name = line.split()[-1][:-1].split('[')[0]
                    out.write('static u32_t __OFS_%s_%s __attribute__((unused,section(".offsets"))) = O(%s, %s) ;\n' %
                              (struct_name.upper(), name.upper(), struct_name, name))
                elif line.startswith('#'):
                    out.write(line + "\n")
    out.write("\n}\n")

env.StaticObject("tcb_layout.cc")
env.Command(["tcb_layout.cc"],
        ["kernel/include/api/v4/tcb.h",
         "kernel/include/glue/v4-%s/ktcb.h" % arch,
         "kernel/include/glue/v4-%s/utcb.h" % arch],
        Action(mk_tcblayout_c, genstring))


env.Command(["kernel/include/tcb_layout.h"], ["tcb_layout.o"], Action(mk_tcblayout, genstring))

def mk_kdb_helper(target, source, env):
    out = file(target[0].abspath, "w")
    for fn in source:
        for line in open(fn.abspath).readlines():
            line = line.strip()
            # FIXME: Maybe use a regexp here?
            if "DECLARE_CMD (" in line or "DECLARE_CMD(" in line:
                name = line.split("(", 2)[1].split(",")[0]
                out.write("static cmd_ret_t %s(cmd_group_t*);\n" % (name))

env.Command(["kernel/include/kdb_class_helper.h"], ["kernel/include/kdb/cmd.h"] +
            Flatten([src_glob(glob) for glob in kdb_src]),
            Action(mk_kdb_helper, genstring))

if platform == "pc99":
    linker_script_src = ["kernel/src/glue/v4-ia32/linker-pc99.lds"]
else:
    linker_script_src = ["kernel/src/platform/%s/linker.lds" % platform]

link_script = env.Command(["lds.tmp"], linker_script_src,
                          Action("$CPPCOM", genstring),
                          CPPFLAGS = env["CPPFLAGS"] +  ["-P", "-C"],
                          CPPDEFINES=env["CPPDEFINES"] + ["ASSEMBLY"])

obj = env.KengeProgram("l4kernel", source = src + kdb_src,
                       LINKSCRIPTS = link_script,
                       LINKFLAGS = ["-static", "-O2"],
                       LIBS=["gcc"])

[env.Depends(obj_,  ["kernel/include/tcb_layout.h"]) for obj_ in obj.children()]

Return("obj")
