/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "InitializeLogging.h"
#include "Logging.h"

#if !LOG_DISABLED

#include <Eina.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

void initializeLoggingChannelsIfNecessary()
{
    static bool didInitializeLoggingChannels = false;
    if (didInitializeLoggingChannels)
        return;

    didInitializeLoggingChannels = true;

    char* logEnv = getenv("WEBKIT_DEBUG");
    if (!logEnv)
        return;

#if defined(NDEBUG)
    EINA_LOG_WARN("WEBKIT_DEBUG is not empty, but this is a release build. Notice that many log messages will only appear in a debug build.");
#endif

    char** logv = eina_str_split(logEnv, ",", -1);

    EINA_SAFETY_ON_NULL_RETURN(logv);

    for (int i = 0; logv[i]; i++) {
        if (WTFLogChannel* channel = getChannelFromName(logv[i]))
            channel->state = WTFLogChannelOn;
    }

    free(*logv);
    free(logv);

    // To disable logging notImplemented set the DISABLE_NI_WARNING
    // environment variable to 1.
    LogNotYetImplemented.state = WTFLogChannelOn;
}

}

#endif // !LOG_DISABLED
