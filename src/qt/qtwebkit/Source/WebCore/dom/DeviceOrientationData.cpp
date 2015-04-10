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
#include "DeviceOrientationData.h"

namespace WebCore {

PassRefPtr<DeviceOrientationData> DeviceOrientationData::create()
{
    return adoptRef(new DeviceOrientationData);
}

PassRefPtr<DeviceOrientationData> DeviceOrientationData::create(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma, bool canProvideAbsolute, bool absolute)
{
    return adoptRef(new DeviceOrientationData(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma, canProvideAbsolute, absolute));
}


DeviceOrientationData::DeviceOrientationData()
    : m_canProvideAlpha(false)
    , m_canProvideBeta(false)
    , m_canProvideGamma(false)
    , m_canProvideAbsolute(false)
    , m_alpha(0)
    , m_beta(0)
    , m_gamma(0)
    , m_absolute(false)
{
}

DeviceOrientationData::DeviceOrientationData(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma, bool canProvideAbsolute, bool absolute)
    : m_canProvideAlpha(canProvideAlpha)
    , m_canProvideBeta(canProvideBeta)
    , m_canProvideGamma(canProvideGamma)
    , m_canProvideAbsolute(canProvideAbsolute)
    , m_alpha(alpha)
    , m_beta(beta)
    , m_gamma(gamma)
    , m_absolute(absolute)
{
}

double DeviceOrientationData::alpha() const
{
    return m_alpha;
}

double DeviceOrientationData::beta() const
{
    return m_beta;
}

double DeviceOrientationData::gamma() const
{
    return m_gamma;
}

bool DeviceOrientationData::absolute() const
{
    return m_absolute;
}

bool DeviceOrientationData::canProvideAlpha() const
{
    return m_canProvideAlpha;
}

bool DeviceOrientationData::canProvideBeta() const
{
    return m_canProvideBeta;
}

bool DeviceOrientationData::canProvideGamma() const
{
    return m_canProvideGamma;
}

bool DeviceOrientationData::canProvideAbsolute() const
{
    return m_canProvideAbsolute;
}

} // namespace WebCore
