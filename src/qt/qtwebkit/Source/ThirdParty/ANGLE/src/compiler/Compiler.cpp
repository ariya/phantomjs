//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/BuiltInFunctionEmulator.h"
#include "compiler/DetectCallDepth.h"
#include "compiler/ForLoopUnroll.h"
#include "compiler/Initialize.h"
#include "compiler/InitializeParseContext.h"
#include "compiler/MapLongVariableNames.h"
#include "compiler/ParseHelper.h"
#include "compiler/RenameFunction.h"
#include "compiler/ShHandle.h"
#include "compiler/ValidateLimitations.h"
#include "compiler/VariablePacker.h"
#include "compiler/depgraph/DependencyGraph.h"
#include "compiler/depgraph/DependencyGraphOutput.h"
#include "compiler/timing/RestrictFragmentShaderTiming.h"
#include "compiler/timing/RestrictVertexShaderTiming.h"
#include "third_party/compiler/ArrayBoundsClamper.h"

bool isWebGLBasedSpec(ShShaderSpec spec)
{
     return spec == SH_WEBGL_SPEC || spec == SH_CSS_SHADERS_SPEC;
}

namespace {
class TScopedPoolAllocator {
public:
    TScopedPoolAllocator(TPoolAllocator* allocator, bool pushPop)
        : mAllocator(allocator), mPushPopAllocator(pushPop) {
        if (mPushPopAllocator) mAllocator->push();
        SetGlobalPoolAllocator(mAllocator);
    }
    ~TScopedPoolAllocator() {
        SetGlobalPoolAllocator(NULL);
        if (mPushPopAllocator) mAllocator->pop();
    }

private:
    TPoolAllocator* mAllocator;
    bool mPushPopAllocator;
};
}  // namespace

TShHandleBase::TShHandleBase() {
    allocator.push();
    SetGlobalPoolAllocator(&allocator);
}

TShHandleBase::~TShHandleBase() {
    SetGlobalPoolAllocator(NULL);
    allocator.popAll();
}

TCompiler::TCompiler(ShShaderType type, ShShaderSpec spec)
    : shaderType(type),
      shaderSpec(spec),
      maxUniformVectors(0),
      maxExpressionComplexity(0),
      maxCallStackDepth(0),
      fragmentPrecisionHigh(false),
      clampingStrategy(SH_CLAMP_WITH_CLAMP_INTRINSIC),
      builtInFunctionEmulator(type)
{
    longNameMap = LongNameMap::GetInstance();
}

TCompiler::~TCompiler()
{
    ASSERT(longNameMap);
    longNameMap->Release();
}

bool TCompiler::Init(const ShBuiltInResources& resources)
{
    maxUniformVectors = (shaderType == SH_VERTEX_SHADER) ?
        resources.MaxVertexUniformVectors :
        resources.MaxFragmentUniformVectors;
    maxExpressionComplexity = resources.MaxExpressionComplexity;
    maxCallStackDepth = resources.MaxCallStackDepth;
    TScopedPoolAllocator scopedAlloc(&allocator, false);

    // Generate built-in symbol table.
    if (!InitBuiltInSymbolTable(resources))
        return false;
    InitExtensionBehavior(resources, extensionBehavior);
    fragmentPrecisionHigh = resources.FragmentPrecisionHigh == 1;

    arrayBoundsClamper.SetClampingStrategy(resources.ArrayIndexClampingStrategy);
    clampingStrategy = resources.ArrayIndexClampingStrategy;

    hashFunction = resources.HashFunction;

    return true;
}

