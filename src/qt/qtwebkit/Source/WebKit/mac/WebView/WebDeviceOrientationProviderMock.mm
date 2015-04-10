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

#import "WebDeviceOrientationProviderMockInternal.h"

#import "WebDeviceOrientationInternal.h"
#import <wtf/PassOwnPtr.h>

using namespace WebCore;

@implementation WebDeviceOrientationProviderMockInternal

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    m_core = adoptPtr(new DeviceOrientationClientMock());
    return self;
}

- (void)setOrientation:(WebDeviceOrientation*)orientation
{
    m_core->setOrientation(core(orientation));
}

- (void)setController:(DeviceOrientationController*)controller
{
    m_core->setController(controller);
}

- (void)startUpdating
{
    m_core->startUpdating();
}

- (void)stopUpdating
{
    m_core->stopUpdating();
}

- (WebDeviceOrientation*)lastOrientation
{
    return [[WebDeviceOrientation alloc] initWithCoreDeviceOrientation:m_core->lastOrientation()];
}

@end

@implementation WebDeviceOrientationProviderMock (Internal)

- (void)setController:(WebCore::DeviceOrientationController*)controller
{
   [m_internal setController:controller];
}

@end

@implementation WebDeviceOrientationProviderMock

+ (WebDeviceOrientationProviderMock *)shared
{
    static WebDeviceOrientationProviderMock *provider = [[WebDeviceOrientationProviderMock alloc] init];
    return provider;
}

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    m_internal = [[WebDeviceOrientationProviderMockInternal alloc] init];
    return self;
}

- (void)dealloc
{
    [super dealloc];
    [m_internal release];
}

- (void)setOrientation:(WebDeviceOrientation*)orientation
{
    [m_internal setOrientation:orientation];
}

- (void)startUpdating
{
    [m_internal startUpdating];
}

- (void)stopUpdating
{
    [m_internal stopUpdating];
}

- (WebDeviceOrientation*)lastOrientation
{
    return [m_internal lastOrientation];
}

@end
