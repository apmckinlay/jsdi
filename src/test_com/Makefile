# Copyright 2013 (c) Suneido Software Corp. All rights reserved.
# Licensed under GPLv2.

#===============================================================================
# file: com-test/Makefile
# auth: Victor Schappert
# date: 20131017
# desc: Sub-Makefile to build COM testing static library (test_com.a)
#===============================================================================

OBJDIR          ?=.

#===============================================================================
# DIRECTORIES AND FILES
#===============================================================================

IDL_FILE        :=test_com.idl
RES_SCRIPT      :=test_com.rc
IMPL_HEADER     :=test_com.h
IMPL_SRC        :=test_com.cpp

GEN_TLB_FILE    :=midl.tlb
GEN_HEADER      :=midl.h
GEN_IID_FILE    :=midl_iid.c

IID_OBJECT      :=$(OBJDIR)/midl_iid.o
IMPL_OBJECT     :=$(OBJDIR)/test_com.o
TARGET_A        :=test_com.a

#===============================================================================
# TOOLS AND BASIC FLAGS
#===============================================================================

RES_TOOL:=windres

#===============================================================================
# TARGETS
#===============================================================================

$(OBJDIR)/$(TARGET_A): $(IID_OBJECT) $(IMPL_OBJECT)
	@echo $@
	@ar rcs $@ $^

$(GEN_HEADER) $(GEN_TLB_FILE) $(GEN_IID_FILE): $(IDL_FILE)
	@echo $@
	@midl $(IDL_FILE) //h $(GEN_HEADER) //tlb $(GEN_TLB_FILE) //iid $(GEN_IID_FILE)

# NOTE: The reason the below is compiling midl_iid.c with -x c and no CC_FLAGS
#       is that to get the symbols exported properly as "C" symbols you have to
#       compile as a "C" source file. This is because at least with gcc 4.6.2,
#       the extern "C" { } block does NOT cause 'const' variables declared
#       within the block to get external linkage as "C" symbols. They instead
#       get internal linkage as C++ symbols. 
$(IID_OBJECT): $(GEN_IID_FILE)
	@echo $@
	$(CC) -x c -O3 -DNDEBUG -c $(GEN_IID_FILE) -o $@

$(IMPL_OBJECT): $(GEN_HEADER) $(IMPL_HEADER) $(IMPL_SRC)
	@echo $@
	@$(CC) $(CC_FLAGS) -c $(IMPL_SRC) -o $@

# NOTE: The TLB file is currently being built by the 'res' sub-project, not
#       this sub-project because of a limitation of MinGW which prevents it
#       from linking more than one object file containing resources into the
#       final project.
#       See: http://stackoverflow.com/a/19509850/1911388

.PHONY: clean
clean:
	@rm -rf $(IID_OBJECT) $(IMPL_OBJECT) $(OBJDIR)/$(TARGET_A)
