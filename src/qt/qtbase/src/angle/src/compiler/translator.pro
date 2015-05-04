CONFIG += static
include(../config.pri)

# Mingw 4.7 chokes on implicit move semantics, so disable C++11 here
mingw: CONFIG -= c++11

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

DEFINES += _SECURE_SCL=0 _LIB COMPILER_IMPLEMENTATION

FLEX_SOURCES = $$ANGLE_DIR/src/compiler/translator/glslang.l
BISON_SOURCES = $$ANGLE_DIR/src/compiler/translator/glslang.y

HEADERS += \
    $$ANGLE_DIR/include/GLSLANG/ResourceLimits.h \
    $$ANGLE_DIR/include/GLSLANG/ShaderLang.h \
    $$ANGLE_DIR/include/GLSLANG/ShaderVars.h \
    $$ANGLE_DIR/src/common/angleutils.h \
    $$ANGLE_DIR/src/common/blocklayout.h \
    $$ANGLE_DIR/src/common/debug.h \
    $$ANGLE_DIR/src/common/platform.h \
    $$ANGLE_DIR/src/common/tls.h \
    $$ANGLE_DIR/src/common/utilities.h \
    $$ANGLE_DIR/src/compiler/translator/BaseTypes.h \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulator.h \
    $$ANGLE_DIR/src/compiler/translator/Common.h \
    $$ANGLE_DIR/src/compiler/translator/Compiler.h \
    $$ANGLE_DIR/src/compiler/translator/compilerdebug.h \
    $$ANGLE_DIR/src/compiler/translator/ConstantUnion.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraph.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphBuilder.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphOutput.h \
    $$ANGLE_DIR/src/compiler/translator/DetectCallDepth.h \
    $$ANGLE_DIR/src/compiler/translator/DetectDiscontinuity.h \
    $$ANGLE_DIR/src/compiler/translator/Diagnostics.h \
    $$ANGLE_DIR/src/compiler/translator/DirectiveHandler.h \
    $$ANGLE_DIR/src/compiler/translator/ExtensionBehavior.h \
    $$ANGLE_DIR/src/compiler/translator/FlagStd140Structs.h \
    $$ANGLE_DIR/src/compiler/translator/ForLoopUnroll.h \
    $$ANGLE_DIR/src/compiler/translator/HashNames.h \
    $$ANGLE_DIR/src/compiler/translator/InfoSink.h \
    $$ANGLE_DIR/src/compiler/translator/Initialize.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeDll.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeParseContext.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeVariables.h \
    $$ANGLE_DIR/src/compiler/translator/intermediate.h \
    $$ANGLE_DIR/src/compiler/translator/IntermNode.h \
    $$ANGLE_DIR/src/compiler/translator/LoopInfo.h \
    $$ANGLE_DIR/src/compiler/translator/MMap.h \
    $$ANGLE_DIR/src/compiler/translator/NodeSearch.h \
    $$ANGLE_DIR/src/compiler/translator/osinclude.h \
    $$ANGLE_DIR/src/compiler/translator/OutputESSL.h \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSLBase.h \
    $$ANGLE_DIR/src/compiler/translator/OutputHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/ParseContext.h \
    $$ANGLE_DIR/src/compiler/translator/PoolAlloc.h \
    $$ANGLE_DIR/src/compiler/translator/Pragma.h \
    $$ANGLE_DIR/src/compiler/translator/QualifierAlive.h \
    $$ANGLE_DIR/src/compiler/translator/RegenerateStructNames.h \
    $$ANGLE_DIR/src/compiler/translator/RemoveTree.h \
    $$ANGLE_DIR/src/compiler/translator/RenameFunction.h \
    $$ANGLE_DIR/src/compiler/translator/RewriteElseBlocks.h \
    $$ANGLE_DIR/src/compiler/translator/ScalarizeVecAndMatConstructorArgs.h \
    $$ANGLE_DIR/src/compiler/translator/SearchSymbol.h \
    $$ANGLE_DIR/src/compiler/translator/ShHandle.h \
    $$ANGLE_DIR/src/compiler/translator/StructureHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/SymbolTable.h \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictFragmentShaderTiming.h \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictVertexShaderTiming.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorESSL.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/Types.h \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuit.h \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitAST.h \
    $$ANGLE_DIR/src/compiler/translator/UniformHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/UtilsHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/util.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateLimitations.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateOutputs.h \
    $$ANGLE_DIR/src/compiler/translator/VariableInfo.h \
    $$ANGLE_DIR/src/compiler/translator/VariablePacker.h \
    $$ANGLE_DIR/src/compiler/translator/VersionGLSL.h \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.h


