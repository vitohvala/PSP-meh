TARGET=hello
OBJS = main.o callbacks.o graphics.o framebuffer.o
CFLAGS = -Wall -Wextra -O2 -G0 
LIBS = -lpspgu -lpspgum -lpng -lz -lm -lpsppower 
ASFLAGS = $(CFLAGS)

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = img
PSPSDK = /usr/psp/sdk
include $(PSPSDK)/lib/build.mak

