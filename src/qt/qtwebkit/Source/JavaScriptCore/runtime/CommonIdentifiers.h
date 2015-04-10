/*
 *  Copyright (C) 2003, 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef CommonIdentifiers_h
#define CommonIdentifiers_h

#include "Identifier.h"
#include <wtf/Noncopyable.h>

// MarkedArgumentBuffer of property names, passed to a macro so we can do set them up various
// ways without repeating the list.
#define JSC_COMMON_IDENTIFIERS_EACH_PROPERTY_NAME(macro) \
    macro(Array) \
    macro(Boolean) \
    macro(Date) \
    macro(Error) \
    macro(EvalError) \
    macro(Function) \
    macro(Infinity) \
    macro(JSON) \
    macro(Math) \
    macro(NaN) \
    macro(Number) \
    macro(Object) \
    macro(RangeError) \
    macro(ReferenceError) \
    macro(RegExp) \
    macro(String) \
    macro(SyntaxError) \
    macro(TypeError) \
    macro(URIError) \
    macro(UTC) \
    macro(__defineGetter__) \
    macro(__defineSetter__) \
    macro(__lookupGetter__) \
    macro(__lookupSetter__) \
    macro(anonymous) \
    macro(apply) \
    macro(arguments) \
    macro(bind) \
    macro(bytecode) \
    macro(bytecodeIndex) \
    macro(bytecodes) \
    macro(bytecodesID) \
    macro(call) \
    macro(callee) \
    macro(caller) \
    macro(compilationKind) \
    macro(compilations) \
    macro(compile) \
    macro(configurable) \
    macro(constructor) \
    macro(count) \
    macro(counters) \
    macro(description) \
    macro(descriptions) \
    macro(displayName) \
    macro(document) \
    macro(enumerable) \
    macro(eval) \
    macro(exec) \
    macro(executionCount) \
    macro(exitKind) \
    macro(fromCharCode) \
    macro(get) \
    macro(global) \
    macro(hasOwnProperty) \
    macro(hash) \
    macro(header) \
    macro(id) \
    macro(ignoreCase) \
    macro(index) \
    macro(inferredName) \
    macro(input) \
    macro(instructionCount) \
    macro(isArray) \
    macro(isPrototypeOf) \
    macro(isWatchpoint) \
    macro(join) \
    macro(lastIndex) \
    macro(length) \
    macro(message) \
    macro(multiline) \
    macro(name) \
    macro(now) \
    macro(numInlinedCalls) \
    macro(numInlinedGetByIds) \
    macro(numInlinedPutByIds) \
    macro(opcode) \
    macro(origin) \
    macro(osrExitSites) \
    macro(osrExits) \
    macro(parse) \
    macro(profiledBytecodes) \
    macro(propertyIsEnumerable) \
    macro(prototype) \
    macro(set) \
    macro(source) \
    macro(sourceCode) \
    macro(stack) \
    macro(test) \
    macro(toExponential) \
    macro(toFixed) \
    macro(toISOString) \
    macro(toJSON) \
    macro(toLocaleString) \
    macro(toPrecision) \
    macro(toString) \
    macro(value) \
    macro(valueOf) \
    macro(window) \
    macro(writable)

#define JSC_COMMON_IDENTIFIERS_EACH_KEYWORD(macro) \
    macro(null) \
    macro(undefined) \
    macro(true) \
    macro(false) \
    macro(break) \
    macro(case) \
    macro(catch) \
    macro(const) \
    macro(default) \
    macro(finally) \
    macro(for) \
    macro(instanceof) \
    macro(new) \
    macro(var) \
    macro(continue) \
    macro(function) \
    macro(return) \
    macro(void) \
    macro(delete) \
    macro(if) \
    macro(this) \
    macro(do) \
    macro(while) \
    macro(else) \
    macro(in) \
    macro(switch) \
    macro(throw) \
    macro(try) \
    macro(typeof) \
    macro(with) \
    macro(debugger) \
    macro(class) \
    macro(enum) \
    macro(export) \
    macro(extends) \
    macro(import) \
    macro(super) \
    macro(implements) \
    macro(interface) \
    macro(let) \
    macro(package) \
    macro(private) \
    macro(protected) \
    macro(public) \
    macro(static) \
    macro(yield)

namespace JSC {

    class CommonIdentifiers {
        WTF_MAKE_NONCOPYABLE(CommonIdentifiers); WTF_MAKE_FAST_ALLOCATED;
    private:
        CommonIdentifiers(VM*);
        friend class VM;

    public:
        const Identifier nullIdentifier;
        const Identifier emptyIdentifier;
        const Identifier underscoreProto;
        const Identifier thisIdentifier;
        const Identifier useStrictIdentifier;

        
#define JSC_IDENTIFIER_DECLARE_KEYWORD_NAME_GLOBAL(name) const Identifier name##Keyword;
        JSC_COMMON_IDENTIFIERS_EACH_KEYWORD(JSC_IDENTIFIER_DECLARE_KEYWORD_NAME_GLOBAL)
#undef JSC_IDENTIFIER_DECLARE_KEYWORD_NAME_GLOBAL
        
#define JSC_IDENTIFIER_DECLARE_PROPERTY_NAME_GLOBAL(name) const Identifier name;
        JSC_COMMON_IDENTIFIERS_EACH_PROPERTY_NAME(JSC_IDENTIFIER_DECLARE_PROPERTY_NAME_GLOBAL)
#undef JSC_IDENTIFIER_DECLARE_PROPERTY_NAME_GLOBAL
    };

} // namespace JSC

#endif // CommonIdentifiers_h
