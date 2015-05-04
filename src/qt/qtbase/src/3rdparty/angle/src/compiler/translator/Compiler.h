//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
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

#include "compiler/translator/BuiltInFunctionEmulator.h"
#include "compiler/translator/ExtensionBehavior.h"
#include "compiler/translator/HashNames.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/Pragma.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/VariableInfo.h"
#include "third_party/compiler/ArrayBoundsClamper.h"

class TCompiler;
class TDependencyGraph;
class TranslatorHLSL;

//
// Helper function to identify specs that are based on the WebGL spec,
// like the CSS Shaders spec.
//
bool IsWebGLBasedSpec(ShShaderSpec spec);

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
class TCompiler : public TShHandleBase
{
  public:
    TCompiler(sh::GLenum type, ShShaderSpec spec, ShShaderOutput output);
    virtual ~TCompiler();
    virtual TCompiler* getAsCompiler() { return this; }

    bool Init(const ShBuiltInResources& resources);
    bool compile(const char* const shaderStrings[],
                 size_t numStrings,
                 int compileOptions);

    // Get results of the last compilation.
    int getShaderVersion() const { return shaderVersion; }
    TInfoSink& getInfoSink() { return infoSink; }

    const std::vector<sh::Attribute> &getAttributes() const { return attributes; }
    const std::vector<sh::Attribute> &getOutputVariables() const { return outputVariables; }
    const std::vector<sh::Uniform> &getUniforms() const { return uniforms; }
    const std::vector<sh::Varying> &getVaryings() const { return varyings; }
    const std::vector<sh::InterfaceBlock> &getInterfaceBlocks() const { return interfaceBlocks; }

    ShHashFunction64 getHashFunction() const { return hashFunction; }
    NameMap& getNameMap() { return nameMap; }
    TSymbolTable& getSymbolTable() { return symbolTable; }
    ShShaderSpec getShaderSpec() const { return shaderSpec; }
    ShShaderOutput getOutputType() const { return outputType; }
    const std::string &getBuiltInResourcesString() const { return builtInResourcesString; }

    // Get the resources set by InitBuiltInSymbolTable
    const ShBuiltInResources& getResources() const;

  protected:
    sh::GLenum getShaderType() const { return shaderType; }
    // Initialize symbol-table with built-in symbols.
    bool InitBuiltInSymbolTable(const ShBuiltInResources& resources);
    // Compute the string representation of the built-in resources
    void setResourceString();
    // Clears the results from the previous compilation.
    void clearResults();
    // Return true if function recursion is detected or call depth exceeded.
    bool detectCallDepth(TIntermNode* root, TInfoSink& infoSink, bool limitCallStackDepth);
    // Returns true if a program has no conflicting or missing fragment outputs
    bool validateOutputs(TIntermNode* root);
    // Rewrites a shader's intermediate tree according to the CSS Shaders spec.
    void rewriteCSSShader(TIntermNode* root);
    // Returns true if the given shader does not exceed the minimum
    // functionality mandated in GLSL 1.0 spec Appendix A.
    bool validateLimitations(TIntermNode* root);
    // Collect info for all attribs, uniforms, varyings.
    void collectVariables(TIntermNode* root);
    // Translate to object code.
    virtual void translate(TIntermNode* root) = 0;
    // Returns true if, after applying the packing rules in the GLSL 1.017 spec
    // Appendix A, section 7, the shader does not use too many uniforms.
    bool enforcePackingRestrictions();
    // Insert statements to initialize varyings without static use in the beginning
    // of main(). It is to work around a Mac driver where such varyings in a vertex
    // shader may be optimized out incorrectly at compile time, causing a link failure.
    // This function should only be applied to vertex shaders.
    void initializeVaryingsWithoutStaticUse(TIntermNode* root);
    // Insert gl_Position = vec4(0,0,0,0) to the beginning of main().
    // It is to work around a Linux driver bug where missing this causes compile failure
    // while spec says it is allowed.
    // This function should only be applied to vertex shaders.
    void initializeGLPosition(TIntermNode* root);
    // Returns true if the shader passes the restrictions that aim to prevent timing attacks.
    bool enforceTimingRestrictions(TIntermNode* root, bool outputGraph);
    // Returns true if the shader does not use samplers.
    bool enforceVertexShaderTimingRestrictions(TIntermNode* root);
    // Returns true if the shader does not use sampler dependent values to affect control
    // flow or in operations whose time can depend on the input values.
    bool enforceFragmentShaderTimingRestrictions(const TDependencyGraph& graph);
    // Return true if the maximum expression complexity is below the limit.
    bool limitExpressionComplexity(TIntermNode* root);
    // Get built-in extensions with default behavior.
    const TExtensionBehavior& getExtensionBehavior() const;
    const TPragma& getPragma() const { return mPragma; }
    void writePragma();

    const ArrayBoundsClamper& getArrayBoundsClamper() const;
    ShArrayIndexClampingStrategy getArrayIndexClampingStrategy() const;
    const BuiltInFunctionEmulator& getBuiltInFunctionEmulator() const;

    std::vector<sh::Attribute> attributes;
    std::vector<sh::Attribute> outputVariables;
    std::vector<sh::Uniform> uniforms;
    std::vector<sh::ShaderVariable> expandedUniforms;
    std::vector<sh::Varying> varyings;
    std::vector<sh::InterfaceBlock> interfaceBlocks;

  private:
    sh::GLenum shaderType;
    ShShaderSpec shaderSpec;
    ShShaderOutput outputType;

    int maxUniformVectors;
    int maxExpressionComplexity;
    int maxCallStackDepth;

    ShBuiltInResources compileResources;
    std::string builtInResourcesString;

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
    int shaderVersion;
    TInfoSink infoSink;  // Output sink.

    // name hashing.
    ShHashFunction64 hashFunction;
    NameMap nameMap;

    TPragma mPragma;
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
    sh::GLenum type, ShShaderSpec spec, ShShaderOutput output);
void DeleteCompiler(TCompiler*);

#endif // _SHHANDLE_INCLUDED_
