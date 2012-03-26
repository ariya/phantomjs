/*
    Copyright (C) 2010 Robert Hogan <robert@roberthogan.net>

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

#ifndef QWebScriptWorld_h_
#define QWebScriptWorld_h_

#include <QtCore/qurl.h>
#include <QtCore/qshareddata.h>

#include "qwebkitglobal.h"

namespace WebCore {
    class DOMWrapperWorld;
}

class QWebScriptWorldPrivate;
class QWebFrame;

class QWEBKIT_EXPORT QWebScriptWorld {
public:
    QWebScriptWorld();
    QWebScriptWorld(const QWebScriptWorld&);
    QWebScriptWorld &operator=(const QWebScriptWorld&);
    ~QWebScriptWorld();

    WebCore::DOMWrapperWorld* world() const;

private:
    QExplicitlySharedDataPointer<QWebScriptWorldPrivate> d;

    friend class QWebFrame;
};

#endif
