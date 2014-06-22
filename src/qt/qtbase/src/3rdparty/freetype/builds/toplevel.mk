#
# FreeType build system -- top-level sub-Makefile
#


# Copyright 1996-2000, 2001, 2003, 2006, 2008, 2009 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# This file is designed for GNU Make, do not use it with another Make tool!
#
# It works as follows:
#
# - When invoked for the first time, this Makefile includes the rules found
#   in `PROJECT/builds/detect.mk'.  They are in charge of detecting the
#   current platform.
#
#   A summary of the detection is displayed, and the file `config.mk' is
#   created in the current directory.
#
# - When invoked later, this Makefile includes the rules found in
#   `config.mk'.  This sub-Makefile defines some system-specific variables
#   (like compiler, compilation flags, object suffix, etc.), then includes
#   the rules found in `PROJECT/builds/PROJECT.mk', used to build the
#   library.
#
# See the comments in `builds/detect.mk' and `builds/PROJECT.mk' for more
# details on host platform detection and library builds.


# First of all, check whether we have `$(value ...)'.  We do this by testing
# for `$(eval ...)' which has been introduced in the same GNU make version.

eval_available :=
$(eval eval_available := T)
ifneq ($(eval_available),T)
  $(error FreeType's build system needs a Make program which supports $$(value))
endif


.PHONY: all dist distclean modules setup


# The `space' variable is used to avoid trailing spaces in defining the
# `T' variable later.
#
empty :=
space := $(empty) $(empty)


# The main configuration file, defining the `XXX_MODULES' variables.  We
# prefer a `modules.cfg' file in OBJ_DIR over TOP_DIR.
#
ifndef MODULES_CFG
  MODULES_CFG := $(TOP_DIR)/modules.cfg
  ifneq ($(wildcard $(OBJ_DIR)/modules.cfg),)
    MODULES_CFG := $(OBJ_DIR)/modules.cfg
  endif
endif


# FTMODULE_H, as its name suggests, indicates where the FreeType module
# class file resides.
#
FTMODULE_H ?= $(OBJ_DIR)/ftmodule.h


include $(MODULES_CFG)


# The list of modules we are using.
#
MODULES := $(FONT_MODULES)    \
           $(HINTING_MODULES) \
           $(RASTER_MODULES)  \
           $(AUX_MODULES)


CONFIG_MK ?= config.mk

# If no configuration sub-makefile is present, or if `setup' is the target
# to be built, run the auto-detection rules to figure out which
# configuration rules file to use.
#
# Note that the configuration file is put in the current directory, which is
# not necessarily $(TOP_DIR).

# If `config.mk' is not present, set `check_platform'.
#
ifeq ($(wildcard $(CONFIG_MK)),)
  check_platform := 1
endif

# If `setup' is one of the targets requested, set `check_platform'.
#
ifneq ($(findstring setup,$(MAKECMDGOALS)),)
  check_platform := 1
endif

# Include the automatic host platform detection rules when we need to
# check the platform.
#
ifdef check_platform

  all modules: setup

  include $(TOP_DIR)/builds/detect.mk

  # This rule makes sense for Unix only to remove files created by a run
  # of the configure script which hasn't been successful (so that no
  # `config.mk' has been created).  It uses the built-in $(RM) command of
  # GNU make.  Similarly, `nul' is created if e.g. `make setup win32' has
  # been erroneously used.
  #
  # Note: This test is duplicated in `builds/unix/detect.mk'.
  #
  is_unix := $(strip $(wildcard /sbin/init) \
                     $(wildcard /usr/sbin/init) \
                     $(wildcard /hurd/auth))
  ifneq ($(is_unix),)

    distclean:
	  $(RM) builds/unix/config.cache
	  $(RM) builds/unix/config.log
	  $(RM) builds/unix/config.status
	  $(RM) builds/unix/unix-def.mk
	  $(RM) builds/unix/unix-cc.mk
	  $(RM) builds/unix/freetype2.pc
	  $(RM) nul

  endif # test is_unix

  # IMPORTANT:
  #
  # `setup' must be defined by the host platform detection rules to create
  # the `config.mk' file in the current directory.

else

  # A configuration sub-Makefile is present -- simply run it.
  #
  all: single

  BUILD_PROJECT := yes
  include $(CONFIG_MK)

endif # test check_platform


# We always need the list of modules in ftmodule.h.
#
all setup: $(FTMODULE_H)


# The `modules' target unconditionally rebuilds the module list.
#
modules:
	$(FTMODULE_H_INIT)
	$(FTMODULE_H_CREATE)
	$(FTMODULE_H_DONE)

include $(TOP_DIR)/builds/modules.mk


# This target builds the tarballs.
#
# Not to be run by a normal user -- there are no attempts to make it
# generic.

# we check for `dist', not `distclean'
ifneq ($(findstring distx,$(MAKECMDGOALS)x),)
  FT_H := include/freetype/freetype.h

  major := $(shell sed -n 's/.*FREETYPE_MAJOR[^0-9]*\([0-9]\+\)/\1/p' < $(FT_H))
  minor := $(shell sed -n 's/.*FREETYPE_MINOR[^0-9]*\([0-9]\+\)/\1/p' < $(FT_H))
  patch := $(shell sed -n 's/.*FREETYPE_PATCH[^0-9]*\([0-9]\+\)/\1/p' < $(FT_H))

  version    := $(major).$(minor).$(patch)
  winversion := $(major)$(minor)$(patch)
endif

dist:
	-rm -rf tmp
	rm -f freetype-$(version).tar.gz
	rm -f freetype-$(version).tar.bz2
	rm -f ft$(winversion).zip

	for d in `find . -wholename '*/.git' -prune \
	                 -o -type f \
	                 -o -print` ; do \
	  mkdir -p tmp/$$d ; \
	done ;

	currdir=`pwd` ; \
	for f in `find . -wholename '*/.git' -prune \
	                 -o -name .cvsignore \
	                 -o -type d \
	                 -o -print` ; do \
	  ln -s $$currdir/$$f tmp/$$f ; \
	done

	@# Prevent generation of .pyc files.  Python follows (soft) links if
	@# the link's directory is write protected, so we have temporarily
	@# disable write access here too.
	chmod -w src/tools/docmaker

	cd tmp ; \
	$(MAKE) devel ; \
	$(MAKE) do-dist

	chmod +w src/tools/docmaker

	mv tmp freetype-$(version)

	tar cfh - freetype-$(version) \
	| gzip -9 -c > freetype-$(version).tar.gz
	tar cfh - freetype-$(version) \
	| bzip2 -c > freetype-$(version).tar.bz2

	@# Use CR/LF for zip files.
	zip -lr9 ft$(winversion).zip freetype-$(version)

	rm -fr freetype-$(version)


# The locations of the latest `config.guess' and `config.sub' versions (from
# GNU `config' CVS), relative to the `tmp' directory used during `make dist'.
#
CONFIG_GUESS = ~/git/config/config.guess
CONFIG_SUB   = ~/git/config/config.sub


# Don't say `make do-dist'.  Always use `make dist' instead.
#
.PHONY: do-dist

do-dist: distclean refdoc
	@# Without removing the files, `autoconf' and friends follow links.
	rm -f builds/unix/aclocal.m4
	rm -f builds/unix/configure.ac
	rm -f builds/unix/configure

	sh autogen.sh
	rm -rf builds/unix/autom4te.cache

	cp $(CONFIG_GUESS) builds/unix
	cp $(CONFIG_SUB) builds/unix

# EOF
