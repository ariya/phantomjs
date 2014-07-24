/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009, 2011 Google Inc. All Rights Reserved.
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
 *
 */

#include "config.h"

#if ENABLE(WORKERS) && ENABLE(INDEXED_DATABASE)

#include "WorkerGlobalScopeIndexedDatabase.h"

#include "IDBFactory.h"
#include "IDBFactoryBackendInterface.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"

namespace WebCore {

WorkerGlobalScopeIndexedDatabase::WorkerGlobalScopeIndexedDatabase()
{
}

WorkerGlobalScopeIndexedDatabase::~WorkerGlobalScopeIndexedDatabase()
{
}

const char* WorkerGlobalScopeIndexedDatabase::supplementName()
{
    return "WorkerGlobalScopeIndexedDatabase";
}

WorkerGlobalScopeIndexedDatabase* WorkerGlobalScopeIndexedDatabase::from(ScriptExecutionContext* context)
{
    WorkerGlobalScopeIndexedDatabase* supplement = static_cast<WorkerGlobalScopeIndexedDatabase*>(Supplement<ScriptExecutionContext>::from(context, supplementName()));
    if (!supplement) {
        supplement = new WorkerGlobalScopeIndexedDatabase();
        provideTo(context, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

IDBFactory* WorkerGlobalScopeIndexedDatabase::indexedDB(ScriptExecutionContext* context)
{
    return from(context)->indexedDB();
}

IDBFactory* WorkerGlobalScopeIndexedDatabase::indexedDB()
{
    if (!m_factoryBackend)
        m_factoryBackend = IDBFactoryBackendInterface::create();
    if (!m_idbFactory)
        m_idbFactory = IDBFactory::create(m_factoryBackend.get());
    return m_idbFactory.get();
}

} // namespace WebCore

#endif // ENABLE(WORKERS) && ENABLE(INDEXED_DATABASE)
