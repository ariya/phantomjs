#
# FreeType 2 host platform detection rules
#


# Copyright 1996-2000, 2001, 2002, 2003, 2006, 2008 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# This sub-Makefile is in charge of detecting the current platform.  It sets
# the following variables:
#
#   BUILD_DIR    The configuration and system-specific directory.  Usually
#                `freetype/builds/$(PLATFORM)' but can be different for
#                custom builds of the library.
#
# The following variables must be defined in system specific `detect.mk'
# files:
#
#   PLATFORM     The detected platform.  This will default to `ansi' if
#                auto-detection fails.
#   CONFIG_FILE  The configuration sub-makefile to use.  This usually depends
#                on the compiler defined in the `CC' environment variable.
#   DELETE       The shell command used to remove a given file.
#   COPY         The shell command used to copy one file.
#   SEP          The platform-specific directory separator.
#   COMPILER_SEP The separator used in arguments of the compilation tools.
#   CC           The compiler to use.
#
# You need to set the following variable(s) before calling it:
#
#   TOP_DIR      The top-most directory in the FreeType library source
#                hierarchy.  If not defined, it will default to `.'.

# Set auto-detection default to `ansi' resp. UNIX-like operating systems.
#
PLATFORM     := ansi
DELETE       := $(RM)
COPY         := cp
CAT          := cat
SEP          := /

BUILD_CONFIG := $(TOP_DIR)/builds

# These two assignments must be delayed.
BUILD_DIR    = $(BUILD_CONFIG)/$(PLATFORM)
CONFIG_RULES = $(BUILD_DIR)/$(CONFIG_FILE)

# We define the BACKSLASH variable to hold a single back-slash character.
# This is needed because a line like
#
#   SEP := \
#
# does not work with GNU Make (the backslash is interpreted as a line
# continuation).  While a line like
#
#   SEP := \\
#
# really defines $(SEP) as `\' on Unix, and `\\' on Dos and Windows!
#
BACKSLASH := $(strip \ )

# Find all auto-detectable platforms.
#
PLATFORMS := $(notdir $(subst /detect.mk,,$(wildcard $(BUILD_CONFIG)/*/detect.mk)))
.PHONY: $(PLATFORMS) ansi

# Filter out platform specified as setup target.
#
PLATFORM := $(firstword $(filter $(MAKECMDGOALS),$(PLATFORMS)))

# If no setup target platform was specified, enable auto-detection/
# default platform.
#
ifeq ($(PLATFORM),)
  PLATFORM := ansi
endif

# If the user has explicitly asked for `ansi' on the command line,
# disable auto-detection.
#
ifeq ($(findstring ansi,$(MAKECMDGOALS)),)
  # Now, include all detection rule files found in the `builds/<system>'
  # directories.  Note that the calling order of the various `detect.mk'
  # files isn't predictable.
  #
  include $(wildcard $(BUILD_CONFIG)/*/detect.mk)
endif

# In case no detection rule file was successful, use the default.
#
ifndef CONFIG_FILE
  CONFIG_FILE := ansi.mk
  setup: std_setup
  .PHONY: setup
endif

# The following targets are equivalent, with the exception that they use
# a slightly different syntax for the `echo' command.
#
# std_setup: defined for most (i.e. Unix-like) platforms
# dos_setup: defined for Dos-ish platforms like Dos, Windows & OS/2
#
.PHONY: std_setup dos_setup

std_setup:
	@echo ""
	@echo "$(PROJECT_TITLE) build system -- automatic system detection"
	@echo ""
	@echo "The following settings are used:"
	@echo ""
	@echo "  platform                    $(PLATFORM)"
	@echo "  compiler                    $(CC)"
	@echo "  configuration directory     $(BUILD_DIR)"
	@echo "  configuration rules         $(CONFIG_RULES)"
	@echo ""
	@echo "If this does not correspond to your system or settings please remove the file"
	@echo "\`$(CONFIG_MK)' from this directory then read the INSTALL file for help."
	@echo ""
	@echo "Otherwise, simply type \`$(MAKE)' again to build the library,"
	@echo "or \`$(MAKE) refdoc' to build the API reference (the latter needs python)."
	@echo ""
	@$(COPY) $(CONFIG_RULES) $(CONFIG_MK)


# Special case for Dos, Windows, OS/2, where echo "" doesn't work correctly!
#
dos_setup:
	@type builds$(SEP)newline
	@echo $(PROJECT_TITLE) build system -- automatic system detection
	@type builds$(SEP)newline
	@echo The following settings are used:
	@type builds$(SEP)newline
	@echo   platformÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ$(PLATFORM)
	@echo   compilerÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ$(CC)
	@echo   configuration directoryÿÿÿÿÿÿ$(subst /,$(SEP),$(BUILD_DIR))
	@echo   configuration rulesÿÿÿÿÿÿÿÿÿÿ$(subst /,$(SEP),$(CONFIG_RULES))
	@type builds$(SEP)newline
	@echo If this does not correspond to your system or settings please remove the file
	@echo '$(CONFIG_MK)' from this directory then read the INSTALL file for help.
	@type builds$(SEP)newline
	@echo Otherwise, simply type 'make' again to build the library.
	@echo or 'make refdoc' to build the API reference (the latter needs python).
	@type builds$(SEP)newline
	@$(COPY) $(subst /,$(SEP),$(CONFIG_RULES) $(CONFIG_MK)) > nul


# EOF
