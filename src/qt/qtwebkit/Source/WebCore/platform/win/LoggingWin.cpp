/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#include "config.h"
#include "InitializeLogging.h"
#include "Logging.h"

#if !LOG_DISABLED

#include <windows.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static inline void initializeWithUserDefault(WTFLogChannel& channel)
{
    DWORD length = GetEnvironmentVariableA(channel.defaultName, 0, 0);
    if (!length)
        return;

    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[length]);

    if (!GetEnvironmentVariableA(channel.defaultName, buffer.get(), length))
        return;

    String variableValue(buffer.get());

    static const String& hexadecimalPrefix = *new String("0x");
    if (variableValue.length() < 3 || !variableValue.startsWith(hexadecimalPrefix, false)) {
        LOG_ERROR("Unable to parse hex value for %s (%s), logging is off", channel.defaultName, buffer.get());
        return;
    }

    String unprefixedValue = variableValue.substring(2);

    // Now parse the unprefixed string as a hexadecimal number.
    bool parsedSuccessfully = false;
    unsigned logLevel = unprefixedValue.toUIntStrict(&parsedSuccessfully, 16);

    if (!parsedSuccessfully) {
        LOG_ERROR("Unable to parse hex value for %s (%s), logging is off", channel.defaultName, buffer.get());
        return;
    }

    if ((logLevel & channel.mask) == channel.mask)
        channel.state = WTFLogChannelOn;
    else
        channel.state = WTFLogChannelOff;
}

void initializeLoggingChannelsIfNecessary()
{
    static bool haveInitializedLoggingChannels = false;
    if (haveInitializedLoggingChannels)
        return;
    haveInitializedLoggingChannels = true;

    initializeWithUserDefault(LogNotYetImplemented);
    initializeWithUserDefault(LogFrames);
    initializeWithUserDefault(LogLoading);
    initializeWithUserDefault(LogPopupBlocking);
    initializeWithUserDefault(LogEvents);
    initializeWithUserDefault(LogEditing);
    initializeWithUserDefault(LogLiveConnect);
    initializeWithUserDefault(LogIconDatabase);
    initializeWithUserDefault(LogSQLDatabase);
    initializeWithUserDefault(LogSpellingAndGrammar);
    initializeWithUserDefault(LogBackForward);
    initializeWithUserDefault(LogHistory);
    initializeWithUserDefault(LogPageCache);
    initializeWithUserDefault(LogPlatformLeaks);
    initializeWithUserDefault(LogResourceLoading);
    initializeWithUserDefault(LogAnimations);
    initializeWithUserDefault(LogNetwork);
    initializeWithUserDefault(LogFTP);
    initializeWithUserDefault(LogThreading);
    initializeWithUserDefault(LogStorageAPI);
    initializeWithUserDefault(LogMedia);
    initializeWithUserDefault(LogPlugins);
    initializeWithUserDefault(LogArchives);
    initializeWithUserDefault(LogProgress);
    initializeWithUserDefault(LogFileAPI);
}

} // namespace WebCore

#endif // !LOG_DISABLED
