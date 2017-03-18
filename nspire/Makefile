# gpSP makefile
# Gilead Kutnick - Exophase
# TI-Nspire port - calc84maniac

GCC = nspire-gcc-global
LD = nspire-ld
VPATH += ..
GCCFLAGS = -O3 -Wfatal-errors -W -std=c99 -msoft-float -funsigned-char -DNSPIRE_BUILD -DARM_ARCH
LDFLAGS = -Wl,--emit-relocs
OBJCOPY := "$(shell which arm-elf-objcopy 2>/dev/null)"
ifeq (${OBJCOPY},"")
	OBJCOPY := arm-none-eabi-objcopy
endif
OBJS      = main.o cpu.o memory.o video.o input.o sound.o gui.o               \
            cheats.o zip.o cpu_threaded.o arm_stub.o video_blend.o            \
            upscale_aspect.o nspire.o
DISTDIR = .
vpath %.tns $(DISTDIR)

all: gpsp_resources.tns

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $<

%.o: %.S
	$(GCC) $(GCCFLAGS) -c $<

%.o: %.s
	$(GCC) $(GCCFLAGS) -c $<
	
gpsp_resources.tns: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o *.elf
	rm -f $(DISTDIR)/gpsp_resources.tns