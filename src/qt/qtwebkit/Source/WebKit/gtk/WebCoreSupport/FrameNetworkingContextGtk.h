/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef FrameNetworkingContextGtk_h
#define FrameNetworkingContextGtk_h

#include "FrameNetworkingContext.h"

namespace WebKit {

class FrameNetworkingContextGtk : public WebCore::FrameNetworkingContext {
public:
    static PassRefPtr<FrameNetworkingContextGtk> create(WebCore::Frame* frame)
    {
        return adoptRef(new FrameNetworkingContextGtk(frame));
    }

    WebCore::Frame* coreFrame() const { return frame(); }
    virtual uint64_t initiatingPageID() const;

private:
    virtual WebCore::NetworkStorageSession& storageSession() const;

    FrameNetworkingContextGtk(WebCore::Frame* frame)
        : WebCore::FrameNetworkingContext(frame)
    {
    }
};

}

#endif
