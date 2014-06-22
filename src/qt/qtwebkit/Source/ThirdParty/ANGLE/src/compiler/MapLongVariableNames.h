//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_MAP_LONG_VARIABLE_NAMES_H_
#define COMPILER_MAP_LONG_VARIABLE_NAMES_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/VariableInfo.h"

// This size does not include '\0' in the end.
#define MAX_SHORTENED_IDENTIFIER_SIZE 32

// This is a ref-counted singleton. GetInstance() returns a pointer to the
// singleton, and after use, call Release(). GetInstance() and Release() should
// be paired.
class LongNameMap {
public:
    static LongNameMap* GetInstance();
    void Release();

    // Return the mapped name if <originalName, mappedName> is in the map;
    // otherwise, return NULL.
    const char* Find(const char* originalName) const;

    // Insert a pair into the map.
    void Insert(const char* originalName, const char* mappedName);

    // Return the number of entries in the map.
    size_t Size() const;

private:
    LongNameMap();
    ~LongNameMap();

    size_t refCount;
    std::map<std::string, std::string> mLongNameMap;
};

// Traverses intermediate tree to map attributes and uniforms names that are
// longer than MAX_SHORTENED_IDENTIFIER_SIZE to MAX_SHORTENED_IDENTIFIER_SIZE.
class MapLongVariableNames : public TIntermTraverser {
public:
    MapLongVariableNames(LongNameMap* globalMap);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitLoop(Visit, TIntermLoop*);

private:
    TString mapGlobalLongName(const TString& name);

    LongNameMap* mGlobalMap;
};

#endif  // COMPILER_MAP_LONG_VARIABLE_NAMES_H_
