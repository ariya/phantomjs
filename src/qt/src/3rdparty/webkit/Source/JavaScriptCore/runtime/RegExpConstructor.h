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

#ifndef RegExpConstructor_h
#define RegExpConstructor_h

#include "InternalFunction.h"
#include "RegExp.h"
#include <wtf/OwnPtr.h>

namespace JSC {

    class RegExp;
    class RegExpPrototype;
    struct RegExpConstructorPrivate;

    struct RegExpConstructorPrivate {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        // Global search cache / settings
        RegExpConstructorPrivate()
            : lastNumSubPatterns(0)
            , multiline(false)
            , lastOvectorIndex(0)
        {
        }

        const Vector<int, 32>& lastOvector() const { return ovector[lastOvectorIndex]; }
        Vector<int, 32>& lastOvector() { return ovector[lastOvectorIndex]; }
        Vector<int, 32>& tempOvector() { return ovector[lastOvectorIndex ? 0 : 1]; }
        void changeLastOvector() { lastOvectorIndex = lastOvectorIndex ? 0 : 1; }

        UString input;
        UString lastInput;
        Vector<int, 32> ovector[2];
        unsigned lastNumSubPatterns : 30;
        bool multiline : 1;
        unsigned lastOvectorIndex : 1;
    };

    class RegExpConstructor : public InternalFunction {
    public:
        RegExpConstructor(ExecState*, JSGlobalObject*, Structure*, RegExpPrototype*);

        static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

        virtual void put(ExecState*, const Identifier& propertyName, JSValue, PutPropertySlot&);
        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);

        static const ClassInfo s_info;

        void performMatch(RegExp*, const UString&, int startOffset, int& position, int& length, int** ovector = 0);
        JSObject* arrayOfMatches(ExecState*) const;

        void setInput(const UString&);
        const UString& input() const;

        void setMultiline(bool);
        bool multiline() const;

        JSValue getBackref(ExecState*, unsigned) const;
        JSValue getLastParen(ExecState*) const;
        JSValue getLeftContext(ExecState*) const;
        JSValue getRightContext(ExecState*) const;

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | ImplementsHasInstance | InternalFunction::StructureFlags;

    private:
        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);

        OwnPtr<RegExpConstructorPrivate> d;
    };

    RegExpConstructor* asRegExpConstructor(JSValue);

    JSObject* constructRegExp(ExecState*, JSGlobalObject*, const ArgList&);

    inline RegExpConstructor* asRegExpConstructor(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&RegExpConstructor::s_info));
        return static_cast<RegExpConstructor*>(asObject(value));
    }

    /* 
      To facilitate result caching, exec(), test(), match(), search(), and replace() dipatch regular
      expression matching through the performMatch function. We use cached results to calculate, 
      e.g., RegExp.lastMatch and RegExp.leftParen.
    */
    ALWAYS_INLINE void RegExpConstructor::performMatch(RegExp* r, const UString& s, int startOffset, int& position, int& length, int** ovector)
    {
        position = r->match(s, startOffset, &d->tempOvector());

        if (ovector)
            *ovector = d->tempOvector().data();

        if (position != -1) {
            ASSERT(!d->tempOvector().isEmpty());

            length = d->tempOvector()[1] - d->tempOvector()[0];

            d->input = s;
            d->lastInput = s;
            d->changeLastOvector();
            d->lastNumSubPatterns = r->numSubpatterns();
        }
    }

} // namespace JSC

#endif // RegExpConstructor_h
