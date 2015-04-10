/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2012 ProFUSION embedded systems

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
#include "FrameNetworkingContextGtk.h"

#include "NetworkStorageSession.h"
#include "NotImplemented.h"

using namespace WebCore;

namespace WebKit {

NetworkStorageSession& FrameNetworkingContextGtk::storageSession() const
{
    return NetworkStorageSession::defaultStorageSession();
}

uint64_t FrameNetworkingContextGtk::initiatingPageID() const
{
    notImplemented();
    return 0;
}

}
