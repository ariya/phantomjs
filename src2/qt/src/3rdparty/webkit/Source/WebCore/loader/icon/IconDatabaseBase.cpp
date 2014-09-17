/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "IconDatabaseBase.h"

#include "IconDatabase.h"
#include "SharedBuffer.h"

namespace WebCore {

String IconDatabaseBase::synchronousIconURLForPageURL(const String&)
{
    return String();
}

String IconDatabaseBase::databasePath() const
{
    return String();
}

bool IconDatabaseBase::open(const String&, const String&)
{
    return false;
}

static IconDatabaseBase* globalDatabase = 0;

// Functions to get/set the global icon database.
IconDatabaseBase& iconDatabase()
{
    if (globalDatabase)
        return *globalDatabase;

    static IconDatabaseBase* defaultDatabase = 0;        
    if (!defaultDatabase)
        defaultDatabase = new IconDatabase;

    return *defaultDatabase;
}

void setGlobalIconDatabase(IconDatabaseBase* newGlobalDatabase)
{
    globalDatabase = newGlobalDatabase;
}

} // namespace WebCore
