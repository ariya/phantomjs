/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(NOTIFICATIONS)

#include "NotificationCenter.h"

#include "Document.h"
#include "VoidCallback.h"
#include "WorkerContext.h"

namespace WebCore {

NotificationCenter::NotificationCenter(ScriptExecutionContext* context, NotificationPresenter* presenter)
    : ActiveDOMObject(context, this)
    , m_notificationPresenter(presenter) {}

int NotificationCenter::checkPermission()
{
    if (!presenter() || !scriptExecutionContext())
        return NotificationPresenter::PermissionDenied;
    return m_notificationPresenter->checkPermission(scriptExecutionContext());
}

void NotificationCenter::requestPermission(PassRefPtr<VoidCallback> callback)
{
    if (!presenter() || !scriptExecutionContext())
        return;
    m_notificationPresenter->requestPermission(scriptExecutionContext(), callback);
}

void NotificationCenter::disconnectFrame()
{
    // m_notificationPresenter should never be 0. But just to be safe, we check it here.
    // Due to the mysterious bug http://code.google.com/p/chromium/issues/detail?id=49323.
    ASSERT(m_notificationPresenter);
    if (!m_notificationPresenter)
        return;
    m_notificationPresenter->cancelRequestsForPermission(scriptExecutionContext());
    m_notificationPresenter = 0;
}

} // namespace WebCore

#endif // ENABLE(NOTIFICATIONS)
