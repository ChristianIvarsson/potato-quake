CPP=arm-linux-gnueabihf-g++
CC=arm-linux-gnueabihf-gcc
LD=arm-linux-gnueabihf-ld
AS=arm-linux-gnueabihf-as

LIBS= -lm

# There's a couple of multidimensional arrays in there and pointers are cast back and forth like there's no tomorrow.
# Don't get me started on strings...
# Wno-shift-overflow is due to edge_sentinel.u = 2000 << 24; in r_edge.c
# Given the other black magic I'm affraid of changing it but 208 << 24 "should" yield the same result.
CSHUTUP= -Wno-pointer-sign -Wno-pointer-to-int-cast -Wno-strict-aliasing \
		 -Wno-trigraphs -trigraphs -fstack-protector-all -Wno-shift-overflow -Wno-format-overflow -Wno-stringop-truncation -Wno-aggressive-loop-optimizations

# Additional flags to alter engine behaviour
# -DNOSURFCACHE   - Do not use a full surface cache for lighting effects
# -DNOWARP        - Do not warp the screen when under water (saves another 64~ k of ram)
# -DTRACE_STACK   - Trace max stack depth (_VERY_ heavy)
ENGFLAGS= -DNOSURFCACHE -DNOWARP -DTRACE_STACK

# -fdata-sections -ffunction-sections
CFLAGS= -std=gnu99 -Og -ggdb -Wall -Wpedantic -Werror $(ENGFLAGS) $(CSHUTUP)
# -Wl,--gc-sections
LDFLAGS= -Wl,-Map=quake.map


.PHONY: all upload clean showsz

all: gendir saybuild quake

gendir:
	@mkdir -p out/rend

# "root buhuhu..."
# Easiest is to add a fingerprint so you don't have to enter a password every time
upload:
	@echo "Uploading.."
	@scp quake root@hallonpaj:/mnt/usb/quake

saybuild:
	@echo "Building.."

HOSTOBJECTS := $(patsubst %.c, out/%.o, $(wildcard *.c))
out/%.o: %.c
	@$(CC) $(CFLAGS) $< -c -o $@

COREOBJECTS := $(patsubst ../core/%.c, ./out/%.o, $(wildcard ../core/*.c))
out/%.o: ../core/%.c
	@$(CC) $(CFLAGS) $< -c -o $@

RENDOBJS := $(patsubst ../core/render/original/%.c, ./out/rend/%.o, $(wildcard ../core/render/original/*.c))
out/rend/%.o: ../core/render/original/%.c
	@$(CC) $(CFLAGS) $< -c -o $@

QUAKEOBJS=\
	$(HOSTOBJECTS) \
	$(COREOBJECTS) \
	$(RENDOBJS) \


quake: $(QUAKEOBJS)
	@echo "Linking..."
	@$(CC) $(LDFLAGS) -o $@ $(QUAKEOBJS) $(LIBS)
	@echo
#	@./sizehelp

showsz:
	@./sizehelp
	@echo

clean:
	@echo "Cleaning.."
	@rm -f out/*.o
	@rm -f out/rend/*.o
	@rm -f quake
