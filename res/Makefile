#=============================================================================
# file: res/Makefile
# auth: Victor Schappert
# date: 20130830
# desc: Sub-Makefile to build resource.o
#=============================================================================

RES_SRCDIR:=res
RES_TARGET:=$(OBJDIR)/resource.o

RES_HEADER:=resource.h
RES_SCRIPT:=suneido.rc
RES_SOURCES:=$(wildcard $(RES_SRCDIR)/*)

RES_TOOL:=windres

$(RES_TARGET) : $(RES_HEADER) $(RES_SCRIPT) $(RES_SOURCES)
	$(RES_TOOL) $(RES_SCRIPT) $(RES_TARGET)