bool TCompiler::compile(const char* const shaderStrings[],
                        size_t numStrings,
                        int compileOptions)
{
    TScopedPoolAllocator scopedAlloc(&allocator, true);
    clearResults();

    if (numStrings == 0)
        return true;

    // If compiling for WebGL, validate loop and indexing as well.
    if (isWebGLBasedSpec(shaderSpec))
        compileOptions |= SH_VALIDATE_LOOP_INDEXING;

    // First string is path of source file if flag is set. The actual source follows.
    const char* sourcePath = NULL;
    size_t firstSource = 0;
    if (compileOptions & SH_SOURCE_PATH)
    {
        sourcePath = shaderStrings[0];
        ++firstSource;
    }

    TIntermediate intermediate(infoSink);
    TParseContext parseContext(symbolTable, extensionBehavior, intermediate,
                               shaderType, shaderSpec, compileOptions, true,
                               sourcePath, infoSink);
    parseContext.fragmentPrecisionHigh = fragmentPrecisionHigh;
    GlobalParseContext = &parseContext;

    // We preserve symbols at the built-in level from compile-to-compile.
    // Start pushing the user-defined symbols at global level.
    symbolTable.push();
    if (!symbolTable.atGlobalLevel()) {
        infoSink.info.prefix(EPrefixInternalError);
        infoSink.info << "Wrong symbol table level";
    }

    // Parse shader.
    bool success =
        (PaParseStrings(numStrings - firstSource, &shaderStrings[firstSource], NULL, &parseContext) == 0) &&
        (parseContext.treeRoot != NULL);
    if (success) {
        TIntermNode* root = parseContext.treeRoot;
        success = intermediate.postProcess(root);

        if (success)
            success = detectCallDepth(root, infoSink, (compileOptions & SH_LIMIT_CALL_STACK_DEPTH) != 0);

        if (success && (compileOptions & SH_VALIDATE_LOOP_INDEXING))
            success = validateLimitations(root);

        if (success && (compileOptions & SH_TIMING_RESTRICTIONS))
            success = enforceTimingRestrictions(root, (compileOptions & SH_DEPENDENCY_GRAPH) != 0);

        if (success && shaderSpec == SH_CSS_SHADERS_SPEC)
            rewriteCSSShader(root);

        // Unroll for-loop markup needs to happen after validateLimitations pass.
        if (success && (compileOptions & SH_UNROLL_FOR_LOOP_WITH_INTEGER_INDEX))
            ForLoopUnroll::MarkForLoopsWithIntegerIndicesForUnrolling(root);

        // Built-in function emulation needs to happen after validateLimitations pass.
        if (success && (compileOptions & SH_EMULATE_BUILT_IN_FUNCTIONS))
            builtInFunctionEmulator.MarkBuiltInFunctionsForEmulation(root);

        // Clamping uniform array bounds needs to happen after validateLimitations pass.
        if (success && (compileOptions & SH_CLAMP_INDIRECT_ARRAY_BOUNDS))
            arrayBoundsClamper.MarkIndirectArrayBoundsForClamping(root);

        // Disallow expressions deemed too complex.
        if (success && (compileOptions & SH_LIMIT_EXPRESSION_COMPLEXITY))
            success = limitExpressionComplexity(root);

        // Call mapLongVariableNames() before collectAttribsUniforms() so in
        // collectAttribsUniforms() we already have the mapped symbol names and
        // we could composite mapped and original variable names.
        // Also, if we hash all the names, then no need to do this for long names.
        if (success && (compileOptions & SH_MAP_LONG_VARIABLE_NAMES) && hashFunction == NULL)
            mapLongVariableNames(root);

        if (success && (compileOptions & SH_ATTRIBUTES_UNIFORMS)) {
            collectAttribsUniforms(root);
            if (compileOptions & SH_ENFORCE_PACKING_RESTRICTIONS) {
                success = enforcePackingRestrictions();
                if (!success) {
                    infoSink.info.prefix(EPrefixError);
                    infoSink.info << "too many uniforms";
                }
            }
        }

        if (success && (compileOptions & SH_INTERMEDIATE_TREE))
            intermediate.outputTree(root);

        if (success && (compileOptions & SH_OBJECT_CODE))
            translate(root);
    }

    // Cleanup memory.
    intermediate.remove(parseContext.treeRoot);
    // Ensure symbol table is returned to the built-in level,
    // throwing away all but the built-ins.
    while (!symbolTable.atBuiltInLevel())
        symbolTable.pop();

    return success;
}

bool TCompiler::InitBuiltInSymbolTable(const ShBuiltInResources &resources)
{
    compileResources = resources;

    assert(symbolTable.isEmpty());
    symbolTable.push();

    TPublicType integer;
    integer.type = EbtInt;
    integer.size = 1;
    integer.matrix = false;
    integer.array = false;

    TPublicType floatingPoint;
    floatingPoint.type = EbtFloat;
    floatingPoint.size = 1;
    floatingPoint.matrix = false;
    floatingPoint.array = false;

    switch(shaderType)
    {
      case SH_FRAGMENT_SHADER:
        symbolTable.setDefaultPrecision(integer, EbpMedium);
        break;
      case SH_VERTEX_SHADER:
        symbolTable.setDefaultPrecision(integer, EbpHigh);
        symbolTable.setDefaultPrecision(floatingPoint, EbpHigh);
        break;
      default: assert(false && "Language not supported");
    }

    InsertBuiltInFunctions(shaderType, shaderSpec, resources, symbolTable);

    IdentifyBuiltIns(shaderType, shaderSpec, resources, symbolTable);

    return true;
}

void TCompiler::clearResults()
{
    arrayBoundsClamper.Cleanup();
    infoSink.info.erase();
    infoSink.obj.erase();
    infoSink.debug.erase();

    attribs.clear();
    uniforms.clear();

    builtInFunctionEmulator.Cleanup();

    nameMap.clear();
}

