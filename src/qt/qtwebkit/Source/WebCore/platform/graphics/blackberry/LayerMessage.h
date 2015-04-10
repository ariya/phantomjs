/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#ifndef LayerMessage_h
#define LayerMessage_h

#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformMessage.h>
#include <BlackBerryPlatformMessageClient.h>

typedef BlackBerry::Platform::Message PlatformMessage;

namespace WebCore {

inline bool isCompositingThread()
{
    return BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread();
}

inline pthread_t compositingThread()
{
    return BlackBerry::Platform::userInterfaceThreadMessageClient()->threadIdentifier();
}

inline bool isWebKitThread()
{
    return BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread();
}

inline pthread_t webKitThread()
{
    return BlackBerry::Platform::webKitThreadMessageClient()->threadIdentifier();
}

inline void dispatchCompositingMessage(PlatformMessage* msg)
{
    BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchMessage(msg);
}

inline void dispatchSyncCompositingMessage(PlatformMessage* msg)
{
    BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(msg);
}

enum PlatformMessagePriority { PlatformMessagePriorityLow = -1, PlatformMessagePriorityNormal = 0, PlatformMessagePriorityHigh = 1 };

inline void dispatchWebKitMessage(PlatformMessage* msg, PlatformMessagePriority priority = PlatformMessagePriorityNormal)
{
    if (priority != PlatformMessagePriorityNormal)
        msg->setPriority(BlackBerry::Platform::webKitThreadMessageClient()->threadPriority() + priority);
    BlackBerry::Platform::webKitThreadMessageClient()->dispatchMessage(msg);
}

} // namespace WebCore

#endif // LayerMessage_h
