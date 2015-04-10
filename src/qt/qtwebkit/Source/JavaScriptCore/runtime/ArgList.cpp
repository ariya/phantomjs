/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "ArgList.h"

#include "HeapRootVisitor.h"
#include "JSCJSValue.h"
#include "JSObject.h"
#include "Operations.h"

using std::min;

namespace JSC {

void ArgList::getSlice(int startIndex, ArgList& result) const
{
    if (startIndex <= 0 || startIndex >= m_argCount) {
        result = ArgList();
        return;
    }

    result.m_args = m_args - startIndex;
    result.m_argCount =  m_argCount - startIndex;
}

void MarkedArgumentBuffer::markLists(HeapRootVisitor& heapRootVisitor, ListSet& markSet)
{
    ListSet::iterator end = markSet.end();
    for (ListSet::iterator it = markSet.begin(); it != end; ++it) {
        MarkedArgumentBuffer* list = *it;
        for (int i = 0; i < list->m_size; ++i)
            heapRootVisitor.visit(reinterpret_cast<JSValue*>(&list->slotFor(i)));
    }
}

void MarkedArgumentBuffer::slowAppend(JSValue v)
{
    int newCapacity = m_capacity * 4;
    EncodedJSValue* newBuffer = &(new EncodedJSValue[newCapacity])[newCapacity - 1];
    for (int i = 0; i < m_capacity; ++i)
        newBuffer[-i] = m_buffer[-i];

    if (EncodedJSValue* base = mallocBase())
        delete [] base;

    m_buffer = newBuffer;
    m_capacity = newCapacity;

    slotFor(m_size) = JSValue::encode(v);
    ++m_size;

    if (m_markSet)
        return;

    // As long as our size stays within our Vector's inline 
    // capacity, all our values are allocated on the stack, and 
    // therefore don't need explicit marking. Once our size exceeds
    // our Vector's inline capacity, though, our values move to the 
    // heap, where they do need explicit marking.
    for (int i = 0; i < m_size; ++i) {
        Heap* heap = Heap::heap(JSValue::decode(slotFor(i)));
        if (!heap)
            continue;

        m_markSet = &heap->markListSet();
        m_markSet->add(this);
        break;
    }
}

} // namespace JSC
