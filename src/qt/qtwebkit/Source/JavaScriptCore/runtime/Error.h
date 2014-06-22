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

#include "InternalFunction.h"
#include "Interpreter.h"
#include "JSObject.h"
#include <stdint.h>

namespace JSC {

    class ExecState;
    class VM;
    class JSGlobalObject;
    class JSObject;
    class SourceCode;
    class Structure;

    // Methods to create a range of internal errors.
    JSObject* createError(JSGlobalObject*, const String&);
    JSObject* createEvalError(JSGlobalObject*, const String&);
    JSObject* createRangeError(JSGlobalObject*, const String&);
    JSObject* createReferenceError(JSGlobalObject*, const String&);
    JSObject* createSyntaxError(JSGlobalObject*, const String&);
    JSObject* createTypeError(JSGlobalObject*, const String&);
    JSObject* createNotEnoughArgumentsError(JSGlobalObject*);
    JSObject* createURIError(JSGlobalObject*, const String&);
    // ExecState wrappers.
    JS_EXPORT_PRIVATE JSObject* createError(ExecState*, const String&);
    JSObject* createEvalError(ExecState*, const String&);
    JS_EXPORT_PRIVATE JSObject* createRangeError(ExecState*, const String&);
    JS_EXPORT_PRIVATE JSObject* createReferenceError(ExecState*, const String&);
    JS_EXPORT_PRIVATE JSObject* createSyntaxError(ExecState*, const String&);
    JS_EXPORT_PRIVATE JSObject* createTypeError(ExecState*, const String&);
    JS_EXPORT_PRIVATE JSObject* createNotEnoughArgumentsError(ExecState*);
    JSObject* createURIError(ExecState*, const String&);

    // Methods to add 
    bool hasErrorInfo(ExecState*, JSObject* error);
    // ExecState wrappers.
    JSObject* addErrorInfo(ExecState*, JSObject* error, int line, const SourceCode&);

    // Methods to throw Errors.
    JS_EXPORT_PRIVATE JSValue throwError(ExecState*, JSValue);
    JS_EXPORT_PRIVATE JSObject* throwError(ExecState*, JSObject*);

    // Convenience wrappers, create an throw an exception with a default message.
    JS_EXPORT_PRIVATE JSObject* throwTypeError(ExecState*);
    JS_EXPORT_PRIVATE JSObject* throwSyntaxError(ExecState*);

    // Convenience wrappers, wrap result as an EncodedJSValue.
    inline EncodedJSValue throwVMError(ExecState* exec, JSValue error) { return JSValue::encode(throwError(exec, error)); }
    inline EncodedJSValue throwVMTypeError(ExecState* exec) { return JSValue::encode(throwTypeError(exec)); }

    class StrictModeTypeErrorFunction : public InternalFunction {
    private:
        StrictModeTypeErrorFunction(JSGlobalObject* globalObject, Structure* structure, const String& message)
            : InternalFunction(globalObject, structure)
            , m_message(message)
        {
        }

        static void destroy(JSCell*);

    public:
        typedef InternalFunction Base;

        static StrictModeTypeErrorFunction* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, const String& message)
        {
            StrictModeTypeErrorFunction* function = new (NotNull, allocateCell<StrictModeTypeErrorFunction>(*exec->heap())) StrictModeTypeErrorFunction(globalObject, structure, message);
            function->finishCreation(exec->vm(), String());
            return function;
        }
    
        static EncodedJSValue JSC_HOST_CALL constructThrowTypeError(ExecState* exec)
        {
            throwTypeError(exec, static_cast<StrictModeTypeErrorFunction*>(exec->callee())->m_message);
            return JSValue::encode(jsNull());
        }
    
        static ConstructType getConstructData(JSCell*, ConstructData& constructData)
        {
            constructData.native.function = constructThrowTypeError;
            return ConstructTypeHost;
        }
    
        static EncodedJSValue JSC_HOST_CALL callThrowTypeError(ExecState* exec)
        {
            throwTypeError(exec, static_cast<StrictModeTypeErrorFunction*>(exec->callee())->m_message);
            return JSValue::encode(jsNull());
        }

        static CallType getCallData(JSCell*, CallData& callData)
        {
            callData.native.function = callThrowTypeError;
            return CallTypeHost;
        }

        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) 
        { 
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info); 
        }

    private:
        String m_message;
    };

} // namespace JSC

#endif // Error_h
