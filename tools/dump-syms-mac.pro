INCLUDEPATH += ../src/breakpad/src

OBJECTIVE_SOURCES += ../src/breakpad/src/tools/mac/dump_syms/dump_syms_tool.mm \
  ../src/breakpad/src/common/mac/dump_syms.mm

SOURCES += ../src/breakpad/src/common/module.cc \
  ../src/breakpad/src/common/dwarf/dwarf2diehandler.cc \
  ../src/breakpad/src/common/dwarf/bytereader.cc \
  ../src/breakpad/src/common/stabs_to_module.cc \
  ../src/breakpad/src/common/mac/stabs_reader.cc \
  ../src/breakpad/src/common/dwarf_cu_to_module.cc \
  ../src/breakpad/src/common/dwarf_cfi_to_module.cc \
  ../src/breakpad/src/common/dwarf_line_to_module.cc \
  ../src/breakpad/src/common/language.cc \
  ../src/breakpad/src/common/md5.cc \
  ../src/breakpad/src/common/mac/macho_reader.cc \
  ../src/breakpad/src/common/mac/file_id.cc \
  ../src/breakpad/src/common/mac/macho_id.cc \
  ../src/breakpad/src/common/mac/macho_utilities.cc \
  ../src/breakpad/src/common/mac/macho_walker.cc \
  ../src/breakpad/src/common/dwarf/dwarf2reader.cc

TARGET = dump_syms
