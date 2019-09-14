# global options 
CFLAGS+=-I$(top)/lib
CFLAGS += -g
CFLAGS += -D _USR_SYS_INIT_KILL
CFLAGS += -Wall -Wno-format -Wno-unused-label -Wno-unused-variable
CFLAGS += -fmessage-length=0
CFLAGS+=-fPIC 
#CFLAGS+=-DDEBUG # use for the global debugging
