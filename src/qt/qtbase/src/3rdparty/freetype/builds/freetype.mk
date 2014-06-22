#
# FreeType 2 library sub-Makefile
#


# Copyright 1996-2000, 2001, 2002, 2003, 2004, 2005, 2006, 2008 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# DO NOT INVOKE THIS MAKEFILE DIRECTLY!  IT IS MEANT TO BE INCLUDED BY
# OTHER MAKEFILES.


# The following variables (set by other Makefile components, in the
# environment, or on the command line) are used:
#
#   BUILD_DIR      The architecture dependent directory,
#                  e.g. `$(TOP_DIR)/builds/unix'.  Added to INCLUDES also.
#
#   OBJ_DIR        The directory in which object files are created.
#
#   LIB_DIR        The directory in which the library is created.
#
#   DOC_DIR        The directory in which the API reference is created.
#
#   INCLUDES       A list of directories to be included additionally.
#
#   DEVEL_DIR      Development directory which is added to the INCLUDES
#                  variable before the standard include directories.
#
#   CFLAGS         Compilation flags.  This overrides the default settings
#                  in the platform-specific configuration files.
#
#   FTSYS_SRC      If set, its value is used as the name of a replacement
#                  file for `src/base/ftsystem.c'.
#
#   FTDEBUG_SRC    If set, its value is used as the name of a replacement
#                  file for `src/base/ftdebug.c'.  [For a normal build, this
#                  file does nothing.]
#
#   FTMODULE_H     The file which contains the list of module classes for
#                  the current build.  Usually, this is automatically
#                  created by `modules.mk'.
#
#   BASE_OBJ_S
#   BASE_OBJ_M     A list of base objects (for single object and multiple
#                  object builds, respectively).  Set up in
#                  `src/base/rules.mk'.
#
#   BASE_EXT_OBJ   A list of base extension objects.  Set up in
#                  `src/base/rules.mk'.
#
#   DRV_OBJ_S
#   DRV_OBJ_M      A list of driver objects (for single object and multiple
#                  object builds, respectively).  Set up cumulatively in
#                  `src/<driver>/rules.mk'.
#
#   CLEAN
#   DISTCLEAN      The sub-makefiles can append additional stuff to these two
#                  variables which is to be removed for the `clean' resp.
#                  `distclean' target.
#
#   TOP_DIR, SEP,
#   COMPILER_SEP,
#   LIBRARY, CC,
#   A, I, O, T     Check `config.mk' for details.


# The targets `objects' and `library' are defined at the end of this
# Makefile after all other rules have been included.
#
.PHONY: single multi objects library refdoc

# default target -- build single objects and library
#
single: objects library

# `multi' target -- build multiple objects and library
#
multi: objects library


# The FreeType source directory, usually `./src'.
#
SRC_DIR := $(TOP_DIR)/src

# The directory where the base layer components are placed, usually
# `./src/base'.
#
BASE_DIR := $(SRC_DIR)/base

# Other derived directories.
#
PUBLIC_DIR   := $(TOP_DIR)/include/freetype
INTERNAL_DIR := $(PUBLIC_DIR)/internal
SERVICES_DIR := $(INTERNAL_DIR)/services
CONFIG_DIR   := $(PUBLIC_DIR)/config

# The documentation directory.
#
DOC_DIR ?= $(TOP_DIR)/docs/reference

# The final name of the library file.
#
PROJECT_LIBRARY := $(LIB_DIR)/$(LIBRARY).$A


# include paths
#
# IMPORTANT NOTE: The architecture-dependent directory must ALWAYS be placed
#                 before the standard include list.  Porters are then able to
#                 put their own version of some of the FreeType components
#                 in the `freetype/builds/<system>' directory, as these
#                 files will override the default sources.
#
INCLUDES := $(subst /,$(COMPILER_SEP),$(OBJ_DIR) \
                                      $(DEVEL_DIR) \
                                      $(BUILD_DIR) \
                                      $(TOP_DIR)/include)

INCLUDE_FLAGS := $(INCLUDES:%=$I%)


