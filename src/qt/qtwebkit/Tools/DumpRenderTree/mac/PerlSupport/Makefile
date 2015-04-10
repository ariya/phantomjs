# Copyright (C) 2009 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

CONFIGURATION_BUILD_DIR ?= .
OUTPUT_DIR=$(CONFIGURATION_BUILD_DIR)

WRAPPER_DIR=$(OUTPUT_DIR)/DerivedSources/DumpRenderTree
WRAPPER=$(WRAPPER_DIR)/DumpRenderTreeSupport_wrap.c
PERL_MODULE=$(OUTPUT_DIR)/DumpRenderTreeSupport.pm
DYLIB=$(OUTPUT_DIR)/DumpRenderTreeSupport.dylib
DUMPRENDERTREE=$(OUTPUT_DIR)/DumpRenderTree
PERL=/usr/bin/perl

OSX_VERSION = $(shell sw_vers -productVersion | cut -d. -f 2)
ifeq "$(OSX_VERSION)" "5"
GENERATE_WRAPPER = YES
endif
ifeq "$(OSX_VERSION)" "6"
GENERATE_WRAPPER = NO
endif

ifeq "$(GENERATE_WRAPPER)" "YES"

SWIG=/usr/bin/swig

all: $(DYLIB) $(PERL_MODULE)

$(WRAPPER) $(PERL_MODULE): DumpRenderTreeSupport.c $(DUMPRENDERTREE)
	mkdir -p $(WRAPPER_DIR)
	$(SWIG) -o $(WRAPPER) -outdir $(OUTPUT_DIR) -perl -module DumpRenderTreeSupport $<


else


all: $(DYLIB) $(PERL_MODULE)

$(WRAPPER): DumpRenderTreeSupport_wrapPregenerated.c $(DUMPRENDERTREE)
	mkdir -p $(WRAPPER_DIR)
	cp DumpRenderTreeSupport_wrapPregenerated.c $(WRAPPER)

$(PERL_MODULE): DumpRenderTreeSupportPregenerated.pm $(DUMPRENDERTREE)
	cp DumpRenderTreeSupportPregenerated.pm $(PERL_MODULE)


endif

$(DYLIB): DumpRenderTreeSupport.c $(WRAPPER)
	gcc -g -dynamiclib -o $(DYLIB) `$(PERL) -MExtUtils::Embed -eperl_inc` `$(PERL) -MExtUtils::Embed -e'my $$opts = ldopts(0); $$opts =~ s/-arch [^ ]*( |$$)//g; print $$opts, " -arch ", join(" -arch ", split(" ",$$ENV{ARCHS}))'` $^

clean:
	rm -f $(WRAPPER) $(PERL_MODULE) $(DYLIB)

installhdrs installsrc:

INSTALL_LOCATION=$(DSTROOT)/$(SYSTEM_LIBRARY_DIR)/Frameworks/$(WEBKIT_FRAMEWORK_RESOURCES_PATH)

install: all
	mkdir -p $(INSTALL_LOCATION)
	cp $(DYLIB) $(INSTALL_LOCATION)/DumpRenderTreeSupport.dylib
	cp $(PERL_MODULE) $(INSTALL_LOCATION)/DumpRenderTreeSupport.pm

