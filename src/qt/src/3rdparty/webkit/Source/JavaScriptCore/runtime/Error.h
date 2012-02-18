/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef Error_h
#define Error_h

#include "JSObject.h"
#include <stdint.h>

namespace JSC {

    class ExecState;
    class JSGlobalData;
    class JSGlobalObject;
    class JSObject;
    class SourceCode;
    class Structure;
    class UString;

    // Methods to create a range of internal errors.
    JSObject* createError(JSGlobalObject*, const UString&);
    JSObject* createEvalError(JSGlobalObject*, const UString&);
    JSObject* createRangeError(JSGlobalObject*, const UString&);
    JSObject* createReferenceError(JSGlobalObject*, const UString&);
    JSObject* createSyntaxError(JSGlobalObject*, const UString&);
    JSObject* createTypeError(JSGlobalObject*, const UString&);
    JSObject* createURIError(JSGlobalObject*, const UString&);
    // ExecState wrappers.
    JSObject* createError(ExecState*, const UString&);
    JSObject* createEvalError(ExecState*, const UString&);
    JSObject* createRangeError(ExecState*, const UString&);
    JSObject* createReferenceError(ExecState*, const UString&);
    JSObject* createSyntaxError(ExecState*, const UString&);
    JSObject* createTypeError(ExecState*, const UString&);
    JSObject* createURIError(ExecState*, const UString&);

    // Methods to add 
    bool hasErrorInfo(ExecState*, JSObject* error);
    JSObject* addErrorInfo(JSGlobalData*, JSObject* error, int line, const SourceCode&);
    // ExecState wrappers.
    JSObject* addErrorInfo(ExecState*, JSObject* error, int line, const SourceCode&);

    // Methods to throw Errors.
    JSValue throwError(ExecState*, JSValue);
    JSObject* throwError(ExecState*, JSObject*);

    // Convenience wrappers, create an throw an exception with a default message.
    JSObject* throwTypeError(ExecState*);
    JSObject* throwSyntaxError(ExecState*);

    // Convenience wrappers, wrap result as an EncodedJSValue.
    inline EncodedJSValue throwVMError(ExecState* exec, JSValue error) { return JSValue::encode(throwError(exec, error)); }
    inline EncodedJSValue throwVMTypeError(ExecState* exec) { return JSValue::encode(throwTypeError(exec)); }

    JSValue createTypeErrorFunction(ExecState* exec, const UString& message);
    
} // namespace JSC

#endif // Error_h