# C flags used for the compilation of an object file.  This must include at
# least the paths for the `base' and `builds/<system>' directories;
# debug/optimization/warning flags + ansi compliance if needed.
#
# $(INCLUDE_FLAGS) should come before $(CFLAGS) to avoid problems with
# old FreeType versions.
#
# Note what we also define the macro FT2_BUILD_LIBRARY when building
# FreeType.  This is required to let our sources include the internal
# headers (something forbidden by clients).
#
# Finally, we define FT_CONFIG_MODULES_H so that the compiler uses the
# generated version of `ftmodule.h' in $(OBJ_DIR).  If there is an
# `ftoption.h' files in $(OBJ_DIR), define FT_CONFIG_OPTIONS_H too.
#
ifneq ($(wildcard $(OBJ_DIR)/ftoption.h),)
  FTOPTION_H    := $(OBJ_DIR)/ftoption.h
  FTOPTION_FLAG := $DFT_CONFIG_OPTIONS_H="<ftoption.h>"
endif

FT_CFLAGS  = $(CPPFLAGS) \
             $(INCLUDE_FLAGS) \
             $(CFLAGS) \
             $DFT2_BUILD_LIBRARY \
             $DFT_CONFIG_MODULES_H="<ftmodule.h>" \
             $(FTOPTION_FLAG)
FT_CC      = $(CC) $(FT_CFLAGS)
FT_COMPILE = $(CC) $(ANSIFLAGS) $(FT_CFLAGS)


# Include the `exports' rules file.
#
include $(TOP_DIR)/builds/exports.mk


# Initialize the list of objects.
#
OBJECTS_LIST :=


