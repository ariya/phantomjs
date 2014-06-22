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

#import "WebDeviceOrientationInternal.h"

using namespace WebCore;

@implementation WebDeviceOrientationInternal

- (id)initWithCoreDeviceOrientation:(PassRefPtr<DeviceOrientationData>)coreDeviceOrientation
{
    self = [super init];
    if (!self)
        return nil;
    m_orientation = coreDeviceOrientation;
    return self;
}

@end

@implementation WebDeviceOrientation (Internal)

- (id)initWithCoreDeviceOrientation:(PassRefPtr<WebCore::DeviceOrientationData>)coreDeviceOrientation
{
    self = [super init];
    if (!self)
        return nil;
    m_internal = [[WebDeviceOrientationInternal alloc] initWithCoreDeviceOrientation:coreDeviceOrientation];
    return self;
}

@end

@implementation WebDeviceOrientation

DeviceOrientationData* core(WebDeviceOrientation* orientation)
{
    return orientation ? orientation->m_internal->m_orientation.get() : 0;
}

- (id)initWithCanProvideAlpha:(bool)canProvideAlpha alpha:(double)alpha canProvideBeta:(bool)canProvideBeta beta:(double)beta canProvideGamma:(bool)canProvideGamma gamma:(double)gamma
{
    self = [super init];
    if (!self)
        return nil;
    m_internal = [[WebDeviceOrientationInternal alloc] initWithCoreDeviceOrientation:DeviceOrientationData::create(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma)];
    return self;
}

- (void)dealloc
{
    [m_internal release];
    [super dealloc];
}

@end
