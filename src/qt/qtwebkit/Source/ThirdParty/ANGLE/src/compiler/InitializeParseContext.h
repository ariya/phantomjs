//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef __INITIALIZE_PARSE_CONTEXT_INCLUDED_
#define __INITIALIZE_PARSE_CONTEXT_INCLUDED_

bool InitializeParseContextIndex();
bool FreeParseContextIndex();

bool InitializeGlobalParseContext();
bool FreeParseContext();

struct TParseContext;
typedef TParseContext* TParseContextPointer;
extern TParseContextPointer& GetGlobalParseContext();
#define GlobalParseContext GetGlobalParseContext()

typedef struct TThreadParseContextRec
{
    TParseContext *lpGlobalParseContext;
} TThreadParseContext;

#endif // __INITIALIZE_PARSE_CONTEXT_INCLUDED_
