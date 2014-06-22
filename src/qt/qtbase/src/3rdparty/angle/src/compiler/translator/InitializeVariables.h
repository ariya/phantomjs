//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_INITIALIZE_VARIABLES_H_
#define COMPILER_INITIALIZE_VARIABLES_H_

#include "compiler/translator/intermediate.h"

class InitializeVariables : public TIntermTraverser
{
  public:
    struct InitVariableInfo
    {
        TString name;
        TType type;

        InitVariableInfo(const TString& _name, const TType& _type)
            : name(_name),
              type(_type)
        {
        }
    };
    typedef TVector<InitVariableInfo> InitVariableInfoList;

    InitializeVariables(const InitVariableInfoList& vars)
        : mCodeInserted(false),
          mVariables(vars)
    {
    }

  protected:
    virtual bool visitBinary(Visit visit, TIntermBinary* node) { return false; }
    virtual bool visitUnary(Visit visit, TIntermUnary* node) { return false; }
    virtual bool visitSelection(Visit visit, TIntermSelection* node) { return false; }
    virtual bool visitLoop(Visit visit, TIntermLoop* node) { return false; }
    virtual bool visitBranch(Visit visit, TIntermBranch* node) { return false; }

    virtual bool visitAggregate(Visit visit, TIntermAggregate* node);

  private:
    void insertInitCode(TIntermSequence& sequence);

    InitVariableInfoList mVariables;
    bool mCodeInserted;
};

#endif  // COMPILER_INITIALIZE_VARIABLES_H_
