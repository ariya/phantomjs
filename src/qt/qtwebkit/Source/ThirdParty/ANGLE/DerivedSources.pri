# -------------------------------------------------------------------
# Derived sources for ANGLE
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

# This file is both a top level target, and included from Target.pri,
# so that the resulting generated sources can be added to SOURCES.
# We only set the template if we're a top level target, so that we
# don't override what Target.pri has already set.
sanitizedFile = $$toSanitizedPath($$_FILE_)
equals(sanitizedFile, $$toSanitizedPath($$_PRO_FILE_)):TEMPLATE = derived

ANGLE_FLEX_SOURCES = \
    $$PWD/src/compiler/glslang.l \
    $$PWD/src/compiler/preprocessor/Tokenizer.l

angleflex.output = ${QMAKE_FILE_BASE}_lex.cpp
angleflex.input = ANGLE_FLEX_SOURCES
angleflex.commands = $$FLEX --noline --nounistd --outfile=${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
GENERATORS += angleflex

ANGLE_BISON_SOURCES = \
    $$PWD/src/compiler/glslang.y \
    $$PWD/src/compiler/preprocessor/ExpressionParser.y

anglebison_decl.output = ${QMAKE_FILE_BASE}_tab.h
anglebison_decl.input = ANGLE_BISON_SOURCES
anglebison_decl.commands = bison --no-lines --skeleton=yacc.c --defines=${QMAKE_FILE_OUT} --output=${QMAKE_FUNC_FILE_OUT_PATH}$${QMAKE_DIR_SEP}${QMAKE_FILE_OUT_BASE}.cpp ${QMAKE_FILE_IN}
anglebison_decl.variable_out = GENERATED_FILES
GENERATORS += anglebison_decl

anglebison_impl.input = ANGLE_BISON_SOURCES
anglebison_impl.commands = $$MAKEFILE_NOOP_COMMAND
anglebison_impl.depends = $$GENERATED_SOURCES_DESTDIR/${QMAKE_FILE_BASE}_tab.h
anglebison_impl.output = ${QMAKE_FILE_BASE}_tab.cpp
GENERATORS += anglebison_impl

