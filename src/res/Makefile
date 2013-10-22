# Copyright 2013 (c) Suneido Software Corp. All rights reserved.
# Licensed under GPLv2.

#===============================================================================
# file: res/Makefile
# auth: Victor Schappert
# date: 20130830
# desc: Sub-Makefile to build resource.o
#===============================================================================

RES_SRCDIR:=res
RES_TARGET:=$(OBJDIR)/resource.o

RES_HEADER:=resource.h
RES_SCRIPT:=suneido.rc
RES_SOURCES:=$(wildcard $(RES_SRCDIR)/*)

RES_TOOL:=windres

# Generated type library file used by the COM testing module. This has to be
# included in the generated resource object. See note in suneido.rc.
GEN_TLB_FILE:=../test_com/midl.tlb

$(RES_TARGET) : $(RES_HEADER) $(RES_SCRIPT) $(RES_SOURCES) $(GEN_TLB_FILE)
	@echo $(RES_TARGET)
	@$(RES_TOOL) $(RES_SCRIPT) $(RES_TARGET)