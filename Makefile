lib.name = goertzel

class.sources = goertzel~.c

datafiles = goertzel~-help.pd goertzel-vanilla~.pd

objectsdir = ./build
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
