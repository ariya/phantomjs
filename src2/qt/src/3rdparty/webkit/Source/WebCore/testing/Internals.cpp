/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "Internals.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "Settings.h"

namespace WebCore {

const char* Internals::internalsId = "internals";

PassRefPtr<Internals> Internals::create()
{
    return adoptRef(new Internals);
}

Internals::~Internals()
{
}

Internals::Internals()
    : passwordEchoDurationInSecondsBackedUp(false)
    , passwordEchoEnabledBackedUp(false)
{
}

void Internals::setPasswordEchoEnabled(Document* document, bool enabled, ExceptionCode& ec)
{
    if (!document || !document->settings()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    if (!passwordEchoEnabledBackedUp) {
        passwordEchoEnabledBackup = document->settings()->passwordEchoEnabled();
        passwordEchoEnabledBackedUp = true;
    }
    document->settings()->setPasswordEchoEnabled(enabled);
}

void Internals::setPasswordEchoDurationInSeconds(Document* document, double durationInSeconds, ExceptionCode& ec)
{
    if (!document || !document->settings()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    if (!passwordEchoDurationInSecondsBackedUp) {
        passwordEchoDurationInSecondsBackup = document->settings()->passwordEchoDurationInSeconds();
        passwordEchoDurationInSecondsBackedUp = true;
    }
    document->settings()->setPasswordEchoDurationInSeconds(durationInSeconds);
}

void Internals::reset(Document* document)
{
    if (!document || !document->settings())
        return;

    if (passwordEchoDurationInSecondsBackedUp) {
        document->settings()->setPasswordEchoDurationInSeconds(passwordEchoDurationInSecondsBackup);
        passwordEchoDurationInSecondsBackedUp = false;
    }

    if (passwordEchoEnabledBackedUp) {
        document->settings()->setPasswordEchoEnabled(passwordEchoEnabledBackup);
        passwordEchoEnabledBackedUp = false;
    }
}

}

