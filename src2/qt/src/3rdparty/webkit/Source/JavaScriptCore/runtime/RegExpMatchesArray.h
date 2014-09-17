/*
 *  Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef RegExpMatchesArray_h
#define RegExpMatchesArray_h

#include "JSArray.h"

namespace JSC {

    class RegExpMatchesArray : public JSArray {
    public:
        RegExpMatchesArray(ExecState*, RegExpConstructorPrivate*);
        virtual ~RegExpMatchesArray();

    private:
        virtual bool getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return JSArray::getOwnPropertySlot(exec, propertyName, slot);
        }

        virtual bool getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return JSArray::getOwnPropertySlot(exec, propertyName, slot);
        }

        virtual bool getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return JSArray::getOwnPropertyDescriptor(exec, propertyName, descriptor);
        }

        virtual void put(ExecState* exec, const Identifier& propertyName, JSValue v, PutPropertySlot& slot)
        {
            if (subclassData())
                fillArrayInstance(exec);
            JSArray::put(exec, propertyName, v, slot);
        }

        virtual void put(ExecState* exec, unsigned propertyName, JSValue v)
        {
            if (subclassData())
                fillArrayInstance(exec);
            JSArray::put(exec, propertyName, v);
        }

        virtual bool deleteProperty(ExecState* exec, const Identifier& propertyName)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return JSArray::deleteProperty(exec, propertyName);
        }

        virtual bool deleteProperty(ExecState* exec, unsigned propertyName)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return JSArray::deleteProperty(exec, propertyName);
        }

        virtual void getOwnPropertyNames(ExecState* exec, PropertyNameArray& arr, EnumerationMode mode = ExcludeDontEnumProperties)
        {
            if (subclassData())
                fillArrayInstance(exec);
            JSArray::getOwnPropertyNames(exec, arr, mode);
        }

        void fillArrayInstance(ExecState*);
};

}

#endif // RegExpMatchesArray_h
