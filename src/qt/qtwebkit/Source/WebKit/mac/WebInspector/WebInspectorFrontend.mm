/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "WebInspectorFrontend.h"

#import "WebInspectorClient.h"
#import <WebCore/InspectorFrontendClient.h>

using namespace WebCore;

@implementation WebInspectorFrontend

- (id)initWithFrontendClient:(WebInspectorFrontendClient *)frontendClient
{
    if (!(self = [super init]))
        return nil;

    m_frontendClient = frontendClient;
    return self;
}

- (void)attach
{
    m_frontendClient->attachWindow(InspectorFrontendClient::DOCKED_TO_BOTTOM);
}

- (void)detach
{
    m_frontendClient->detachWindow();
}

- (BOOL)isDebuggingEnabled
{
    return m_frontendClient->isDebuggingEnabled();
}

- (void)setDebuggingEnabled:(BOOL)enabled
{
    m_frontendClient->setDebuggingEnabled(enabled);
}

- (BOOL)isProfilingJavaScript
{
    return m_frontendClient->isProfilingJavaScript();
}

- (void)startProfilingJavaScript
{
    m_frontendClient->startProfilingJavaScript();
}

- (void)stopProfilingJavaScript
{
    m_frontendClient->stopProfilingJavaScript();
}

- (BOOL)isTimelineProfilingEnabled
{
    return m_frontendClient->isTimelineProfilingEnabled();
}

- (void)setTimelineProfilingEnabled:(BOOL)enabled
{
    m_frontendClient->setTimelineProfilingEnabled(enabled);
}

- (void)showConsole
{
    m_frontendClient->showConsole();
}

@end
