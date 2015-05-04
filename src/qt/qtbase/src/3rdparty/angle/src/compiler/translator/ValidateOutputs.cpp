//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/ValidateOutputs.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/InitializeParseContext.h"
#include "compiler/translator/ParseContext.h"

ValidateOutputs::ValidateOutputs(TInfoSinkBase& sink, int maxDrawBuffers)
    : mSink(sink),
      mMaxDrawBuffers(maxDrawBuffers),
      mNumErrors(0),
      mHasUnspecifiedOutputLocation(false)
{
}

void ValidateOutputs::visitSymbol(TIntermSymbol *symbol)
{
    TString name = symbol->getSymbol();
    TQualifier qualifier = symbol->getQualifier();

    if (mVisitedSymbols.count(name) == 1)
        return;

    mVisitedSymbols.insert(name);

    if (qualifier == EvqFragmentOut)
    {
        const TType &type = symbol->getType();
        const int location = type.getLayoutQualifier().location;

        if (mHasUnspecifiedOutputLocation)
        {
            error(symbol->getLine(), "must explicitly specify all locations when using multiple fragment outputs", name.c_str());
        }
        else if (location == -1)
        {
            mHasUnspecifiedOutputLocation = true;
        }
        else
        {
            OutputMap::iterator mapEntry = mOutputMap.find(location);
            if (mapEntry == mOutputMap.end())
            {
                const int elementCount = type.isArray() ? type.getArraySize() : 1;
                if (location + elementCount > mMaxDrawBuffers)
                {
                    error(symbol->getLine(), "output location must be < MAX_DRAW_BUFFERS", name.c_str());
                }

                for (int elementIndex = 0; elementIndex < elementCount; elementIndex++)
                {
                    const int offsetLocation = location + elementIndex;
                    mOutputMap[offsetLocation] = symbol;
                }
            }
            else
            {
                std::stringstream strstr;
                strstr << "conflicting output locations with previously defined output '"
                       << mapEntry->second->getSymbol() << "'";

                error(symbol->getLine(), strstr.str().c_str(), name.c_str());
            }
        }
    }
}

void ValidateOutputs::error(TSourceLoc loc, const char *reason, const char* token)
{
    mSink.prefix(EPrefixError);
    mSink.location(loc);
    mSink << "'" << token << "' : " << reason << "\n";
    mNumErrors++;
}
