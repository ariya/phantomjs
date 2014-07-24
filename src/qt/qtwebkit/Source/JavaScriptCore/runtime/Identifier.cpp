/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2012 Apple Inc. All rights reserved.
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
#include "Identifier.h"

#include "CallFrame.h"
#include "JSObject.h"
#include "JSScope.h"
#include "NumericStrings.h"
#include "Operations.h"
#include <new>
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/FastMalloc.h>
#include <wtf/HashSet.h>
#include <wtf/text/ASCIIFastPath.h>
#include <wtf/text/StringHash.h>

using WTF::ThreadSpecific;

namespace JSC {

IdentifierTable* createIdentifierTable()
{
    return new IdentifierTable;
}

void deleteIdentifierTable(IdentifierTable* table)
{
    delete table;
}

struct IdentifierASCIIStringTranslator {
    static unsigned hash(const LChar* c)
    {
        return StringHasher::computeHashAndMaskTop8Bits(c);
    }

    static bool equal(StringImpl* r, const LChar* s)
    {
        return Identifier::equal(r, s);
    }

    static void translate(StringImpl*& location, const LChar* c, unsigned hash)
    {
        size_t length = strlen(reinterpret_cast<const char*>(c));
        location = StringImpl::createFromLiteral(reinterpret_cast<const char*>(c), length).leakRef();
        location->setHash(hash);
    }
};

struct IdentifierLCharFromUCharTranslator {
    static unsigned hash(const CharBuffer<UChar>& buf)
    {
        return StringHasher::computeHashAndMaskTop8Bits(buf.s, buf.length);
    }
    
    static bool equal(StringImpl* str, const CharBuffer<UChar>& buf)
    {
        return Identifier::equal(str, buf.s, buf.length);
    }
    
    static void translate(StringImpl*& location, const CharBuffer<UChar>& buf, unsigned hash)
    {
        LChar* d;
        StringImpl* r = StringImpl::createUninitialized(buf.length, d).leakRef();
        WTF::copyLCharsFromUCharSource(d, buf.s, buf.length);
        r->setHash(hash);
        location = r; 
    }
};

PassRefPtr<StringImpl> Identifier::add(VM* vm, const char* c)
{
    ASSERT(c);
    ASSERT(c[0]);
    if (!c[1])
        return add(vm, vm->smallStrings.singleCharacterStringRep(c[0]));

    IdentifierTable& identifierTable = *vm->identifierTable;

    HashSet<StringImpl*>::AddResult addResult = identifierTable.add<const LChar*, IdentifierASCIIStringTranslator>(reinterpret_cast<const LChar*>(c));

    // If the string is newly-translated, then we need to adopt it.
    // The boolean in the pair tells us if that is so.
    RefPtr<StringImpl> addedString = addResult.isNewEntry ? adoptRef(*addResult.iterator) : *addResult.iterator;

    return addedString.release();
}

PassRefPtr<StringImpl> Identifier::add(ExecState* exec, const char* c)
{
    return add(&exec->vm(), c);
}

PassRefPtr<StringImpl> Identifier::add8(VM* vm, const UChar* s, int length)
{
    if (length == 1) {
        UChar c = s[0];
        ASSERT(c <= 0xff);
        if (canUseSingleCharacterString(c))
            return add(vm, vm->smallStrings.singleCharacterStringRep(c));
    }
    
    if (!length)
        return StringImpl::empty();
    CharBuffer<UChar> buf = { s, static_cast<unsigned>(length) };
    HashSet<StringImpl*>::AddResult addResult = vm->identifierTable->add<CharBuffer<UChar>, IdentifierLCharFromUCharTranslator >(buf);
    
    // If the string is newly-translated, then we need to adopt it.
    // The boolean in the pair tells us if that is so.
    return addResult.isNewEntry ? adoptRef(*addResult.iterator) : *addResult.iterator;
}

PassRefPtr<StringImpl> Identifier::addSlowCase(VM* vm, StringImpl* r)
{
    ASSERT(!r->isIdentifier());
    // The empty & null strings are static singletons, and static strings are handled
    // in ::add() in the header, so we should never get here with a zero length string.
    ASSERT(r->length());

    if (r->length() == 1) {
        UChar c = (*r)[0];
        if (c <= maxSingleCharacterString)
            r = vm->smallStrings.singleCharacterStringRep(c);
            if (r->isIdentifier())
                return r;
    }

    return *vm->identifierTable->add(r).iterator;
}

PassRefPtr<StringImpl> Identifier::addSlowCase(ExecState* exec, StringImpl* r)
{
    return addSlowCase(&exec->vm(), r);
}

Identifier Identifier::from(ExecState* exec, unsigned value)
{
    return Identifier(exec, exec->vm().numericStrings.add(value));
}

Identifier Identifier::from(ExecState* exec, int value)
{
    return Identifier(exec, exec->vm().numericStrings.add(value));
}

Identifier Identifier::from(ExecState* exec, double value)
{
    return Identifier(exec, exec->vm().numericStrings.add(value));
}

Identifier Identifier::from(VM* vm, unsigned value)
{
    return Identifier(vm, vm->numericStrings.add(value));
}

Identifier Identifier::from(VM* vm, int value)
{
    return Identifier(vm, vm->numericStrings.add(value));
}

Identifier Identifier::from(VM* vm, double value)
{
    return Identifier(vm, vm->numericStrings.add(value));
}

#ifndef NDEBUG

void Identifier::checkCurrentIdentifierTable(VM* vm)
{
    // Check the identifier table accessible through the threadspecific matches the
    // vm's identifier table.
    ASSERT_UNUSED(vm, vm->identifierTable == wtfThreadData().currentIdentifierTable());
}

void Identifier::checkCurrentIdentifierTable(ExecState* exec)
{
    checkCurrentIdentifierTable(&exec->vm());
}

#else

// These only exists so that our exports are the same for debug and release builds.
// This would be an RELEASE_ASSERT_NOT_REACHED(), but we're in NDEBUG only code here!
NO_RETURN_DUE_TO_CRASH void Identifier::checkCurrentIdentifierTable(VM*) { CRASH(); }
NO_RETURN_DUE_TO_CRASH void Identifier::checkCurrentIdentifierTable(ExecState*) { CRASH(); }

#endif

} // namespace JSC