# Define $(PUBLIC_H) as the list of all public header files located in
# `$(TOP_DIR)/include/freetype'.  $(INTERNAL_H), and $(CONFIG_H) are defined
# similarly.
#
# This is used to simplify the dependency rules -- if one of these files
# changes, the whole library is recompiled.
#
PUBLIC_H   := $(wildcard $(PUBLIC_DIR)/*.h)
INTERNAL_H := $(wildcard $(INTERNAL_DIR)/*.h) \
              $(wildcard $(SERVICES_DIR)/*.h)
CONFIG_H   := $(wildcard $(CONFIG_DIR)/*.h) \
              $(wildcard $(BUILD_DIR)/freetype/config/*.h) \
              $(FTMODULE_H) \
              $(FTOPTION_H)
DEVEL_H    := $(wildcard $(TOP_DIR)/devel/*.h)

FREETYPE_H := $(PUBLIC_H) $(INTERNAL_H) $(CONFIG_H) $(DEVEL_H)


# ftsystem component
#
FTSYS_SRC ?= $(BASE_DIR)/ftsystem.c

FTSYS_OBJ := $(OBJ_DIR)/ftsystem.$O

OBJECTS_LIST += $(FTSYS_OBJ)

$(FTSYS_OBJ): $(FTSYS_SRC) $(FREETYPE_H)
	$(FT_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


# ftdebug component
#
FTDEBUG_SRC ?= $(BASE_DIR)/ftdebug.c

FTDEBUG_OBJ := $(OBJ_DIR)/ftdebug.$O

OBJECTS_LIST += $(FTDEBUG_OBJ)

$(FTDEBUG_OBJ): $(FTDEBUG_SRC) $(FREETYPE_H)
	$(FT_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


# Include all rule files from FreeType components.
#
include $(SRC_DIR)/base/rules.mk
include $(patsubst %,$(SRC_DIR)/%/rules.mk,$(MODULES))


# ftinit component
#
#   The C source `ftinit.c' contains the FreeType initialization routines.
#   It is able to automatically register one or more drivers when the API
#   function FT_Init_FreeType() is called.
#
#   The set of initial drivers is determined by the driver Makefiles
#   includes above.  Each driver Makefile updates the FTINIT_xxx lists
#   which contain additional include paths and macros used to compile the
#   single `ftinit.c' source.
#
FTINIT_SRC := $(BASE_DIR)/ftinit.c
FTINIT_OBJ := $(OBJ_DIR)/ftinit.$O

OBJECTS_LIST += $(FTINIT_OBJ)

$(FTINIT_OBJ): $(FTINIT_SRC) $(FREETYPE_H)
	$(FT_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


# All FreeType library objects.
#
OBJ_M := $(BASE_OBJ_M) $(BASE_EXT_OBJ) $(DRV_OBJS_M)
OBJ_S := $(BASE_OBJ_S) $(BASE_EXT_OBJ) $(DRV_OBJS_S)


# The target `multi' on the Make command line indicates that we want to
# compile each source file independently.
#
# Otherwise, each module/driver is compiled in a single object file through
# source file inclusion (see `src/base/ftbase.c' or
# `src/truetype/truetype.c' for examples).
#
BASE_OBJECTS := $(OBJECTS_LIST)

ifneq ($(findstring multi,$(MAKECMDGOALS)),)
  OBJECTS_LIST += $(OBJ_M)
else
  OBJECTS_LIST += $(OBJ_S)
endif

objects: $(OBJECTS_LIST)

library: $(PROJECT_LIBRARY)

dll: $(PROJECT_LIBRARY) exported_symbols

.c.$O:
	$(FT_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


ifneq ($(findstring refdoc,$(MAKECMDGOALS)),)
  # poor man's `sed' emulation with make's built-in string functions
  work := $(strip $(shell $(CAT) $(PUBLIC_DIR)/freetype.h))
  work := $(subst |,x,$(work))
  work := $(subst $(space),|,$(work))
  work := $(subst \#define|FREETYPE_MAJOR|,$(space),$(work))
  work := $(word 2,$(work))
  major := $(subst |,$(space),$(work))
  major := $(firstword $(major))

  work := $(subst \#define|FREETYPE_MINOR|,$(space),$(work))
  work := $(word 2,$(work))
  minor := $(subst |,$(space),$(work))
  minor := $(firstword $(minor))

  work := $(subst \#define|FREETYPE_PATCH|,$(space),$(work))
  work := $(word 2,$(work))
  patch := $(subst |,$(space),$(work))
  patch := $(firstword $(patch))

  version := $(major).$(minor).$(patch)
endif

# We write-protect the docmaker directory to suppress generation
# of .pyc files.
#
refdoc:
	-chmod -w $(SRC_DIR)/tools/docmaker
	python $(SRC_DIR)/tools/docmaker/docmaker.py \
               --prefix=ft2                          \
               --title=FreeType-$(version)           \
               --output=$(DOC_DIR)                   \
               $(PUBLIC_DIR)/*.h                     \
               $(PUBLIC_DIR)/config/*.h              \
               $(PUBLIC_DIR)/cache/*.h
	-chmod +w $(SRC_DIR)/tools/docmaker


.PHONY: clean_project_std distclean_project_std

# Standard cleaning and distclean rules.  These are not accepted
# on all systems though.
#
clean_project_std:
	-$(DELETE) $(BASE_OBJECTS) $(OBJ_M) $(OBJ_S) $(CLEAN)

distclean_project_std: clean_project_std
	-$(DELETE) $(PROJECT_LIBRARY)
	-$(DELETE) *.orig *~ core *.core $(DISTCLEAN)


.PHONY: clean_project_dos distclean_project_dos

# The Dos command shell does not support very long list of arguments, so
# we are stuck with wildcards.
#
# Don't break the command lines with \; this prevents the "del" command from
# working correctly on Win9x.
#
clean_project_dos:
	-$(DELETE) $(subst /,$(SEP),$(OBJ_DIR)/*.$O $(CLEAN) $(NO_OUTPUT))

distclean_project_dos: clean_project_dos
	-$(DELETE) $(subst /,$(SEP),$(PROJECT_LIBRARY) $(DISTCLEAN) $(NO_OUTPUT))


.PHONY: remove_config_mk remove_ftmodule_h

# Remove configuration file (used for distclean).
#
remove_config_mk:
	-$(DELETE) $(subst /,$(SEP),$(CONFIG_MK) $(NO_OUTPUT))

# Remove module list (used for distclean).
#
remove_ftmodule_h:
	-$(DELETE) $(subst /,$(SEP),$(FTMODULE_H) $(NO_OUTPUT))


.PHONY: clean distclean

# The `config.mk' file must define `clean_freetype' and
# `distclean_freetype'.  Implementations may use to relay these to either
# the `std' or `dos' versions from above, or simply provide their own
# implementation.
#
clean: clean_project
distclean: distclean_project remove_config_mk remove_ftmodule_h
	-$(DELETE) $(subst /,$(SEP),$(DOC_DIR)/*.html $(NO_OUTPUT))


# EOF
