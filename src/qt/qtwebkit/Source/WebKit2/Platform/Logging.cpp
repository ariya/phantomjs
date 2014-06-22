/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Logging.h"

#if !LOG_DISABLED

namespace WebKit {

WTFLogChannel LogSessionState      = { 0x00000001, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogContextMenu       = { 0x00000002, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogTextInput         = { 0x00000004, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogView              = { 0x00000008, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogIconDatabase      = { 0x00000010, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogKeyHandling       = { 0x00000020, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogPlugins           = { 0x00000040, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogNetwork           = { 0x00000080, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogNetworkScheduling = { 0x00000100, "WebKit2LogLevel", WTFLogChannelOff };
WTFLogChannel LogInspectorServer   = { 0x00000200, "WebKit2LogLevel", WTFLogChannelOff };

#if !PLATFORM(MAC) && !PLATFORM(GTK) && !PLATFORM(QT) && !PLATFORM(EFL)
void initializeLogChannel(WTFLogChannel* channel)
{
    // FIXME: Each platform will need to define their own initializeLogChannel().
}
#endif

#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
WTFLogChannel* getChannelFromName(const String& channelName)
{
    if (!(channelName.length() >= 2))
        return 0;

    if (equalIgnoringCase(channelName, String("SessionState")))
        return &LogSessionState;

    if (equalIgnoringCase(channelName, String("ContextMenu")))
        return &LogContextMenu;

    if (equalIgnoringCase(channelName, String("TextInput")))
        return &LogTextInput;

    if (equalIgnoringCase(channelName, String("View")))
        return &LogView;

    if (equalIgnoringCase(channelName, String("IconDatabase")))
        return &LogIconDatabase;

    if (equalIgnoringCase(channelName, String("KeyHandling")))
        return &LogKeyHandling;

    if (equalIgnoringCase(channelName, String("Plugins")))
        return &LogPlugins;

    if (equalIgnoringCase(channelName, String("Network")))
        return &LogNetwork;

    if (equalIgnoringCase(channelName, String("InspectorServer")))
        return &LogInspectorServer;

    return 0;
}
#endif

void initializeLogChannelsIfNecessary()
{
    static bool haveInitializedLogChannels = false;
    if (haveInitializedLogChannels)
        return;
    haveInitializedLogChannels = true;

    initializeLogChannel(&LogSessionState);
    initializeLogChannel(&LogContextMenu);
    initializeLogChannel(&LogTextInput);
    initializeLogChannel(&LogView);
    initializeLogChannel(&LogIconDatabase);
    initializeLogChannel(&LogKeyHandling);
    initializeLogChannel(&LogPlugins);
    initializeLogChannel(&LogNetwork);
    initializeLogChannel(&LogNetworkScheduling);
    initializeLogChannel(&LogInspectorServer);
}

} // namespace WebKit

#endif // !LOG_DISABLED
