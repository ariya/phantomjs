/*
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "Lookup.h"

#include "Executable.h"
#include "JSFunction.h"

namespace JSC {

void HashTable::createTable(JSGlobalData* globalData) const
{
    ASSERT(!table);
    int linkIndex = compactHashSizeMask + 1;
    HashEntry* entries = new HashEntry[compactSize];
    for (int i = 0; i < compactSize; ++i)
        entries[i].setKey(0);
    for (int i = 0; values[i].key; ++i) {
        StringImpl* identifier = Identifier::add(globalData, values[i].key).leakRef();
        int hashIndex = identifier->existingHash() & compactHashSizeMask;
        HashEntry* entry = &entries[hashIndex];

        if (entry->key()) {
            while (entry->next()) {
                entry = entry->next();
            }
            ASSERT(linkIndex < compactSize);
            entry->setNext(&entries[linkIndex++]);
            entry = entry->next();
        }

        entry->initialize(identifier, values[i].attributes, values[i].value1, values[i].value2
#if ENABLE(JIT)
                          , values[i].generator
#endif
                          );
    }
    table = entries;
}

void HashTable::deleteTable() const
{
    if (table) {
        int max = compactSize;
        for (int i = 0; i != max; ++i) {
            if (StringImpl* key = table[i].key())
                key->deref();
        }
        delete [] table;
        table = 0;
    }
}

void setUpStaticFunctionSlot(ExecState* exec, const HashEntry* entry, JSObject* thisObj, const Identifier& propertyName, PropertySlot& slot)
{
    ASSERT(thisObj->structure()->anonymousSlotCount() > 0);
    ASSERT(thisObj->getAnonymousValue(0).isCell() && asObject(thisObj->getAnonymousValue(0).asCell())->isGlobalObject());
    ASSERT(entry->attributes() & Function);
    WriteBarrierBase<Unknown>* location = thisObj->getDirectLocation(exec->globalData(), propertyName);

    if (!location) {
        JSFunction* function;
        JSGlobalObject* globalObject = asGlobalObject(thisObj->getAnonymousValue(0).asCell());
#if ENABLE(JIT) && ENABLE(JIT_OPTIMIZE_NATIVE_CALL)
        if (entry->generator())
            function = new (exec) JSFunction(exec, globalObject, globalObject->functionStructure(), entry->functionLength(), propertyName, exec->globalData().getHostFunction(entry->function(), entry->generator()));
        else
#endif
            function = new (exec) JSFunction(exec, globalObject, globalObject->functionStructure(), entry->functionLength(), propertyName, entry->function());

        thisObj->putDirectFunction(exec->globalData(), propertyName, function, entry->attributes());
        location = thisObj->getDirectLocation(exec->globalData(), propertyName);
    }

    slot.setValue(thisObj, location->get(), thisObj->offsetForLocation(location));
}

} // namespace JSC
