//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_DIAGNOSTICS_H_
#define COMPILER_DIAGNOSTICS_H_

#include "compiler/preprocessor/DiagnosticsBase.h"

class TInfoSink;

class TDiagnostics : public pp::Diagnostics
{
  public:
    TDiagnostics(TInfoSink& infoSink);
    virtual ~TDiagnostics();

    TInfoSink& infoSink() { return mInfoSink; }

    int numErrors() const { return mNumErrors; }
    int numWarnings() const { return mNumWarnings; }

    void writeInfo(Severity severity,
                   const pp::SourceLocation& loc,
                   const std::string& reason,
                   const std::string& token,
                   const std::string& extra);

    void writeDebug(const std::string& str);

  protected:
    virtual void print(ID id,
                       const pp::SourceLocation& loc,
                       const std::string& text);

  private:
    TInfoSink& mInfoSink;
    int mNumErrors;
    int mNumWarnings;
};

#endif  // COMPILER_DIAGNOSTICS_H_
