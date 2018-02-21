SLUG = dekstop
VERSION = 0.6.0dev

RACK_DIR ?= ../..

FLAGS += -Iportaudio
LDFLAGS += -lsamplerate

SOURCES = $(wildcard src/*.cpp portaudio/*.c)

include $(RACK_DIR)/plugin.mk

DISTRIBUTABLES += $(wildcard LICENSE*) res