SOURCES += \
    $$ANGLE_DIR/src/common/tls.cpp \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulator.cpp \
    $$ANGLE_DIR/src/compiler/translator/CodeGen.cpp \
    $$ANGLE_DIR/src/compiler/translator/Compiler.cpp \
    $$ANGLE_DIR/src/compiler/translator/compilerdebug.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraph.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphBuilder.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphOutput.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphTraverse.cpp \
    $$ANGLE_DIR/src/compiler/translator/DetectCallDepth.cpp \
    $$ANGLE_DIR/src/compiler/translator/DetectDiscontinuity.cpp \
    $$ANGLE_DIR/src/compiler/translator/Diagnostics.cpp \
    $$ANGLE_DIR/src/compiler/translator/DirectiveHandler.cpp \
    $$ANGLE_DIR/src/compiler/translator/FlagStd140Structs.cpp \
    $$ANGLE_DIR/src/compiler/translator/ForLoopUnroll.cpp \
    $$ANGLE_DIR/src/compiler/translator/InfoSink.cpp \
    $$ANGLE_DIR/src/compiler/translator/Initialize.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeDll.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeParseContext.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeVariables.cpp \
    $$ANGLE_DIR/src/compiler/translator/Intermediate.cpp \
    $$ANGLE_DIR/src/compiler/translator/IntermNode.cpp \
    $$ANGLE_DIR/src/compiler/translator/intermOut.cpp \
    $$ANGLE_DIR/src/compiler/translator/IntermTraverse.cpp \
    $$ANGLE_DIR/src/compiler/translator/LoopInfo.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputESSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSLBase.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/parseConst.cpp \
    $$ANGLE_DIR/src/compiler/translator/ParseContext.cpp \
    $$ANGLE_DIR/src/compiler/translator/PoolAlloc.cpp \
    $$ANGLE_DIR/src/compiler/translator/QualifierAlive.cpp \
    $$ANGLE_DIR/src/compiler/translator/RegenerateStructNames.cpp \
    $$ANGLE_DIR/src/compiler/translator/RemoveTree.cpp \
    $$ANGLE_DIR/src/compiler/translator/RewriteElseBlocks.cpp \
    $$ANGLE_DIR/src/compiler/translator/ScalarizeVecAndMatConstructorArgs.cpp \
    $$ANGLE_DIR/src/compiler/translator/SearchSymbol.cpp \
    $$ANGLE_DIR/src/compiler/translator/ShaderLang.cpp \
    $$ANGLE_DIR/src/compiler/translator/ShaderVars.cpp \
    $$ANGLE_DIR/src/compiler/translator/StructureHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/SymbolTable.cpp \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictFragmentShaderTiming.cpp \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictVertexShaderTiming.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorESSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/Types.cpp \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuit.cpp \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitAST.cpp \
    $$ANGLE_DIR/src/compiler/translator/UniformHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/UtilsHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/util.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateLimitations.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateOutputs.cpp \
    $$ANGLE_DIR/src/compiler/translator/VariableInfo.cpp \
    $$ANGLE_DIR/src/compiler/translator/VariablePacker.cpp \
    $$ANGLE_DIR/src/compiler/translator/VersionGLSL.cpp \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.cpp


# NOTE: 'win_flex' and 'bison' can be found in qt5/gnuwin32/bin
flex.commands = $$addGnuPath(win_flex) --noline --nounistd --outfile=${QMAKE_FILE_BASE}_lex.cpp ${QMAKE_FILE_NAME}
flex.output = ${QMAKE_FILE_BASE}_lex.cpp
flex.input = FLEX_SOURCES
flex.dependency_type = TYPE_C
flex.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += flex

bison.commands = $$addGnuPath(bison) --no-lines --skeleton=yacc.c --defines=${QMAKE_FILE_BASE}_tab.h \
                --output=${QMAKE_FILE_BASE}_tab.cpp ${QMAKE_FILE_NAME}
bison.output = ${QMAKE_FILE_BASE}_tab.h
bison.input = BISON_SOURCES
bison.dependency_type = TYPE_C
bison.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison

# This is a dummy compiler to work around the fact that an extra compiler can only
# have one output file even if the command generates two.
MAKEFILE_NOOP_COMMAND = @echo -n
msvc: MAKEFILE_NOOP_COMMAND = @echo >NUL
bison_impl.output = ${QMAKE_FILE_BASE}_tab.cpp
bison_impl.input = BISON_SOURCES
bison_impl.commands = $$MAKEFILE_NOOP_COMMAND
bison_impl.depends = ${QMAKE_FILE_BASE}_tab.h
bison_impl.output = ${QMAKE_FILE_BASE}_tab.cpp
bison_impl.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison_impl
