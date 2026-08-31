CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS=-Iinclude

BUILD=build
SRC=src

COMMON=$(SRC)/common.c

TMAKEGEN=$(BUILD)/tmakegen
TSDKGEN=$(BUILD)/tsdkgen

TMAKEGEN_SRC=$(SRC)/tmakegen.c
TSDKGEN_SRC=$(SRC)/tsdkgen.c

.PHONY: all clean rebuild install dirs

all: dirs $(TMAKEGEN) $(TSDKGEN)

dirs:
	mkdir -p $(BUILD)

$(TMAKEGEN): $(TMAKEGEN_SRC) $(COMMON) include/common.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TMAKEGEN_SRC) $(COMMON) -o $(TMAKEGEN)

$(TSDKGEN): $(TSDKGEN_SRC) $(COMMON) include/common.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TSDKGEN_SRC) $(COMMON) -o $(TSDKGEN)

clean:
	rm -f $(TMAKEGEN)
	rm -f $(TSDKGEN)

rebuild: clean all

install: all
	install -Dm755 $(TMAKEGEN) /usr/local/bin/tmakegen
	install -Dm755 $(TSDKGEN) /usr/local/bin/tsdkgen

help:
	@echo "TinyDistro AppBuilder v2.3.0"
	@echo
	@echo "Targets:"
	@echo "  all       Build tmakegen and tsdkgen"
	@echo "  clean     Remove AppBuilder binaries"
	@echo "  rebuild   Clean and rebuild everything"
	@echo "  install   Install both tools"
	@echo "  help      Show this help"
