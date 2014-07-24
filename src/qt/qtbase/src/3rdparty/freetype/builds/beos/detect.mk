#
# FreeType 2 configuration file to detect an BeOS host platform.
#


# Copyright 1996-2000, 2003, 2006 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


.PHONY: setup


ifeq ($(PLATFORM),ansi)

  ifdef BE_HOST_CPU

    PLATFORM := beos

  endif # test MACHTYPE beos
endif

ifeq ($(PLATFORM),beos)

  DELETE      := rm -f
  CAT         := cat
  SEP         := /
  BUILD_DIR   := $(TOP_DIR)/builds/beos
  CONFIG_FILE := beos.mk

  setup: std_setup

endif   # test PLATFORM beos


# EOF
