//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef CROSSCOMPILERGLSL_OUTPUTGLSLBASE_H_
#define CROSSCOMPILERGLSL_OUTPUTGLSLBASE_H_

#include <set>

#include "compiler/translator/IntermNode.h"
#include "compiler/translator/LoopInfo.h"
#include "compiler/translator/ParseContext.h"

class TOutputGLSLBase : public TIntermTraverser
{
  public:
    TOutputGLSLBase(TInfoSinkBase &objSink,
                    ShArrayIndexClampingStrategy clampingStrategy,
                    ShHashFunction64 hashFunction,
                    NameMap &nameMap,
                    TSymbolTable& symbolTable,
                    int shaderVersion);

  protected:
    TInfoSinkBase &objSink() { return mObjSink; }
    void writeTriplet(Visit visit, const char *preStr, const char *inStr, const char *postStr);
    void writeVariableType(const TType &type);
    virtual bool writeVariablePrecision(TPrecision precision) = 0;
    void writeFunctionParameters(const TIntermSequence &args);
    const ConstantUnion *writeConstantUnion(const TType &type, const ConstantUnion *pConstUnion);
    TString getTypeName(const TType &type);

    virtual void visitSymbol(TIntermSymbol *node);
    virtual void visitConstantUnion(TIntermConstantUnion *node);
    virtual bool visitBinary(Visit visit, TIntermBinary *node);
    virtual bool visitUnary(Visit visit, TIntermUnary *node);
    virtual bool visitSelection(Visit visit, TIntermSelection *node);
    virtual bool visitAggregate(Visit visit, TIntermAggregate *node);
    virtual bool visitLoop(Visit visit, TIntermLoop *node);
    virtual bool visitBranch(Visit visit, TIntermBranch *node);

    void visitCodeBlock(TIntermNode *node);

    // Return the original name if hash function pointer is NULL;
    // otherwise return the hashed name.
    TString hashName(const TString &name);
    // Same as hashName(), but without hashing built-in variables.
    TString hashVariableName(const TString &name);
    // Same as hashName(), but without hashing built-in functions.
    TString hashFunctionName(const TString &mangled_name);
    // Used to translate function names for differences between ESSL and GLSL
    virtual TString translateTextureFunction(TString &name) { return name; }

  private:
    bool structDeclared(const TStructure *structure) const;
    void declareStruct(const TStructure *structure);

    void writeBuiltInFunctionTriplet(Visit visit, const char *preStr, bool useEmulatedFunction);

    TInfoSinkBase &mObjSink;
    bool mDeclaringVariables;

    // This set contains all the ids of the structs from every scope.
    std::set<int> mDeclaredStructs;

    // Stack of loops that need to be unrolled.
    TLoopStack mLoopUnrollStack;

    ShArrayIndexClampingStrategy mClampingStrategy;

    // name hashing.
    ShHashFunction64 mHashFunction;

    NameMap &mNameMap;

    TSymbolTable &mSymbolTable;

    const int mShaderVersion;
};

#endif  // CROSSCOMPILERGLSL_OUTPUTGLSLBASE_H_
