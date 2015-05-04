CONFIG += static
include(../../config.pri)

INCLUDEPATH = $$ANGLE_DIR/src/compiler/preprocessor

DEFINES += _SECURE_SCL=0


FLEX_SOURCES =  \
    $$ANGLE_DIR/src/compiler/preprocessor/Tokenizer.l

BISON_SOURCES = \
    $$ANGLE_DIR/src/compiler/preprocessor/ExpressionParser.y

HEADERS += \
    $$ANGLE_DIR/src/compiler/preprocessor/DiagnosticsBase.h \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveHandlerBase.h \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveParser.h \
    $$ANGLE_DIR/src/compiler/preprocessor/ExpressionParser.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Input.h \
    $$ANGLE_DIR/src/compiler/preprocessor/length_limits.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Lexer.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Macro.h \
    $$ANGLE_DIR/src/compiler/preprocessor/MacroExpander.h \
    $$ANGLE_DIR/src/compiler/preprocessor/numeric_lex.h \
    $$ANGLE_DIR/src/compiler/preprocessor/pp_utils.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Preprocessor.h \
    $$ANGLE_DIR/src/compiler/preprocessor/SourceLocation.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Token.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Tokenizer.h

SOURCES += \
    $$ANGLE_DIR/src/compiler/preprocessor/DiagnosticsBase.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveHandlerBase.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveParser.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Input.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Lexer.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Macro.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/MacroExpander.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Preprocessor.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Token.cpp

# NOTE: 'win_flex' and 'bison' can be found in qt5/gnuwin32/bin
flex.commands = $$addGnuPath(win_flex) --noline --nounistd --outfile=${QMAKE_FILE_BASE}.cpp ${QMAKE_FILE_NAME}
flex.output = ${QMAKE_FILE_BASE}.cpp
flex.input = FLEX_SOURCES
flex.dependency_type = TYPE_C
flex.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += flex

bison.commands = $$addGnuPath(bison) --no-lines --skeleton=yacc.c  --output=${QMAKE_FILE_BASE}.cpp ${QMAKE_FILE_NAME}
bison.output = ${QMAKE_FILE_BASE}.cpp
bison.input = BISON_SOURCES
bison.dependency_type = TYPE_C
bison.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison
