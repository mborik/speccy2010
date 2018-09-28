CD     = cd
CAT    = cat
RM     = rm -f

MAKE   = make
CC     = gcc
CFLAGS = -Os -s

SJASMPLUS = sjasmplus --nologo

BIN2H  = _utils/bin2h
PACKER = _utils/pack

BOOTLOGO_SRC = bootlogo/speccy2010.logo.scr
BOOTLOGO_DST = ../arm/shell/logo.h

#------------------------------------------------------------------------------
default: all

all: logo snaploader bsdos mb02


mb02: bsdos
	$(BIN2H) bsdos308s/bootstrap.bin "epromBootstrap" "SPEC_MB02_BOOTSTAP_H_INCLUDED" > bsdos308s/bootstrap.h

bsdos: bsdos308s/bsdos308s.a80
	$(CD) bsdos308s; $(SJASMPLUS) --lst=bsdos308s.lst bsdos308s.a80; \
	$(CAT) Xch140x1.bin bsdos308s.bin > bsdos308s.rom

snaploader: snaploader/loader.a80
	$(CD) snaploader; $(SJASMPLUS) loader.a80
	$(BIN2H) snaploader/loader.bin "snapshotBootstrap" "SPEC_SNAPSHOT_BOOTSTRAP_H_INCLUDED" > snaploader/loader.h

logo: bin2h bootlogo
	$(PACKER) $(BOOTLOGO_SRC)
	$(BIN2H) "$(BOOTLOGO_SRC).bin" "logoPacked" "SHELL_LOGO_H_INCLUDED" > $(BOOTLOGO_DST)

bootlogo: $(PACKER).c
	$(CC) $(CFLAGS) -o $(PACKER) $<

bin2h: $(BIN2H).c
	$(CC) $(CFLAGS) -o $(BIN2H) $<

clean:
	@$(RM) \
		$(BOOTLOGO_SRC).bin \
		snaploader/loader.bin \
		snaploader/loader.h \
		bsdos308s/bsdos308s.lst \
		bsdos308s/bsdos308s.bin \
		bsdos308s/bsdos308s.rom \
		bsdos308s/bootstrap.bin \
		bsdos308s/bootstrap.h

distclean: clean
	$(RM) $(BIN2H) $(PACKER)

.PHONY: bin2h bootlogo snaploader bsdos clean distclean
.SUFFIXES: .c .a80
