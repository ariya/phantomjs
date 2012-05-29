/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
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
 */

#include "config.h"
#include "Logging.h"
#include "PlatformString.h"

namespace WebCore {

WTFLogChannel LogNotYetImplemented = { 0x00000001, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel LogFrames =            { 0x00000010, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogLoading =           { 0x00000020, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogPopupBlocking =     { 0x00000040, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogEvents =            { 0x00000080, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel LogEditing =           { 0x00000100, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogLiveConnect =       { 0x00000200, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogIconDatabase =      { 0x00000400, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogSQLDatabase =       { 0x00000800, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel LogSpellingAndGrammar ={ 0x00001000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogBackForward =       { 0x00002000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogHistory =           { 0x00004000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogPageCache =         { 0x00008000, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel LogPlatformLeaks =     { 0x00010000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogResourceLoading =   { 0x00020000, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel LogNetwork =           { 0x00100000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogFTP =               { 0x00200000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogThreading =         { 0x00400000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogStorageAPI =        { 0x00800000, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel LogMedia =             { 0x01000000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogPlugins =           { 0x02000000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogArchives =          { 0x04000000, "WebCoreLogLevel", WTFLogChannelOff };
WTFLogChannel LogProgress =          { 0x08000000, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel LogFileAPI =           { 0x10000000, "WebCoreLogLevel", WTFLogChannelOff };

WTFLogChannel* getChannelFromName(const String& channelName)
{
    if (!(channelName.length() >= 2))
        return 0;

    if (equalIgnoringCase(channelName, String("BackForward")))
        return &LogBackForward;

    if (equalIgnoringCase(channelName, String("Editing")))
        return &LogEditing;

    if (equalIgnoringCase(channelName, String("Events")))
        return &LogEvents;

    if (equalIgnoringCase(channelName, String("Frames")))
        return &LogFrames;

    if (equalIgnoringCase(channelName, String("FTP")))
        return &LogFTP;

    if (equalIgnoringCase(channelName, String("History")))
        return &LogHistory;

    if (equalIgnoringCase(channelName, String("IconDatabase")))
        return &LogIconDatabase;

    if (equalIgnoringCase(channelName, String("Loading")))
        return &LogLoading;

    if (equalIgnoringCase(channelName, String("Media")))
        return &LogMedia;

    if (equalIgnoringCase(channelName, String("Network")))
        return &LogNetwork;

    if (equalIgnoringCase(channelName, String("NotYetImplemented")))
        return &LogNotYetImplemented;

    if (equalIgnoringCase(channelName, String("PageCache")))
        return &LogPageCache;

    if (equalIgnoringCase(channelName, String("PlatformLeaks")))
        return &LogPlatformLeaks;

    if (equalIgnoringCase(channelName, String("ResourceLoading")))
        return &LogResourceLoading;

    if (equalIgnoringCase(channelName, String("Plugins")))
        return &LogPlugins;

    if (equalIgnoringCase(channelName, String("PopupBlocking")))
        return &LogPopupBlocking;

    if (equalIgnoringCase(channelName, String("Progress")))
        return &LogProgress;

    if (equalIgnoringCase(channelName, String("SpellingAndGrammar")))
        return &LogSpellingAndGrammar;

    if (equalIgnoringCase(channelName, String("SQLDatabase")))
        return &LogSQLDatabase;

    if (equalIgnoringCase(channelName, String("StorageAPI")))
        return &LogStorageAPI;

    if (equalIgnoringCase(channelName, String("LiveConnect")))
        return &LogLiveConnect;

    if (equalIgnoringCase(channelName, String("Threading")))
        return &LogThreading;

    if (equalIgnoringCase(channelName, String("FileAPI")))
        return &LogFileAPI;

    return 0;
}

}
