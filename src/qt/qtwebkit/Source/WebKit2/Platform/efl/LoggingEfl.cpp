/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

void initializeLogChannel(WTFLogChannel* channel)
{
    DEFINE_STATIC_LOCAL(Vector<WTFLogChannel*>, activatedChannels, ());

    // Fill activatedChannels vector only once based on names set in logValue.
    if (activatedChannels.isEmpty()) {
        const String logValue = getenv("WEBKIT_DEBUG");
        if (logValue.isEmpty())
            return;

        Vector<String> activatedNames;
        logValue.split(',', activatedNames);
        for (size_t i = 0; i < activatedNames.size(); i++) {
            WTFLogChannel* activeChannel = getChannelFromName(activatedNames[i]);
            if (activeChannel)
                activatedChannels.append(activeChannel);
        }
    }

    if (activatedChannels.contains(channel))
        channel->state = WTFLogChannelOn;
}

} // namespace WebKit

#endif // !LOG_DISABLED
