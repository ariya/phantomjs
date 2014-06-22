/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef qt_pixmapruntime_h
#define qt_pixmapruntime_h

#include "BridgeJSC.h"
#include "JavaScript.h"
#include <QVariant>

namespace JSC {

namespace Bindings {

class QtPixmapRuntime {
public:
    static JSObjectRef toJS(JSContextRef, const QVariant&, JSValueRef* exception);
    static QVariant toQt(JSContextRef, JSObjectRef, QMetaType::Type hint, JSValueRef* exception);
    static bool canHandle(QMetaType::Type hint);

    static JSClassRef getClassRef();
};

}

}
#endif
