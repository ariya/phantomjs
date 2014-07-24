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

#ifndef NavigatorMediaStream_h
#define NavigatorMediaStream_h

#if ENABLE(MEDIA_STREAM)

#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Dictionary;
class Navigator;
class NavigatorUserMediaErrorCallback;
class NavigatorUserMediaSuccessCallback;

typedef int ExceptionCode;

class NavigatorMediaStream {
public:
    static void webkitGetUserMedia(Navigator*, const Dictionary&, PassRefPtr<NavigatorUserMediaSuccessCallback>, PassRefPtr<NavigatorUserMediaErrorCallback>, ExceptionCode&);

private:
    NavigatorMediaStream();
    ~NavigatorMediaStream();
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // NavigatorMediaStream_h
