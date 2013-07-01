#=============================================================================
# file: Makefile
# auth: Victor Schappert
# date: 20130618
# desc: GNU-style makefile for jsdi -- the JSuneido DLL Interface
#=============================================================================

SRCDIR  :=src
OBJDIR  :=obj
BINDIR  :=bin
DEPDIR  :=.dep
DOCDIR  :=doc

EXPORTS   :=jsdi.exp
TARGET    :=jsdi.dll
SOURCES   :=$(wildcard $(SRCDIR)/*.cpp)
OBJECTS   :=$(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPENDS   :=$(SOURCES:$(SRCDIR)/%.cpp=$(DEPDIR)/%.d)
DOXYFILE  :=$(DOCDIR)/Doxyfile

# NEEDED to prepend '_' to __stdcall function names when built with MinGW.
# http://stackoverflow.com/q/17199904/1911388
DLLTOOL:=dlltool --add-stdcall-underscore

CC:=g++
OPTLEVEL:=0
DEBUGOPTION:=-g
STDFLAGS:=-std=gnu++0x -D__GXX_EXPERIMENTAL_CXX0X__
INCLUDE_DIRS:="-I$(JAVA_HOME)/include" "-I$(JAVA_HOME)/include/win32"
CCFLAGS:=-O$(OPTLEVEL) $(DEBUGOPTION) $(STDFLAGS) -Wall -fmessage-length=0 \
         $(INCLUDE_DIRS)

LD:=g++ $(BINDIR)/$(EXPORTS) -o
LDFLAGS:=-shared -s

all: dirs $(BINDIR)/$(TARGET)

# Including DEPENDS will trigger dependency generation if the dependency files
# don't yet exist. You have to include it after the default target, though, or
# make will assume that the first dependency file *is* the default target! The
# dependency system in this makefile is copied from 
# http://stackoverflow.com/a/2045668/1911388, which is a pretty good post on
# how to set up dependencies w the gcc toolchain.
include $(DEPENDS)

$(BINDIR)/$(TARGET): $(BINDIR)/$(EXPORTS)
	@$(LD) $@ $(LDFLAGS) $(OBJECTS)

$(BINDIR)/$(EXPORTS): $(OBJECTS)
	@$(DLLTOOL) -e $@ -D $(TARGET) $(OBJECTS)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(CC) $(CCFLAGS) -c $< -o $@

$(DEPENDS): $(DEPDIR)/%.d : $(SRCDIR)/%.cpp
	@$(CC) $(CCFLAGS) -MF"$@" -MG -MM -MP \
                      -MT"$@" -MT"$(<:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)" "$<"

.PHONY: dirs
dirs: $(OBJDIR) $(BINDIR) $(DEPDIR)

$(OBJDIR):
	@mkdir $(OBJDIR)

$(BINDIR):
	@mkdir $(BINDIR)

$(DEPDIR):
	@mkdir $(DEPDIR)

.PHONY: clean
clean:
	@rm -f $(OBJDIR)/* $(BINDIR)/* $(DEPDIR)/*

docs:
	@doxygen $(DOXYFILE) >/dev/null
