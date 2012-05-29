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
#include <QVariant>

namespace JSC {

namespace Bindings {

class QtPixmapInstance : public Instance {
    QVariant data;
public:
    QtPixmapInstance(PassRefPtr<RootObject> rootObj, const QVariant& newData);
    virtual Class* getClass() const;
    virtual JSValue getMethod(ExecState* exec, const Identifier& propertyName);
    virtual JSValue invokeMethod(ExecState*, RuntimeMethod*);
    virtual void getPropertyNames(ExecState*, PropertyNameArray&);

    virtual JSValue defaultValue(ExecState*, PreferredPrimitiveType) const;
    virtual JSValue valueOf(ExecState* exec) const;
    int width() const;
    int height() const;
    QPixmap toPixmap();
    QImage toImage();
    RuntimeObject* newRuntimeObject(ExecState* exec);
    static JSObject* createPixmapRuntimeObject(ExecState*, PassRefPtr<RootObject>, const QVariant&);
    static QVariant variantFromObject(JSObject*, QMetaType::Type hint);
    static bool canHandle(QMetaType::Type hint);
};

}

}
#endif
