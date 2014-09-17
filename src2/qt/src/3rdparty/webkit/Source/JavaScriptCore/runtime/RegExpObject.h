/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
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

#ifndef RegExpObject_h
#define RegExpObject_h

#include "JSObjectWithGlobalObject.h"
#include "RegExp.h"

namespace JSC {

    class RegExpObject : public JSObjectWithGlobalObject {
    public:
        typedef JSObjectWithGlobalObject Base;

        RegExpObject(JSGlobalObject*, Structure*, NonNullPassRefPtr<RegExp>);
        virtual ~RegExpObject();

        void setRegExp(PassRefPtr<RegExp> r) { d->regExp = r; }
        RegExp* regExp() const { return d->regExp.get(); }

        void setLastIndex(size_t lastIndex)
        {
            d->lastIndex.setWithoutWriteBarrier(jsNumber(lastIndex));
        }
        void setLastIndex(JSGlobalData& globalData, JSValue lastIndex)
        {
            d->lastIndex.set(globalData, this, lastIndex);
        }
        JSValue getLastIndex() const
        {
            return d->lastIndex.get();
        }

        JSValue test(ExecState*);
        JSValue exec(ExecState*);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
        virtual void put(ExecState*, const Identifier& propertyName, JSValue, PutPropertySlot&);

        static JS_EXPORTDATA const ClassInfo s_info;

        static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

    protected:
        static const unsigned StructureFlags = OverridesVisitChildren | OverridesGetOwnPropertySlot | JSObjectWithGlobalObject::StructureFlags;

    private:
        virtual void visitChildren(SlotVisitor&);

        bool match(ExecState*);

        struct RegExpObjectData {
            WTF_MAKE_FAST_ALLOCATED;
        public:
            RegExpObjectData(NonNullPassRefPtr<RegExp> regExp)
                : regExp(regExp)
            {
                lastIndex.setWithoutWriteBarrier(jsNumber(0));
            }

            RefPtr<RegExp> regExp;
            WriteBarrier<Unknown> lastIndex;
        };
#if COMPILER(MSVC)
        friend void WTF::deleteOwnedPtr<RegExpObjectData>(RegExpObjectData*);
#endif
        OwnPtr<RegExpObjectData> d;
    };

    RegExpObject* asRegExpObject(JSValue);

    inline RegExpObject* asRegExpObject(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&RegExpObject::s_info));
        return static_cast<RegExpObject*>(asObject(value));
    }

} // namespace JSC

#endif // RegExpObject_h
