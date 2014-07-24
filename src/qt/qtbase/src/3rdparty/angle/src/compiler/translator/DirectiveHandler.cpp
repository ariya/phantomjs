//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/DirectiveHandler.h"

#include <sstream>

#include "compiler/translator/compilerdebug.h"
#include "compiler/translator/Diagnostics.h"

static TBehavior getBehavior(const std::string& str)
{
    static const std::string kRequire("require");
    static const std::string kEnable("enable");
    static const std::string kDisable("disable");
    static const std::string kWarn("warn");

    if (str == kRequire) return EBhRequire;
    else if (str == kEnable) return EBhEnable;
    else if (str == kDisable) return EBhDisable;
    else if (str == kWarn) return EBhWarn;
    return EBhUndefined;
}

TDirectiveHandler::TDirectiveHandler(TExtensionBehavior& extBehavior,
                                     TDiagnostics& diagnostics)
    : mExtensionBehavior(extBehavior),
      mDiagnostics(diagnostics)
{
}

TDirectiveHandler::~TDirectiveHandler()
{
}

void TDirectiveHandler::handleError(const pp::SourceLocation& loc,
                                    const std::string& msg)
{
    mDiagnostics.writeInfo(pp::Diagnostics::PP_ERROR, loc, msg, "", "");
}

void TDirectiveHandler::handlePragma(const pp::SourceLocation& loc,
                                     const std::string& name,
                                     const std::string& value)
{
    static const std::string kSTDGL("STDGL");
    static const std::string kOptimize("optimize");
    static const std::string kDebug("debug");
    static const std::string kOn("on");
    static const std::string kOff("off");

    bool invalidValue = false;
    if (name == kSTDGL)
    {
        // The STDGL pragma is used to reserve pragmas for use by future
        // revisions of GLSL. Ignore it.
        return;
    }
    else if (name == kOptimize)
    {
        if (value == kOn) mPragma.optimize = true;
        else if (value == kOff) mPragma.optimize = false;
        else invalidValue = true;
    }
    else if (name == kDebug)
    {
        if (value == kOn) mPragma.debug = true;
        else if (value == kOff) mPragma.debug = false;
        else invalidValue = true;
    }
    else
    {
        mDiagnostics.report(pp::Diagnostics::PP_UNRECOGNIZED_PRAGMA, loc, name);
        return;
    }

    if (invalidValue)
      mDiagnostics.writeInfo(pp::Diagnostics::PP_ERROR, loc,
                             "invalid pragma value", value,
                             "'on' or 'off' expected");
}

void TDirectiveHandler::handleExtension(const pp::SourceLocation& loc,
                                        const std::string& name,
                                        const std::string& behavior)
{
    static const std::string kExtAll("all");

    TBehavior behaviorVal = getBehavior(behavior);
    if (behaviorVal == EBhUndefined)
    {
        mDiagnostics.writeInfo(pp::Diagnostics::PP_ERROR, loc,
                               "behavior", name, "invalid");
        return;
    }

    if (name == kExtAll)
    {
        if (behaviorVal == EBhRequire)
        {
            mDiagnostics.writeInfo(pp::Diagnostics::PP_ERROR, loc,
                                   "extension", name,
                                   "cannot have 'require' behavior");
        }
        else if (behaviorVal == EBhEnable)
        {
            mDiagnostics.writeInfo(pp::Diagnostics::PP_ERROR, loc,
                                   "extension", name,
                                   "cannot have 'enable' behavior");
        }
        else
        {
            for (TExtensionBehavior::iterator iter = mExtensionBehavior.begin();
                 iter != mExtensionBehavior.end(); ++iter)
                iter->second = behaviorVal;
        }
        return;
    }

    TExtensionBehavior::iterator iter = mExtensionBehavior.find(name);
    if (iter != mExtensionBehavior.end())
    {
        iter->second = behaviorVal;
        return;
    }

    pp::Diagnostics::Severity severity = pp::Diagnostics::PP_ERROR;
    switch (behaviorVal) {
      case EBhRequire:
        severity = pp::Diagnostics::PP_ERROR;
        break;
      case EBhEnable:
      case EBhWarn:
      case EBhDisable:
        severity = pp::Diagnostics::PP_WARNING;
        break;
      default:
        UNREACHABLE();
        break;
    }
    mDiagnostics.writeInfo(severity, loc,
                           "extension", name, "is not supported");
}

void TDirectiveHandler::handleVersion(const pp::SourceLocation& loc,
                                      int version)
{
    static const int kVersion = 100;

    if (version != kVersion)
    {
        std::stringstream stream;
        stream << version;
        std::string str = stream.str();
        mDiagnostics.writeInfo(pp::Diagnostics::PP_ERROR, loc,
                               "version number", str, "not supported");
    }
}
