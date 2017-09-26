
SOURCES = $(wildcard src/*.cpp src/*.c)

include ../../plugin.mk


dist: all
	mkdir -p dist/dekstop
	cp LICENSE* dist/dekstop/
	cp plugin.* dist/dekstop/
	cp -R res dist/dekstop/
