//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _SHHANDLE_INCLUDED_
#define _SHHANDLE_INCLUDED_

//
// Machine independent part of the compiler private objects
// sent as ShHandle to the driver.
//
// This should not be included by driver code.
//

#include "GLSLANG/ShaderLang.h"

#include "compiler/BuiltInFunctionEmulator.h"
#include "compiler/ExtensionBehavior.h"
#include "compiler/HashNames.h"
#include "compiler/InfoSink.h"
#include "compiler/SymbolTable.h"
#include "compiler/VariableInfo.h"
#include "third_party/compiler/ArrayBoundsClamper.h"

class LongNameMap;
class TCompiler;
class TDependencyGraph;
class TranslatorHLSL;

//
// Helper function to identify specs that are based on the WebGL spec,
// like the CSS Shaders spec.
//
bool isWebGLBasedSpec(ShShaderSpec spec);

//
// The base class used to back handles returned to the driver.
//
class TShHandleBase {
public:
    TShHandleBase();
    virtual ~TShHandleBase();
    virtual TCompiler* getAsCompiler() { return 0; }
    virtual TranslatorHLSL* getAsTranslatorHLSL() { return 0; }

protected:
    // Memory allocator. Allocates and tracks memory required by the compiler.
    // Deallocates all memory when compiler is destructed.
    TPoolAllocator allocator;
};

//
// The base class for the machine dependent compiler to derive from
// for managing object code from the compile.
//
class TCompiler : public TShHandleBase {
public:
    TCompiler(ShShaderType type, ShShaderSpec spec);
    virtual ~TCompiler();
    virtual TCompiler* getAsCompiler() { return this; }

    bool Init(const ShBuiltInResources& resources);
    bool compile(const char* const shaderStrings[],
                 size_t numStrings,
                 int compileOptions);

    // Get results of the last compilation.
    TInfoSink& getInfoSink() { return infoSink; }
    const TVariableInfoList& getAttribs() const { return attribs; }
    const TVariableInfoList& getUniforms() const { return uniforms; }
    int getMappedNameMaxLength() const;

    ShHashFunction64 getHashFunction() const { return hashFunction; }
    NameMap& getNameMap() { return nameMap; }
    TSymbolTable& getSymbolTable() { return symbolTable; }

protected:
    ShShaderType getShaderType() const { return shaderType; }
    ShShaderSpec getShaderSpec() const { return shaderSpec; }
    // Initialize symbol-table with built-in symbols.
    bool InitBuiltInSymbolTable(const ShBuiltInResources& resources);
    // Clears the results from the previous compilation.
    void clearResults();
    // Return true if function recursion is detected or call depth exceeded.
    bool detectCallDepth(TIntermNode* root, TInfoSink& infoSink, bool limitCallStackDepth);
    // Rewrites a shader's intermediate tree according to the CSS Shaders spec.
    void rewriteCSSShader(TIntermNode* root);
    // Returns true if the given shader does not exceed the minimum
    // functionality mandated in GLSL 1.0 spec Appendix A.
    bool validateLimitations(TIntermNode* root);
    // Collect info for all attribs and uniforms.
    void collectAttribsUniforms(TIntermNode* root);
    // Map long variable names into shorter ones.
    void mapLongVariableNames(TIntermNode* root);
    // Translate to object code.
    virtual void translate(TIntermNode* root) = 0;
    // Returns true if, after applying the packing rules in the GLSL 1.017 spec
    // Appendix A, section 7, the shader does not use too many uniforms.
    bool enforcePackingRestrictions();
    // Returns true if the shader passes the restrictions that aim to prevent timing attacks.
    bool enforceTimingRestrictions(TIntermNode* root, bool outputGraph);
    // Returns true if the shader does not use samplers.
    bool enforceVertexShaderTimingRestrictions(TIntermNode* root);
    // Returns true if the shader does not use sampler dependent values to affect control 
    // flow or in operations whose time can depend on the input values.
    bool enforceFragmentShaderTimingRestrictions(const TDependencyGraph& graph);
    // Return true if the maximum expression complexity below the limit.
    bool limitExpressionComplexity(TIntermNode* root);
    // Get built-in extensions with default behavior.
    const TExtensionBehavior& getExtensionBehavior() const;
    // Get the resources set by InitBuiltInSymbolTable
    const ShBuiltInResources& getResources() const;

    const ArrayBoundsClamper& getArrayBoundsClamper() const;
    ShArrayIndexClampingStrategy getArrayIndexClampingStrategy() const;
    const BuiltInFunctionEmulator& getBuiltInFunctionEmulator() const;

private:
    ShShaderType shaderType;
    ShShaderSpec shaderSpec;

    int maxUniformVectors;
    int maxExpressionComplexity;
    int maxCallStackDepth;

    ShBuiltInResources compileResources;

    // Built-in symbol table for the given language, spec, and resources.
    // It is preserved from compile-to-compile.
    TSymbolTable symbolTable;
    // Built-in extensions with default behavior.
    TExtensionBehavior extensionBehavior;
    bool fragmentPrecisionHigh;

    ArrayBoundsClamper arrayBoundsClamper;
    ShArrayIndexClampingStrategy clampingStrategy;
    BuiltInFunctionEmulator builtInFunctionEmulator;

    // Results of compilation.
    TInfoSink infoSink;  // Output sink.
    TVariableInfoList attribs;  // Active attributes in the compiled shader.
    TVariableInfoList uniforms;  // Active uniforms in the compiled shader.

    // Cached copy of the ref-counted singleton.
    LongNameMap* longNameMap;

    // name hashing.
    ShHashFunction64 hashFunction;
    NameMap nameMap;
};

//
// This is the interface between the machine independent code
// and the machine dependent code.
//
// The machine dependent code should derive from the classes
// above. Then Construct*() and Delete*() will create and 
// destroy the machine dependent objects, which contain the
// above machine independent information.
//
TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output);
void DeleteCompiler(TCompiler*);

#endif // _SHHANDLE_INCLUDED_
