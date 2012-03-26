/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "Logging.h"

#include "PlatformString.h"
#include <QDebug>
#include <QStringList>

namespace WebCore {

void InitializeLoggingChannelsIfNecessary()
{
    static bool haveInitializedLoggingChannels = false;
    if (haveInitializedLoggingChannels)
        return;

    haveInitializedLoggingChannels = true;

    QByteArray loggingEnv = qgetenv("QT_WEBKIT_LOG");
    if (loggingEnv.isEmpty())
        return;

#if defined(NDEBUG)
    qWarning("This is a release build. Setting QT_WEBKIT_LOG will have no effect.");
#else
    QStringList channels = QString::fromLocal8Bit(loggingEnv).split(QLatin1String(","));
    for (int i = 0; i < channels.count(); i++) {
        if (WTFLogChannel* channel = getChannelFromName(channels.at(i)))
            channel->state = WTFLogChannelOn;
    }

    // By default we log calls to notImplemented(). This can be turned
    // off by setting the environment variable DISABLE_NI_WARNING to 1
    LogNotYetImplemented.state = WTFLogChannelOn;
#endif
}

} // namespace WebCore
