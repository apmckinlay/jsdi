#=============================================================================
# file: Makefile
# auth: Victor Schappert
# date: 20130618
# desc: GNU-style makefile for jsdi -- the JSuneido DLL Interface
#=============================================================================

CONFIG  ?=debug

#=============================================================================
# DIRECTORIES AND TARGETS
#=============================================================================

SRCDIR  :=src
OBJDIR  :=obj/$(CONFIG)
BINDIR  :=bin/$(CONFIG)
DEPDIR  :=.dep
DOCDIR  :=doc
ASMDIR  :=.asm/$(CONFIG)
PPDIR   :=.pp/$(CONFIG)

EXPORTS     :=jsdi.exp
TARGET_DLL  :=jsdi.dll
TARGET_EXE  :=jsdi.exe
SOURCES     :=$(wildcard $(SRCDIR)/*.cpp)
OBJECTS     :=$(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
OBJECTS_DLL :=$(filter-out $(OBJDIR)/main_exe.o, $(OBJECTS))
OBJECTS_EXE :=$(filter-out $(OBJDIR)/main_dll.o, $(OBJECTS))
ASMFILES    :=$(SOURCES:$(SRCDIR)/%.cpp=$(ASMDIR)/%.s)
PPFILES     :=$(SOURCES:$(SRCDIR)/%.cpp=$(PPDIR)/%.cpp)
DEPENDS     :=$(SOURCES:$(SRCDIR)/%.cpp=$(DEPDIR)/%.d)
DOXYFILE    :=$(DOCDIR)/Doxyfile

#=============================================================================
# TOOLS AND BASIC FLAGS
#=============================================================================

# NEEDED to prepend '_' to __stdcall function names when built with MinGW.
# http://stackoverflow.com/q/17199904/1911388
DLLTOOL:=dlltool --add-stdcall-underscore

CC:=g++
CC_STDFLAGS:=-std=gnu++0x -D__GXX_EXPERIMENTAL_CXX0X__
CC_INCLUDE_DIRS:="-I$(JAVA_HOME)/include" "-I$(JAVA_HOME)/include/win32"
CC_FLAGS=-O$(OPTLEVEL) $(DEBUGOPTION) $(CC_STDFLAGS) -Wall -fmessage-length=0 \
         $(CC_INCLUDE_DIRS)

LD_DLL:=g++ $(BINDIR)/$(EXPORTS) -o
LD_EXE:=g++ -o
LD_FLAGS_DLL=-shared

#=============================================================================
# CONFIGURATION-SPECIFIC FLAGS
#=============================================================================

# Note that because conditionals are parsed immediately (in make's first pass),
# you can't make file-scope conditionals depend on target-specific variables.
# So to get different configurations, you have to set the CONFIG variable before
# make is invoked. For examplem, from the shell: '$ make CONFIG=release'. See
# section 3.7 of the GNU Make manual.

ifeq ($(CONFIG),debug)
OPTLEVEL:=0
DEBUGOPTION:=-g
LD_FLAGS_DLL+= $(DEBUGOPTION)
else ifeq ($(CONFIG),release)
OPTLEVEL:=3
DEBUGOPTION:=-DNDEBUG
LD_FLAGS_DLL+= -s
else
$(error Unsupported CONFIG value: '$(CONFIG)')
endif

#=============================================================================
# TARGETS
#=============================================================================

.PHONY: build
build: dll exe

.PHONY: dll
dll: dirs $(BINDIR)/$(TARGET_DLL)

.PHONY: exe
exe: dirs $(BINDIR)/$(TARGET_EXE)

.PHONY: asm
asm: dirs $(ASMFILES)

.PHONY: pp
pp: dirs $(PPFILES)

# Including DEPENDS will trigger dependency generation if the dependency files
# don't yet exist. You have to include it after the default target, though, or
# make will assume that the first dependency file *is* the default target! The
# dependency system in this makefile is copied from 
# http://stackoverflow.com/a/2045668/1911388, which is a pretty good post on
# how to set up dependencies w the gcc toolchain.
-include $(DEPENDS)

$(BINDIR)/$(TARGET_DLL): $(BINDIR)/$(EXPORTS)
	@echo $@
	@$(LD_DLL) $@ $(LD_FLAGS_DLL) $(OBJECTS_DLL)

$(BINDIR)/$(TARGET_EXE): $(OBJECTS_EXE)
	@echo $@
	@$(LD_EXE) $@ $(OBJECTS_EXE)

$(BINDIR)/$(EXPORTS): $(OBJECTS_DLL)
	@echo $@
	@$(DLLTOOL) -e $@ -D $(TARGET_DLL) $(OBJECTS_DLL)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@echo $<
	@$(CC) $(CC_FLAGS) -c $< -o $@

$(DEPENDS): $(DEPDIR)/%.d : $(SRCDIR)/%.cpp
	@$(CC) $(CC_FLAGS) -MF"$@" -MG -MM -MP \
                      -MT"$@" -MT"$(<:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)" "$<"

$(ASMFILES): $(ASMDIR)/%.s : $(SRCDIR)/%.cpp
	@echo $<
	@$(CC) $(CC_FLAGS) -S $< -o $@

$(PPFILES): $(PPDIR)/%.cpp : $(SRCDIR)/%.cpp
	@echo $<
	@$(CC) $(CC_FLAGS) -E $< -o $@

.PHONY: dirs
dirs: $(OBJDIR) $(BINDIR) $(DEPDIR) $(ASMDIR) $(PPDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

$(DEPDIR):
	@mkdir $(DEPDIR)

$(ASMDIR):
	@mkdir -p $(ASMDIR)

$(PPDIR):
	@mkdir -p $(PPDIR)

.PHONY: clean
clean:
	@rm -f $(OBJDIR)/* $(BINDIR)/* $(DEPDIR)/* $(ASMDIR)/* $(PPDIR)/*

.PHONY: rebuild
rebuild: clean build

.PHONY: docs
docs:
	@doxygen $(DOXYFILE) >/dev/null
