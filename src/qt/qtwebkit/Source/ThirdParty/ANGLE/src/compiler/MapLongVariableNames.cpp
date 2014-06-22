//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/MapLongVariableNames.h"

namespace {

TString mapLongName(size_t id, const TString& name, bool isGlobal)
{
    ASSERT(name.size() > MAX_SHORTENED_IDENTIFIER_SIZE);
    TStringStream stream;
    stream << "webgl_";
    if (isGlobal)
        stream << "g";
    stream << id;
    if (name[0] != '_')
        stream << "_";
    stream << name.substr(0, MAX_SHORTENED_IDENTIFIER_SIZE - stream.str().size());
    return stream.str();
}

LongNameMap* gLongNameMapInstance = NULL;

}  // anonymous namespace

LongNameMap::LongNameMap()
    : refCount(0)
{
}

LongNameMap::~LongNameMap()
{
}

// static
LongNameMap* LongNameMap::GetInstance()
{
    if (gLongNameMapInstance == NULL)
        gLongNameMapInstance = new LongNameMap;
    gLongNameMapInstance->refCount++;
    return gLongNameMapInstance;
}

void LongNameMap::Release()
{
    ASSERT(gLongNameMapInstance == this);
    ASSERT(refCount > 0);
    refCount--;
    if (refCount == 0) {
        delete gLongNameMapInstance;
        gLongNameMapInstance = NULL;
    }
}

const char* LongNameMap::Find(const char* originalName) const
{
    std::map<std::string, std::string>::const_iterator it = mLongNameMap.find(
        originalName);
    if (it != mLongNameMap.end())
        return (*it).second.c_str();
    return NULL;
}

void LongNameMap::Insert(const char* originalName, const char* mappedName)
{
    mLongNameMap.insert(std::map<std::string, std::string>::value_type(
        originalName, mappedName));
}

size_t LongNameMap::Size() const
{
    return mLongNameMap.size();
}

MapLongVariableNames::MapLongVariableNames(LongNameMap* globalMap)
{
    ASSERT(globalMap);
    mGlobalMap = globalMap;
}

void MapLongVariableNames::visitSymbol(TIntermSymbol* symbol)
{
    ASSERT(symbol != NULL);
    if (symbol->getSymbol().size() > MAX_SHORTENED_IDENTIFIER_SIZE) {
        switch (symbol->getQualifier()) {
          case EvqVaryingIn:
          case EvqVaryingOut:
          case EvqInvariantVaryingIn:
          case EvqInvariantVaryingOut:
          case EvqUniform:
            symbol->setSymbol(
                mapGlobalLongName(symbol->getSymbol()));
            break;
          default:
            symbol->setSymbol(
                mapLongName(symbol->getId(), symbol->getSymbol(), false));
            break;
        };
    }
}

bool MapLongVariableNames::visitLoop(Visit, TIntermLoop* node)
{
    if (node->getInit())
        node->getInit()->traverse(this);
    return true;
}

TString MapLongVariableNames::mapGlobalLongName(const TString& name)
{
    ASSERT(mGlobalMap);
    const char* mappedName = mGlobalMap->Find(name.c_str());
    if (mappedName != NULL)
        return mappedName;
    size_t id = mGlobalMap->Size();
    TString rt = mapLongName(id, name, true);
    mGlobalMap->Insert(name.c_str(), rt.c_str());
    return rt;
}
