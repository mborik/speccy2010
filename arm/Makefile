PREFIX = arm-none-eabi-

CC  = $(PREFIX)gcc
CXX = $(PREFIX)g++
CPP = $(PREFIX)g++
ASM = $(PREFIX)g++
OCP = $(PREFIX)objcopy

RM = rm -rf
MKDIR = mkdir -p

OBJEXT = o
ELFEXT = elf
MAPEXT = map
BINEXT = bin

SRCDIR   = .
OBJDIR   = .objs
BUILDDIR = build
APP_NAME = speccy2010

ELF_TARGET = $(BUILDDIR)/$(APP_NAME).$(ELFEXT)
MAP_TARGET = $(BUILDDIR)/$(APP_NAME).$(MAPEXT)
BIN_TARGET = $(BUILDDIR)/$(APP_NAME).$(BINEXT)

CFLAGS   = -mno-thumb-interwork -mcpu=arm7tdmi
CFLAGS  += -Os -Wall
CFLAGS  += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-exceptions
CPPFLAGS = -I$(SRCDIR) -I$(SRCDIR)/libstr/inc -I/usr/arm-none-eabi/include/
LDFLAGS  = -L$(SRCDIR)/system
LDFLAGS += -T$(SRCDIR)/system/STR75xFx2_FLASH.ld
LDFLAGS += -Wl,--gc-sections -fno-exceptions
LDFLAGS += -Wl,-Map=$(MAP_TARGET)

DIR_TARGETS = \
	$(OBJDIR)/fatfs/option \
	$(OBJDIR)/fatfs \
	$(OBJDIR)/betadisk \
	$(OBJDIR)/libnosys \
	$(OBJDIR)/libstr/src \
	$(OBJDIR)/system \
	$(OBJDIR) \
	$(BUILDDIR)

CC_SRCS = \
	$(SRCDIR)/75x_it.c \
	$(SRCDIR)/betadisk/bdi.c \
	$(SRCDIR)/betadisk/fdc.c \
	$(SRCDIR)/betadisk/floppy.c \
	$(SRCDIR)/betadisk/img_fdi.c \
	$(SRCDIR)/betadisk/img_odi.c \
	$(SRCDIR)/betadisk/img_scl.c \
	$(SRCDIR)/betadisk/img_trd.c \
	$(SRCDIR)/betadisk/trace.c \
	$(SRCDIR)/betadisk/wd1793.c \
	$(SRCDIR)/crc16.c \
	$(SRCDIR)/fatfs/diskio.c \
	$(SRCDIR)/fatfs/ff.c \
	$(SRCDIR)/fatfs/option/ccsbcs.c \
	$(SRCDIR)/libnosys/chown.c \
	$(SRCDIR)/libnosys/close.c \
	$(SRCDIR)/libnosys/execve.c \
	$(SRCDIR)/libnosys/exit.c \
	$(SRCDIR)/libnosys/fork.c \
	$(SRCDIR)/libnosys/fstat.c \
	$(SRCDIR)/libnosys/getpid.c \
	$(SRCDIR)/libnosys/gettod.c \
	$(SRCDIR)/libnosys/isatty.c \
	$(SRCDIR)/libnosys/kill.c \
	$(SRCDIR)/libnosys/link.c \
	$(SRCDIR)/libnosys/lseek.c \
	$(SRCDIR)/libnosys/mallocr.c \
	$(SRCDIR)/libnosys/open.c \
	$(SRCDIR)/libnosys/read.c \
	$(SRCDIR)/libnosys/readlink.c \
	$(SRCDIR)/libnosys/sbrk.c \
	$(SRCDIR)/libnosys/stat.c \
	$(SRCDIR)/libnosys/symlink.c \
	$(SRCDIR)/libnosys/times.c \
	$(SRCDIR)/libnosys/unlink.c \
	$(SRCDIR)/libnosys/wait.c \
	$(SRCDIR)/libnosys/write.c \
	$(SRCDIR)/libstr/src/75x_adc.c \
	$(SRCDIR)/libstr/src/75x_can.c \
	$(SRCDIR)/libstr/src/75x_cfg.c \
	$(SRCDIR)/libstr/src/75x_dma.c \
	$(SRCDIR)/libstr/src/75x_eic.c \
	$(SRCDIR)/libstr/src/75x_extit.c \
	$(SRCDIR)/libstr/src/75x_gpio.c \
	$(SRCDIR)/libstr/src/75x_i2c.c \
	$(SRCDIR)/libstr/src/75x_lib.c \
	$(SRCDIR)/libstr/src/75x_mrcc.c \
	$(SRCDIR)/libstr/src/75x_pwm.c \
	$(SRCDIR)/libstr/src/75x_rtc.c \
	$(SRCDIR)/libstr/src/75x_smi.c \
	$(SRCDIR)/libstr/src/75x_ssp.c \
	$(SRCDIR)/libstr/src/75x_tb.c \
	$(SRCDIR)/libstr/src/75x_tim.c \
	$(SRCDIR)/libstr/src/75x_uart.c \
	$(SRCDIR)/libstr/src/75x_wdg.c

CXX_SRCS = \
	$(SRCDIR)/cstring.cpp \
	$(SRCDIR)/fifo.cpp \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/ramFile.cpp \
	$(SRCDIR)/settings.cpp \
	$(SRCDIR)/specBetadisk.cpp \
	$(SRCDIR)/specConfig.cpp \
	$(SRCDIR)/specKeyboard.cpp \
	$(SRCDIR)/specKeyMap.cpp \
	$(SRCDIR)/specRtc.cpp \
	$(SRCDIR)/specShell.cpp \
	$(SRCDIR)/specSnapshot.cpp \
	$(SRCDIR)/specTape.cpp \
	$(SRCDIR)/uarts.cpp

ASM_SRCS = \
	$(SRCDIR)/system/str750_crt0.s


CC_OBJS  = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.$(OBJEXT),$(CC_SRCS))
CXX_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.$(OBJEXT),$(CXX_SRCS))
ASM_OBJS = $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.$(OBJEXT),$(ASM_SRCS))


all: build

$(CC_OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(CXX_OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(ASM_OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.s
	$(ASM) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(ELF_TARGET): $(CC_OBJS) $(CXX_OBJS) $(ASM_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)

$(BIN_TARGET): $(ELF_TARGET)
	$(OCP) -O binary $^ $@


build: mkdirs $(ELF_TARGET) $(BIN_TARGET)

mkdirs:
	$(MKDIR) $(DIR_TARGETS)

clean:
	$(RM) $(CC_OBJS) $(CXX_OBJS) $(ASM_OBJS)
	$(RM) $(ELF_TARGET) $(MAP_TARGET) $(BIN_TARGET)

distclean: clean
	$(RM) $(BUILDDIR) $(OBJDIR)


.SUFFIXES: .cpp .c .s
.PHONY: mkdirs clean distclean build all
