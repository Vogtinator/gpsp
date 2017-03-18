# gpSP makefile
# Gilead Kutnick - Exophase
# TI-Nspire port - calc84maniac

GCC = nspire-gcc
LD = nspire-ld
GENZEHN = genzehn
VPATH += ..
GCCFLAGS = -O3 -Wfatal-errors -W -std=c99 -msoft-float -funsigned-char -DNSPIRE_BUILD -DARM_ARCH -flto
LDFLAGS = -flto
ZEHNFLAGS = --version 12 --author "calc86maniac" --clickpad-support=false --color-support=true --compress --ndless-min 31 --ndless-rev-min 2001
OBJS      = main.o cpu.o memory.o video.o input.o sound.o gui.o               \
            cheats.o zip.o cpu_threaded.o arm_stub.o video_blend.o            \
            upscale_aspect.o nspire.o
DISTDIR = .
vpath %.tns $(DISTDIR)

all: gpsp_launcher.tns

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $<

%.o: %.S
	$(GCC) $(GCCFLAGS) -c $<

%.o: %.s
	$(GCC) $(GCCFLAGS) -c $<
	
gpsp_launcher.elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

gpsp_launcher.tns: gpsp_launcher.elf
	$(GENZEHN) $(ZEHNFLAGS) --input $^ --output $@

clean:
	rm -f *.o *.elf
	rm -f $(DISTDIR)/gpsp_resources.tns
