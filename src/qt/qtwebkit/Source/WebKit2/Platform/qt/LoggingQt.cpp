/*
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#include <QDebug>
#include <QStringList>

namespace WebKit {

#if !LOG_DISABLED

void initializeLogChannel(WTFLogChannel* channel)
{
    static Vector<WTFLogChannel*> activatedChannels;

    QByteArray loggingEnv = qgetenv("QT_WEBKIT_LOG");
    if (loggingEnv.isEmpty())
        return;

    // Fill activatedChannels vector only once based on names set in logValue.
    if (activatedChannels.isEmpty()) {
        QStringList channels = QString::fromLocal8Bit(loggingEnv).split(QLatin1String(","));
        for (int i = 0; i < channels.count(); i++) {
            if (WTFLogChannel* activeChannel = getChannelFromName(channels.at(i)))
                activatedChannels.append(activeChannel);
        }
    }

    if (activatedChannels.contains(channel))
        channel->state = WTFLogChannelOn;
}

#endif // !LOG_DISABLED

}