bool TCompiler::detectCallDepth(TIntermNode* root, TInfoSink& infoSink, bool limitCallStackDepth)
{
    DetectCallDepth detect(infoSink, limitCallStackDepth, maxCallStackDepth);
    root->traverse(&detect);
    switch (detect.detectCallDepth()) {
        case DetectCallDepth::kErrorNone:
            return true;
        case DetectCallDepth::kErrorMissingMain:
            infoSink.info.prefix(EPrefixError);
            infoSink.info << "Missing main()";
            return false;
        case DetectCallDepth::kErrorRecursion:
            infoSink.info.prefix(EPrefixError);
            infoSink.info << "Function recursion detected";
            return false;
        case DetectCallDepth::kErrorMaxDepthExceeded:
            infoSink.info.prefix(EPrefixError);
            infoSink.info << "Function call stack too deep";
            return false;
        default:
            UNREACHABLE();
            return false;
    }
}

void TCompiler::rewriteCSSShader(TIntermNode* root)
{
    RenameFunction renamer("main(", "css_main(");
    root->traverse(&renamer);
}

bool TCompiler::validateLimitations(TIntermNode* root) {
    ValidateLimitations validate(shaderType, infoSink.info);
    root->traverse(&validate);
    return validate.numErrors() == 0;
}

bool TCompiler::enforceTimingRestrictions(TIntermNode* root, bool outputGraph)
{
    if (shaderSpec != SH_WEBGL_SPEC) {
        infoSink.info << "Timing restrictions must be enforced under the WebGL spec.";
        return false;
    }

    if (shaderType == SH_FRAGMENT_SHADER) {
        TDependencyGraph graph(root);

        // Output any errors first.
        bool success = enforceFragmentShaderTimingRestrictions(graph);
        
        // Then, output the dependency graph.
        if (outputGraph) {
            TDependencyGraphOutput output(infoSink.info);
            output.outputAllSpanningTrees(graph);
        }
        
        return success;
    }
    else {
        return enforceVertexShaderTimingRestrictions(root);
    }
}

bool TCompiler::limitExpressionComplexity(TIntermNode* root)
{
    TIntermTraverser traverser;
    root->traverse(&traverser);
    TDependencyGraph graph(root);

    for (TFunctionCallVector::const_iterator iter = graph.beginUserDefinedFunctionCalls();
         iter != graph.endUserDefinedFunctionCalls();
         ++iter)
    {
        TGraphFunctionCall* samplerSymbol = *iter;
        TDependencyGraphTraverser graphTraverser;
        samplerSymbol->traverse(&graphTraverser);
    }

    if (traverser.getMaxDepth() > maxExpressionComplexity) {
        infoSink.info << "Expression too complex.";
        return false;
    }
    return true;
}

bool TCompiler::enforceFragmentShaderTimingRestrictions(const TDependencyGraph& graph)
{
    RestrictFragmentShaderTiming restrictor(infoSink.info);
    restrictor.enforceRestrictions(graph);
    return restrictor.numErrors() == 0;
}

bool TCompiler::enforceVertexShaderTimingRestrictions(TIntermNode* root)
{
    RestrictVertexShaderTiming restrictor(infoSink.info);
    restrictor.enforceRestrictions(root);
    return restrictor.numErrors() == 0;
}

void TCompiler::collectAttribsUniforms(TIntermNode* root)
{
    CollectAttribsUniforms collect(attribs, uniforms, hashFunction);
    root->traverse(&collect);
}

bool TCompiler::enforcePackingRestrictions()
{
    VariablePacker packer;
    return packer.CheckVariablesWithinPackingLimits(maxUniformVectors, uniforms);
}

void TCompiler::mapLongVariableNames(TIntermNode* root)
{
    ASSERT(longNameMap);
    MapLongVariableNames map(longNameMap);
    root->traverse(&map);
}

int TCompiler::getMappedNameMaxLength() const
{
    return MAX_SHORTENED_IDENTIFIER_SIZE + 1;
}

const TExtensionBehavior& TCompiler::getExtensionBehavior() const
{
    return extensionBehavior;
}

const ShBuiltInResources& TCompiler::getResources() const
{
    return compileResources;
}

const ArrayBoundsClamper& TCompiler::getArrayBoundsClamper() const
{
    return arrayBoundsClamper;
}

ShArrayIndexClampingStrategy TCompiler::getArrayIndexClampingStrategy() const
{
    return clampingStrategy;
}

const BuiltInFunctionEmulator& TCompiler::getBuiltInFunctionEmulator() const
{
    return builtInFunctionEmulator;
}
