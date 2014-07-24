/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "Logging.h"

#include "InitializeLogging.h"

#if !LOG_DISABLED

#include <wtf/text/WTFString.h>

namespace WebCore {

static inline void initializeWithUserDefault(WTFLogChannel& channel, bool enabled)
{
    if (enabled)
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

    String logEnv = getenv("WEBKIT_DEBUG");
    if (logEnv.isEmpty())
        return;

    Vector<String> logv;
    logEnv.split(" ", logv);

    Vector<String>::const_iterator it = logv.begin();
    for (; it != logv.end(); ++it) {
        if (WTFLogChannel* channel = getChannelFromName(*it))
            channel->state = WTFLogChannelOn;
    }
}

} // namespace WebCore

#endif // !LOG_DISABLED
