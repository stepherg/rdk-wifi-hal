# Makefile for wifi_hal_cli
#
# Build:   make
# Install: make install    (default PREFIX=/usr/local)
# Clean:   make clean

CC      ?= gcc
PREFIX  ?= /usr/local

# Pull compiler flags from pkg-config for the HAL dependencies
PKG_DEPS  := libhostap libnl-3.0 libnl-genl-3.0 libcjson
PKG_CFLAGS := $(shell pkg-config --cflags $(PKG_DEPS) 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs   $(PKG_DEPS) 2>/dev/null)

CFLAGS  += -Wall -Wextra -Werror -std=gnu11 -g -O2 -DWIFI_HAL_VERSION_3
CFLAGS  += -I/usr/include/ccsp $(PKG_CFLAGS)

LDFLAGS += -Wl,--allow-shlib-undefined -lhal_wifi -lrdk_wifihal $(PKG_LIBS) -lpthread -lrt -ldl

TARGET  := wifi_hal_cli
SRC     := wifi_hal_cli.c

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/
