/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IDBFactory.h"

#if ENABLE(INDEXED_DATABASE)

#include "DOMStringList.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "GroupSettings.h"
#include "IDBDatabase.h"
#include "IDBDatabaseException.h"
#include "IDBFactoryBackendInterface.h"
#include "IDBKeyRange.h"
#include "IDBRequest.h"
#include "Page.h"
#include "PageGroup.h"
#include "SecurityOrigin.h"

namespace WebCore {

IDBFactory::IDBFactory(IDBFactoryBackendInterface* factory)
    : m_factoryBackend(factory)
{
    // We pass a reference to this object before it can be adopted.
    relaxAdoptionRequirement();
}

IDBFactory::~IDBFactory()
{
}

PassRefPtr<IDBRequest> IDBFactory::open(ScriptExecutionContext* context, const String& name, ExceptionCode& ec)
{
    if (!context->isDocument()) {
        // FIXME: make this work with workers.
        return 0;
    }

    Document* document = static_cast<Document*>(context);
    if (!document->frame() || !document->page())
        return 0;

    if (name.isNull()) {
        ec = IDBDatabaseException::NON_TRANSIENT_ERR;
        return 0;
    }

    RefPtr<IDBRequest> request = IDBRequest::create(document, IDBAny::create(this), 0);
    GroupSettings* groupSettings = document->page()->group().groupSettings();
    m_factoryBackend->open(name, request, document->securityOrigin(), document->frame(), groupSettings->indexedDBDatabasePath(), groupSettings->indexedDBQuotaBytes(), IDBFactoryBackendInterface::DefaultBackingStore);
    return request;
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
