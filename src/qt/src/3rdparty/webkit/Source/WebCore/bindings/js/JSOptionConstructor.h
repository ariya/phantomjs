/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef JSOptionConstructor_h
#define JSOptionConstructor_h

#include "JSDOMBinding.h"
#include "JSDocument.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class JSOptionConstructor : public DOMConstructorWithDocument {
    public:
        JSOptionConstructor(JSC::ExecState*, JSC::Structure*, JSDOMGlobalObject*);

        static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSValue prototype)
        {
            return JSC::Structure::create(globalData, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

        static const JSC::ClassInfo s_info;

    private:
        virtual JSC::ConstructType getConstructData(JSC::ConstructData&);
    };

} // namespace WebCore

#endif // JSOptionConstructor_h
