/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef InternalFunction_h
#define InternalFunction_h

#include "JSObjectWithGlobalObject.h"
#include "Identifier.h"

namespace JSC {

    class FunctionPrototype;

    class InternalFunction : public JSObjectWithGlobalObject {
    public:
        static JS_EXPORTDATA const ClassInfo s_info;

        const UString& name(ExecState*);
        const UString displayName(ExecState*);
        const UString calculatedDisplayName(ExecState*);

        static Structure* createStructure(JSGlobalData& globalData, JSValue proto) 
        { 
            return Structure::create(globalData, proto, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info); 
        }

    protected:
        static const unsigned StructureFlags = ImplementsHasInstance | JSObject::StructureFlags;

        // Only used to allow us to determine the JSFunction vptr
        InternalFunction(VPtrStealingHackType);

        InternalFunction(JSGlobalData*, JSGlobalObject*, Structure*, const Identifier&);

    private:
        virtual CallType getCallData(CallData&) = 0;

        virtual void vtableAnchor();
    };

    InternalFunction* asInternalFunction(JSValue);

    inline InternalFunction* asInternalFunction(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&InternalFunction::s_info));
        return static_cast<InternalFunction*>(asObject(value));
    }

} // namespace JSC

#endif // InternalFunction_h
