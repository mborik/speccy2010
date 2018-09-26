PREFIX = arm-none-eabi-

CC  = $(PREFIX)gcc
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
BASEDIR  = ../arm
OBJDIR   = .objs
BUILDDIR = build
APP_NAME = speccy2010_boot

ELF_TARGET = $(BUILDDIR)/$(APP_NAME).$(ELFEXT)
MAP_TARGET = $(BUILDDIR)/$(APP_NAME).$(MAPEXT)
BIN_TARGET = $(BUILDDIR)/$(APP_NAME).$(BINEXT)

CFLAGS   = -mno-thumb-interwork -mcpu=arm7tdmi
CFLAGS  += -Os -Wall
CFLAGS  += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-exceptions
CPPFLAGS = -I$(SRCDIR) -I$(BASEDIR)/libstr/inc -I/usr/arm-none-eabi/include/
LDFLAGS  = -L$(SRCDIR)/core
LDFLAGS += -T$(SRCDIR)/core/STR75xFx2_FLASH.ld
LDFLAGS += -Wl,--gc-sections -fno-exceptions
LDFLAGS += -Wl,-Map=$(MAP_TARGET)

DIR_TARGETS = \
	$(OBJDIR)/core \
	$(OBJDIR)/fatfs/option \
	$(OBJDIR)/fatfs \
	$(OBJDIR)/libstr/src \
	$(OBJDIR) \
	$(BUILDDIR)

LIB_SRCS = \
	$(BASEDIR)/libstr/src/75x_adc.c \
	$(BASEDIR)/libstr/src/75x_can.c \
	$(BASEDIR)/libstr/src/75x_cfg.c \
	$(BASEDIR)/libstr/src/75x_dma.c \
	$(BASEDIR)/libstr/src/75x_eic.c \
	$(BASEDIR)/libstr/src/75x_extit.c \
	$(BASEDIR)/libstr/src/75x_gpio.c \
	$(BASEDIR)/libstr/src/75x_i2c.c \
	$(BASEDIR)/libstr/src/75x_lib.c \
	$(BASEDIR)/libstr/src/75x_mrcc.c \
	$(BASEDIR)/libstr/src/75x_pwm.c \
	$(BASEDIR)/libstr/src/75x_rtc.c \
	$(BASEDIR)/libstr/src/75x_smi.c \
	$(BASEDIR)/libstr/src/75x_ssp.c \
	$(BASEDIR)/libstr/src/75x_tb.c \
	$(BASEDIR)/libstr/src/75x_tim.c \
	$(BASEDIR)/libstr/src/75x_uart.c \
	$(BASEDIR)/libstr/src/75x_wdg.c \
	$(BASEDIR)/libstr/src/75x_flash.c

CC_SRCS = \
	$(SRCDIR)/fatfs/diskio.c \
	$(SRCDIR)/fatfs/ff.c \
	$(SRCDIR)/fatfs/option/ccsbcs.c \
	$(SRCDIR)/core/str75x_it.c \
	$(SRCDIR)/main.c \

ASM_SRCS = \
	$(SRCDIR)/core/str75x_crt0.s \
	$(SRCDIR)/core/str75x_vect.s


LIB_OBJS = $(patsubst $(BASEDIR)/%.c,$(OBJDIR)/%.$(OBJEXT),$(LIB_SRCS))
CC_OBJS  = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.$(OBJEXT),$(CC_SRCS))
ASM_OBJS = $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.$(OBJEXT),$(ASM_SRCS))


all: build

$(CC_OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(LIB_OBJS): $(OBJDIR)/%.o : $(BASEDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(ASM_OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.s
	$(ASM) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(ELF_TARGET): $(LIB_OBJS) $(CC_OBJS) $(ASM_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)

$(BIN_TARGET): $(ELF_TARGET)
	$(OCP) -O binary $^ $@


build: mkdirs $(ELF_TARGET) $(BIN_TARGET)

mkdirs:
	$(MKDIR) $(DIR_TARGETS)

clean:
	$(RM) $(CC_OBJS) $(LIB_OBJS) $(ASM_OBJS)
	$(RM) $(ELF_TARGET) $(MAP_TARGET) $(BIN_TARGET)

distclean: clean
	$(RM) $(BUILDDIR) $(OBJDIR)


.SUFFIXES: .c .s
.PHONY: mkdirs clean distclean build all
