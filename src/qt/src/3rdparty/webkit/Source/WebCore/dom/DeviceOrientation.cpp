/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DeviceOrientation.h"

namespace WebCore {

PassRefPtr<DeviceOrientation> DeviceOrientation::create()
{
    return adoptRef(new DeviceOrientation);
}

PassRefPtr<DeviceOrientation> DeviceOrientation::create(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    return adoptRef(new DeviceOrientation(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma));
}


DeviceOrientation::DeviceOrientation()
    : m_canProvideAlpha(false)
    , m_canProvideBeta(false)
    , m_canProvideGamma(false)
{
}

DeviceOrientation::DeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
    : m_canProvideAlpha(canProvideAlpha)
    , m_canProvideBeta(canProvideBeta)
    , m_canProvideGamma(canProvideGamma)
    , m_alpha(alpha)
    , m_beta(beta)
    , m_gamma(gamma)
{
}

double DeviceOrientation::alpha() const
{
    return m_alpha;
}

double DeviceOrientation::beta() const
{
    return m_beta;
}

double DeviceOrientation::gamma() const
{
    return m_gamma;
}

bool DeviceOrientation::canProvideAlpha() const
{
    return m_canProvideAlpha;
}

bool DeviceOrientation::canProvideBeta() const
{
    return m_canProvideBeta;
}

bool DeviceOrientation::canProvideGamma() const
{
    return m_canProvideGamma;
}

} // namespace WebCore
