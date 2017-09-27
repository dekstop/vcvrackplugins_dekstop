
SOURCES = $(wildcard src/*.cpp portaudio/*.c)

include ../../plugin.mk

FLAGS += -Iportaudio

dist: all
	mkdir -p dist/dekstop
	cp LICENSE* dist/dekstop/
	cp plugin.* dist/dekstop/
	cp -R res dist/dekstop/
