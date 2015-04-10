# This file is building the plugin for clang in chromium checkout.

# This file requires the clang build system, at least for now. So to use this
# Makefile, you should execute the following commands to copy this directory
# into a clang checkout:
#
#   cp -R <this directory> chromium/src/third_party/llvm/tools/clang/tools/report-memory-usage-plugin
#   cd chromium/src/third_party/llvm/tools/clang/tools/report-memory-usage-plugin
#   make
#
# It can be run with clang build with help of these clang flags:
#
#       -Xclang -load \
#	-Xclang <plugin-libs-dir>/libReportMemoryUsage.so \
#	-Xclang -plugin \
#	-Xclang report-memory-usage

CLANG_LEVEL := ../..
LIBRARYNAME = ReportMemoryUsage

LINK_LIBS_IN_SHARED = 0
SHARED_LIBRARY = 1

include $(CLANG_LEVEL)/Makefile

ifeq ($(OS),Darwin)
  LDFLAGS=-Wl,-undefined,dynamic_lookup
endif

BD=../../../../Release+Asserts
SRC_DIR=../../../../../llvm/tools/clang/tools/report-memory-usage-plugin

test: all
	@${BD}/bin/clang++ \
	-D_GNU_SOURCE \
	-D_DEBUG \
	-D__STDC_CONSTANT_MACROS \
        -D__STDC_FORMAT_MACROS \
	-D__STDC_LIMIT_MACROS \
	-D_GNU_SOURCE \
        -I${BD}/tools/clang/include \
	-Itools/clang/include \
	-I${BD}/include \
	-Iinclude \
        ${SRC_DIR}/tests/Source/WebCore/Test.cpp \
	-fsyntax-only \
        -Xclang -load \
	-Xclang ${BD}/lib/lib${LIBRARYNAME}.so \
	-Xclang -plugin \
	-Xclang report-memory-usage

