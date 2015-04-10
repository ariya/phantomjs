/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "JavaScriptCore.h"
#include "JSBasePrivate.h"
#include "JSContextRefPrivate.h"
#include "JSObjectRefPrivate.h"
#include "JSScriptRefPrivate.h"
#include "JSStringRefPrivate.h"
#include <math.h>
#define ASSERT_DISABLED 0
#include <wtf/Assertions.h>

#if PLATFORM(MAC) || PLATFORM(IOS)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/time.h>
#endif

#if OS(WINDOWS)
#include <windows.h>
#endif

#if COMPILER(MSVC)

#include <wtf/MathExtras.h>

static double nan(const char*)
{
    return std::numeric_limits<double>::quiet_NaN();
}

using std::isinf;
using std::isnan;

#endif

#if JSC_OBJC_API_ENABLED
void testObjectiveCAPI(void);
#endif

extern void JSSynchronousGarbageCollectForDebugging(JSContextRef);

static JSGlobalContextRef context;
int failed;
static void assertEqualsAsBoolean(JSValueRef value, bool expectedValue)
{
    if (JSValueToBoolean(context, value) != expectedValue) {
        fprintf(stderr, "assertEqualsAsBoolean failed: %p, %d\n", value, expectedValue);
        failed = 1;
    }
}

static void assertEqualsAsNumber(JSValueRef value, double expectedValue)
{
    double number = JSValueToNumber(context, value, NULL);

    // FIXME <rdar://4668451> - On i386 the isnan(double) macro tries to map to the isnan(float) function,
    // causing a build break with -Wshorten-64-to-32 enabled.  The issue is known by the appropriate team.
    // After that's resolved, we can remove these casts
    if (number != expectedValue && !(isnan((float)number) && isnan((float)expectedValue))) {
        fprintf(stderr, "assertEqualsAsNumber failed: %p, %lf\n", value, expectedValue);
        failed = 1;
    }
}

static void assertEqualsAsUTF8String(JSValueRef value, const char* expectedValue)
{
    JSStringRef valueAsString = JSValueToStringCopy(context, value, NULL);

    size_t jsSize = JSStringGetMaximumUTF8CStringSize(valueAsString);
    char* jsBuffer = (char*)malloc(jsSize);
    JSStringGetUTF8CString(valueAsString, jsBuffer, jsSize);
    
    unsigned i;
    for (i = 0; jsBuffer[i]; i++) {
        if (jsBuffer[i] != expectedValue[i]) {
            fprintf(stderr, "assertEqualsAsUTF8String failed at character %d: %c(%d) != %c(%d)\n", i, jsBuffer[i], jsBuffer[i], expectedValue[i], expectedValue[i]);
            failed = 1;
        }
    }

    if (jsSize < strlen(jsBuffer) + 1) {
        fprintf(stderr, "assertEqualsAsUTF8String failed: jsSize was too small\n");
        failed = 1;
    }

    free(jsBuffer);
    JSStringRelease(valueAsString);
}

static void assertEqualsAsCharactersPtr(JSValueRef value, const char* expectedValue)
{
    JSStringRef valueAsString = JSValueToStringCopy(context, value, NULL);

    size_t jsLength = JSStringGetLength(valueAsString);
    const JSChar* jsBuffer = JSStringGetCharactersPtr(valueAsString);

    CFStringRef expectedValueAsCFString = CFStringCreateWithCString(kCFAllocatorDefault, 
                                                                    expectedValue,
                                                                    kCFStringEncodingUTF8);    
    CFIndex cfLength = CFStringGetLength(expectedValueAsCFString);
    UniChar* cfBuffer = (UniChar*)malloc(cfLength * sizeof(UniChar));
    CFStringGetCharacters(expectedValueAsCFString, CFRangeMake(0, cfLength), cfBuffer);
    CFRelease(expectedValueAsCFString);

    if (memcmp(jsBuffer, cfBuffer, cfLength * sizeof(UniChar)) != 0) {
        fprintf(stderr, "assertEqualsAsCharactersPtr failed: jsBuffer != cfBuffer\n");
        failed = 1;
    }
    
    if (jsLength != (size_t)cfLength) {
        fprintf(stderr, "assertEqualsAsCharactersPtr failed: jsLength(%ld) != cfLength(%ld)\n", jsLength, cfLength);
        failed = 1;
    }

    free(cfBuffer);
    JSStringRelease(valueAsString);
}

static bool timeZoneIsPST()
{
    char timeZoneName[70];
    struct tm gtm;
    memset(&gtm, 0, sizeof(gtm));
    strftime(timeZoneName, sizeof(timeZoneName), "%Z", &gtm);

    return 0 == strcmp("PST", timeZoneName);
}

static JSValueRef jsGlobalValue; // non-stack value for testing JSValueProtect()

/* MyObject pseudo-class */

static bool MyObject_hasProperty(JSContextRef context, JSObjectRef object, JSStringRef propertyName)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);

    if (JSStringIsEqualToUTF8CString(propertyName, "alwaysOne")
        || JSStringIsEqualToUTF8CString(propertyName, "cantFind")
        || JSStringIsEqualToUTF8CString(propertyName, "throwOnGet")
        || JSStringIsEqualToUTF8CString(propertyName, "myPropertyName")
        || JSStringIsEqualToUTF8CString(propertyName, "hasPropertyLie")
        || JSStringIsEqualToUTF8CString(propertyName, "0")) {
        return true;
    }
    
    return false;
}

static JSValueRef MyObject_getProperty(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    
    if (JSStringIsEqualToUTF8CString(propertyName, "alwaysOne")) {
        return JSValueMakeNumber(context, 1);
    }
    
    if (JSStringIsEqualToUTF8CString(propertyName, "myPropertyName")) {
        return JSValueMakeNumber(context, 1);
    }

    if (JSStringIsEqualToUTF8CString(propertyName, "cantFind")) {
        return JSValueMakeUndefined(context);
    }
    
    if (JSStringIsEqualToUTF8CString(propertyName, "hasPropertyLie")) {
        return 0;
    }

    if (JSStringIsEqualToUTF8CString(propertyName, "throwOnGet")) {
        return JSEvaluateScript(context, JSStringCreateWithUTF8CString("throw 'an exception'"), object, JSStringCreateWithUTF8CString("test script"), 1, exception);
    }

    if (JSStringIsEqualToUTF8CString(propertyName, "0")) {
        *exception = JSValueMakeNumber(context, 1);
        return JSValueMakeNumber(context, 1);
    }
    
    return JSValueMakeNull(context);
}

static bool MyObject_setProperty(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(value);
    UNUSED_PARAM(exception);

    if (JSStringIsEqualToUTF8CString(propertyName, "cantSet"))
        return true; // pretend we set the property in order to swallow it
    
    if (JSStringIsEqualToUTF8CString(propertyName, "throwOnSet")) {
        JSEvaluateScript(context, JSStringCreateWithUTF8CString("throw 'an exception'"), object, JSStringCreateWithUTF8CString("test script"), 1, exception);
    }
    
    return false;
}

static bool MyObject_deleteProperty(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    
    if (JSStringIsEqualToUTF8CString(propertyName, "cantDelete"))
        return true;
    
    if (JSStringIsEqualToUTF8CString(propertyName, "throwOnDelete")) {
        JSEvaluateScript(context, JSStringCreateWithUTF8CString("throw 'an exception'"), object, JSStringCreateWithUTF8CString("test script"), 1, exception);
        return false;
    }

    return false;
}

static void MyObject_getPropertyNames(JSContextRef context, JSObjectRef object, JSPropertyNameAccumulatorRef propertyNames)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    
    JSStringRef propertyName;
    
    propertyName = JSStringCreateWithUTF8CString("alwaysOne");
    JSPropertyNameAccumulatorAddName(propertyNames, propertyName);
    JSStringRelease(propertyName);
    
    propertyName = JSStringCreateWithUTF8CString("myPropertyName");
    JSPropertyNameAccumulatorAddName(propertyNames, propertyName);
    JSStringRelease(propertyName);
}

static JSValueRef MyObject_callAsFunction(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(exception);

    if (argumentCount > 0 && JSValueIsString(context, arguments[0]) && JSStringIsEqualToUTF8CString(JSValueToStringCopy(context, arguments[0], 0), "throwOnCall")) {
        JSEvaluateScript(context, JSStringCreateWithUTF8CString("throw 'an exception'"), object, JSStringCreateWithUTF8CString("test script"), 1, exception);
        return JSValueMakeUndefined(context);
    }

    if (argumentCount > 0 && JSValueIsStrictEqual(context, arguments[0], JSValueMakeNumber(context, 0)))
        return JSValueMakeNumber(context, 1);
    
    return JSValueMakeUndefined(context);
}

static JSObjectRef MyObject_callAsConstructor(JSContextRef context, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);

    if (argumentCount > 0 && JSValueIsString(context, arguments[0]) && JSStringIsEqualToUTF8CString(JSValueToStringCopy(context, arguments[0], 0), "throwOnConstruct")) {
        JSEvaluateScript(context, JSStringCreateWithUTF8CString("throw 'an exception'"), object, JSStringCreateWithUTF8CString("test script"), 1, exception);
        return object;
    }

    if (argumentCount > 0 && JSValueIsStrictEqual(context, arguments[0], JSValueMakeNumber(context, 0)))
        return JSValueToObject(context, JSValueMakeNumber(context, 1), exception);
    
    return JSValueToObject(context, JSValueMakeNumber(context, 0), exception);
}

static bool MyObject_hasInstance(JSContextRef context, JSObjectRef constructor, JSValueRef possibleValue, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(constructor);

    if (JSValueIsString(context, possibleValue) && JSStringIsEqualToUTF8CString(JSValueToStringCopy(context, possibleValue, 0), "throwOnHasInstance")) {
        JSEvaluateScript(context, JSStringCreateWithUTF8CString("throw 'an exception'"), constructor, JSStringCreateWithUTF8CString("test script"), 1, exception);
        return false;
    }

    JSStringRef numberString = JSStringCreateWithUTF8CString("Number");
    JSObjectRef numberConstructor = JSValueToObject(context, JSObjectGetProperty(context, JSContextGetGlobalObject(context), numberString, exception), exception);
    JSStringRelease(numberString);

    return JSValueIsInstanceOfConstructor(context, possibleValue, numberConstructor, exception);
}

static JSValueRef MyObject_convertToType(JSContextRef context, JSObjectRef object, JSType type, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(exception);
    
    switch (type) {
    case kJSTypeNumber:
        return JSValueMakeNumber(context, 1);
    case kJSTypeString:
        {
            JSStringRef string = JSStringCreateWithUTF8CString("MyObjectAsString");
            JSValueRef result = JSValueMakeString(context, string);
            JSStringRelease(string);
            return result;
        }
    default:
        break;
    }

    // string conversion -- forward to default object class
    return JSValueMakeNull(context);
}

static JSValueRef MyObject_convertToTypeWrapper(JSContextRef context, JSObjectRef object, JSType type, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(type);
    UNUSED_PARAM(exception);
    // Forward to default object class
    return 0;
}

static bool MyObject_set_nullGetForwardSet(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);
    UNUSED_PARAM(exception);
    return false; // Forward to parent class.
}

static JSStaticValue evilStaticValues[] = {
    { "nullGetSet", 0, 0, kJSPropertyAttributeNone },
    { "nullGetForwardSet", 0, MyObject_set_nullGetForwardSet, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSStaticFunction evilStaticFunctions[] = {
    { "nullCall", 0, kJSPropertyAttributeNone },
    { 0, 0, 0 }
};

JSClassDefinition MyObject_definition = {
    0,
    kJSClassAttributeNone,
    
    "MyObject",
    NULL,
    
    evilStaticValues,
    evilStaticFunctions,
    
    NULL,
    NULL,
    MyObject_hasProperty,
    MyObject_getProperty,
    MyObject_setProperty,
    MyObject_deleteProperty,
    MyObject_getPropertyNames,
    MyObject_callAsFunction,
    MyObject_callAsConstructor,
    MyObject_hasInstance,
    MyObject_convertToType,
};

JSClassDefinition MyObject_convertToTypeWrapperDefinition = {
    0,
    kJSClassAttributeNone,
    
    "MyObject",
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    MyObject_convertToTypeWrapper,
};

JSClassDefinition MyObject_nullWrapperDefinition = {
    0,
    kJSClassAttributeNone,
    
    "MyObject",
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static JSClassRef MyObject_class(JSContextRef context)
{
    UNUSED_PARAM(context);

    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassRef baseClass = JSClassCreate(&MyObject_definition);
        MyObject_convertToTypeWrapperDefinition.parentClass = baseClass;
        JSClassRef wrapperClass = JSClassCreate(&MyObject_convertToTypeWrapperDefinition);
        MyObject_nullWrapperDefinition.parentClass = wrapperClass;
        jsClass = JSClassCreate(&MyObject_nullWrapperDefinition);
    }

    return jsClass;
}

static JSValueRef PropertyCatchalls_getProperty(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    if (JSStringIsEqualToUTF8CString(propertyName, "x")) {
        static size_t count;
        if (count++ < 5)
            return NULL;

        // Swallow all .x gets after 5, returning null.
        return JSValueMakeNull(context);
    }

    if (JSStringIsEqualToUTF8CString(propertyName, "y")) {
        static size_t count;
        if (count++ < 5)
            return NULL;

        // Swallow all .y gets after 5, returning null.
        return JSValueMakeNull(context);
    }
    
    if (JSStringIsEqualToUTF8CString(propertyName, "z")) {
        static size_t count;
        if (count++ < 5)
            return NULL;

        // Swallow all .y gets after 5, returning null.
        return JSValueMakeNull(context);
    }

    return NULL;
}

static bool PropertyCatchalls_setProperty(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);
    UNUSED_PARAM(exception);

    if (JSStringIsEqualToUTF8CString(propertyName, "x")) {
        static size_t count;
        if (count++ < 5)
            return false;

        // Swallow all .x sets after 4.
        return true;
    }

    if (JSStringIsEqualToUTF8CString(propertyName, "make_throw") || JSStringIsEqualToUTF8CString(propertyName, "0")) {
        *exception = JSValueMakeNumber(context, 5);
        return true;
    }

    return false;
}

static void PropertyCatchalls_getPropertyNames(JSContextRef context, JSObjectRef object, JSPropertyNameAccumulatorRef propertyNames)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);

    static size_t count;
    static const char* numbers[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    
    // Provide a property of a different name every time.
    JSStringRef propertyName = JSStringCreateWithUTF8CString(numbers[count++ % 10]);
    JSPropertyNameAccumulatorAddName(propertyNames, propertyName);
    JSStringRelease(propertyName);
}

JSClassDefinition PropertyCatchalls_definition = {
    0,
    kJSClassAttributeNone,
    
    "PropertyCatchalls",
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    PropertyCatchalls_getProperty,
    PropertyCatchalls_setProperty,
    NULL,
    PropertyCatchalls_getPropertyNames,
    NULL,
    NULL,
    NULL,
    NULL,
};

static JSClassRef PropertyCatchalls_class(JSContextRef context)
{
    UNUSED_PARAM(context);

    static JSClassRef jsClass;
    if (!jsClass)
        jsClass = JSClassCreate(&PropertyCatchalls_definition);
    
    return jsClass;
}

static bool EvilExceptionObject_hasInstance(JSContextRef context, JSObjectRef constructor, JSValueRef possibleValue, JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(constructor);
    
    JSStringRef hasInstanceName = JSStringCreateWithUTF8CString("hasInstance");
    JSValueRef hasInstance = JSObjectGetProperty(context, constructor, hasInstanceName, exception);
    JSStringRelease(hasInstanceName);
    if (!hasInstance)
        return false;
    JSObjectRef function = JSValueToObject(context, hasInstance, exception);
    JSValueRef result = JSObjectCallAsFunction(context, function, constructor, 1, &possibleValue, exception);
    return result && JSValueToBoolean(context, result);
}

static JSValueRef EvilExceptionObject_convertToType(JSContextRef context, JSObjectRef object, JSType type, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(exception);
    JSStringRef funcName;
    switch (type) {
    case kJSTypeNumber:
        funcName = JSStringCreateWithUTF8CString("toNumber");
        break;
    case kJSTypeString:
        funcName = JSStringCreateWithUTF8CString("toStringExplicit");
        break;
    default:
        return JSValueMakeNull(context);
        break;
    }
    
    JSValueRef func = JSObjectGetProperty(context, object, funcName, exception);
    JSStringRelease(funcName);    
    JSObjectRef function = JSValueToObject(context, func, exception);
    if (!function)
        return JSValueMakeNull(context);
    JSValueRef value = JSObjectCallAsFunction(context, function, object, 0, NULL, exception);
    if (!value) {
        JSStringRef errorString = JSStringCreateWithUTF8CString("convertToType failed"); 
        JSValueRef errorStringRef = JSValueMakeString(context, errorString);
        JSStringRelease(errorString);
        return errorStringRef;
    }
    return value;
}

JSClassDefinition EvilExceptionObject_definition = {
    0,
    kJSClassAttributeNone,

    "EvilExceptionObject",
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    EvilExceptionObject_hasInstance,
    EvilExceptionObject_convertToType,
};

static JSClassRef EvilExceptionObject_class(JSContextRef context)
{
    UNUSED_PARAM(context);
    
    static JSClassRef jsClass;
    if (!jsClass)
        jsClass = JSClassCreate(&EvilExceptionObject_definition);
    
    return jsClass;
}

JSClassDefinition EmptyObject_definition = {
    0,
    kJSClassAttributeNone,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static JSClassRef EmptyObject_class(JSContextRef context)
{
    UNUSED_PARAM(context);
    
    static JSClassRef jsClass;
    if (!jsClass)
        jsClass = JSClassCreate(&EmptyObject_definition);
    
    return jsClass;
}


static JSValueRef Base_get(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    return JSValueMakeNumber(ctx, 1); // distinguish base get form derived get
}

static bool Base_set(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);

    *exception = JSValueMakeNumber(ctx, 1); // distinguish base set from derived set
    return true;
}

static JSValueRef Base_callAsFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return JSValueMakeNumber(ctx, 1); // distinguish base call from derived call
}

static JSValueRef Base_returnHardNull(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return 0; // should convert to undefined!
}

static JSStaticFunction Base_staticFunctions[] = {
    { "baseProtoDup", NULL, kJSPropertyAttributeNone },
    { "baseProto", Base_callAsFunction, kJSPropertyAttributeNone },
    { "baseHardNull", Base_returnHardNull, kJSPropertyAttributeNone },
    { 0, 0, 0 }
};

static JSStaticValue Base_staticValues[] = {
    { "baseDup", Base_get, Base_set, kJSPropertyAttributeNone },
    { "baseOnly", Base_get, Base_set, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static bool TestInitializeFinalize;
static void Base_initialize(JSContextRef context, JSObjectRef object)
{
    UNUSED_PARAM(context);

    if (TestInitializeFinalize) {
        ASSERT((void*)1 == JSObjectGetPrivate(object));
        JSObjectSetPrivate(object, (void*)2);
    }
}

static unsigned Base_didFinalize;
static void Base_finalize(JSObjectRef object)
{
    UNUSED_PARAM(object);
    if (TestInitializeFinalize) {
        ASSERT((void*)4 == JSObjectGetPrivate(object));
        Base_didFinalize = true;
    }
}

static JSClassRef Base_class(JSContextRef context)
{
    UNUSED_PARAM(context);

    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.staticValues = Base_staticValues;
        definition.staticFunctions = Base_staticFunctions;
        definition.initialize = Base_initialize;
        definition.finalize = Base_finalize;
        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

static JSValueRef Derived_get(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    return JSValueMakeNumber(ctx, 2); // distinguish base get form derived get
}

static bool Derived_set(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);

    *exception = JSValueMakeNumber(ctx, 2); // distinguish base set from derived set
    return true;
}

static JSValueRef Derived_callAsFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return JSValueMakeNumber(ctx, 2); // distinguish base call from derived call
}

static JSStaticFunction Derived_staticFunctions[] = {
    { "protoOnly", Derived_callAsFunction, kJSPropertyAttributeNone },
    { "protoDup", NULL, kJSPropertyAttributeNone },
    { "baseProtoDup", Derived_callAsFunction, kJSPropertyAttributeNone },
    { 0, 0, 0 }
};

static JSStaticValue Derived_staticValues[] = {
    { "derivedOnly", Derived_get, Derived_set, kJSPropertyAttributeNone },
    { "protoDup", Derived_get, Derived_set, kJSPropertyAttributeNone },
    { "baseDup", Derived_get, Derived_set, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static void Derived_initialize(JSContextRef context, JSObjectRef object)
{
    UNUSED_PARAM(context);

    if (TestInitializeFinalize) {
        ASSERT((void*)2 == JSObjectGetPrivate(object));
        JSObjectSetPrivate(object, (void*)3);
    }
}

static void Derived_finalize(JSObjectRef object)
{
    if (TestInitializeFinalize) {
        ASSERT((void*)3 == JSObjectGetPrivate(object));
        JSObjectSetPrivate(object, (void*)4);
    }
}

static JSClassRef Derived_class(JSContextRef context)
{
    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.parentClass = Base_class(context);
        definition.staticValues = Derived_staticValues;
        definition.staticFunctions = Derived_staticFunctions;
        definition.initialize = Derived_initialize;
        definition.finalize = Derived_finalize;
        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

static JSClassRef Derived2_class(JSContextRef context)
{
    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.parentClass = Derived_class(context);
        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

static JSValueRef print_callAsFunction(JSContextRef ctx, JSObjectRef functionObject, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(functionObject);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(exception);

    ASSERT(JSContextGetGlobalContext(ctx) == context);
    
    if (argumentCount > 0) {
        JSStringRef string = JSValueToStringCopy(ctx, arguments[0], NULL);
        size_t sizeUTF8 = JSStringGetMaximumUTF8CStringSize(string);
        char* stringUTF8 = (char*)malloc(sizeUTF8);
        JSStringGetUTF8CString(string, stringUTF8, sizeUTF8);
        printf("%s\n", stringUTF8);
        free(stringUTF8);
        JSStringRelease(string);
    }
    
    return JSValueMakeUndefined(ctx);
}

static JSObjectRef myConstructor_callAsConstructor(JSContextRef context, JSObjectRef constructorObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(constructorObject);
    UNUSED_PARAM(exception);
    
    JSObjectRef result = JSObjectMake(context, NULL, NULL);
    if (argumentCount > 0) {
        JSStringRef value = JSStringCreateWithUTF8CString("value");
        JSObjectSetProperty(context, result, value, arguments[0], kJSPropertyAttributeNone, NULL);
        JSStringRelease(value);
    }
    
    return result;
}

static JSObjectRef myBadConstructor_callAsConstructor(JSContextRef context, JSObjectRef constructorObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(constructorObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return 0;
}


static void globalObject_initialize(JSContextRef context, JSObjectRef object)
{
    UNUSED_PARAM(object);
    // Ensure that an execution context is passed in
    ASSERT(context);

    // Ensure that the global object is set to the object that we were passed
    JSObjectRef globalObject = JSContextGetGlobalObject(context);
    ASSERT(globalObject);
    ASSERT(object == globalObject);

    // Ensure that the standard global properties have been set on the global object
    JSStringRef array = JSStringCreateWithUTF8CString("Array");
    JSObjectRef arrayConstructor = JSValueToObject(context, JSObjectGetProperty(context, globalObject, array, NULL), NULL);
    JSStringRelease(array);

    UNUSED_PARAM(arrayConstructor);
    ASSERT(arrayConstructor);
}

static JSValueRef globalObject_get(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    return JSValueMakeNumber(ctx, 3);
}

static bool globalObject_set(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);

    *exception = JSValueMakeNumber(ctx, 3);
    return true;
}

static JSValueRef globalObject_call(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);

    return JSValueMakeNumber(ctx, 3);
}

static JSValueRef functionGC(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    JSGarbageCollect(context);
    return JSValueMakeUndefined(context);
}

static JSStaticValue globalObject_staticValues[] = {
    { "globalStaticValue", globalObject_get, globalObject_set, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSStaticFunction globalObject_staticFunctions[] = {
    { "globalStaticFunction", globalObject_call, kJSPropertyAttributeNone },
    { "gc", functionGC, kJSPropertyAttributeNone },
    { 0, 0, 0 }
};

static char* createStringWithContentsOfFile(const char* fileName);

static void testInitializeFinalize()
{
    JSObjectRef o = JSObjectMake(context, Derived_class(context), (void*)1);
    UNUSED_PARAM(o);
    ASSERT(JSObjectGetPrivate(o) == (void*)3);
}

static JSValueRef jsNumberValue =  NULL;

static JSObjectRef aHeapRef = NULL;

static void makeGlobalNumberValue(JSContextRef context) {
    JSValueRef v = JSValueMakeNumber(context, 420);
    JSValueProtect(context, v);
    jsNumberValue = v;
    v = NULL;
}

static bool assertTrue(bool value, const char* message)
{
    if (!value) {
        if (message)
            fprintf(stderr, "assertTrue failed: '%s'\n", message);
        else
            fprintf(stderr, "assertTrue failed.\n");
        failed = 1;
    }
    return value;
}

static bool checkForCycleInPrototypeChain()
{
    bool result = true;
    JSGlobalContextRef context = JSGlobalContextCreate(0);
    JSObjectRef object1 = JSObjectMake(context, /* jsClass */ 0, /* data */ 0);
    JSObjectRef object2 = JSObjectMake(context, /* jsClass */ 0, /* data */ 0);
    JSObjectRef object3 = JSObjectMake(context, /* jsClass */ 0, /* data */ 0);

    JSObjectSetPrototype(context, object1, JSValueMakeNull(context));
    ASSERT(JSValueIsNull(context, JSObjectGetPrototype(context, object1)));

    // object1 -> object1
    JSObjectSetPrototype(context, object1, object1);
    result &= assertTrue(JSValueIsNull(context, JSObjectGetPrototype(context, object1)), "It is possible to assign self as a prototype");

    // object1 -> object2 -> object1
    JSObjectSetPrototype(context, object2, object1);
    ASSERT(JSValueIsStrictEqual(context, JSObjectGetPrototype(context, object2), object1));
    JSObjectSetPrototype(context, object1, object2);
    result &= assertTrue(JSValueIsNull(context, JSObjectGetPrototype(context, object1)), "It is possible to close a prototype chain cycle");

    // object1 -> object2 -> object3 -> object1
    JSObjectSetPrototype(context, object2, object3);
    ASSERT(JSValueIsStrictEqual(context, JSObjectGetPrototype(context, object2), object3));
    JSObjectSetPrototype(context, object1, object2);
    ASSERT(JSValueIsStrictEqual(context, JSObjectGetPrototype(context, object1), object2));
    JSObjectSetPrototype(context, object3, object1);
    result &= assertTrue(!JSValueIsStrictEqual(context, JSObjectGetPrototype(context, object3), object1), "It is possible to close a prototype chain cycle");

    JSValueRef exception;
    JSStringRef code = JSStringCreateWithUTF8CString("o = { }; p = { }; o.__proto__ = p; p.__proto__ = o");
    JSStringRef file = JSStringCreateWithUTF8CString("");
    result &= assertTrue(!JSEvaluateScript(context, code, /* thisObject*/ 0, file, 1, &exception)
                         , "An exception should be thrown");

    JSStringRelease(code);
    JSStringRelease(file);
    JSGlobalContextRelease(context);
    return result;
}

static void checkConstnessInJSObjectNames()
{
    JSStaticFunction fun;
    fun.name = "something";
    JSStaticValue val;
    val.name = "something";
}

#if PLATFORM(MAC) || PLATFORM(IOS)
static double currentCPUTime()
{
    mach_msg_type_number_t infoCount = THREAD_BASIC_INFO_COUNT;
    thread_basic_info_data_t info;

    /* Get thread information */
    mach_port_t threadPort = mach_thread_self();
    thread_info(threadPort, THREAD_BASIC_INFO, (thread_info_t)(&info), &infoCount);
    mach_port_deallocate(mach_task_self(), threadPort);
    
    double time = info.user_time.seconds + info.user_time.microseconds / 1000000.;
    time += info.system_time.seconds + info.system_time.microseconds / 1000000.;
    
    return time;
}

static JSValueRef currentCPUTime_callAsFunction(JSContextRef ctx, JSObjectRef functionObject, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(functionObject);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);

    ASSERT(JSContextGetGlobalContext(ctx) == context);
    return JSValueMakeNumber(ctx, currentCPUTime());
}

bool shouldTerminateCallbackWasCalled = false;
static bool shouldTerminateCallback(JSContextRef ctx, void* context)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(context);
    shouldTerminateCallbackWasCalled = true;
    return true;
}

bool cancelTerminateCallbackWasCalled = false;
static bool cancelTerminateCallback(JSContextRef ctx, void* context)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(context);
    cancelTerminateCallbackWasCalled = true;
    return false;
}

int extendTerminateCallbackCalled = 0;
static bool extendTerminateCallback(JSContextRef ctx, void* context)
{
    UNUSED_PARAM(context);
    extendTerminateCallbackCalled++;
    if (extendTerminateCallbackCalled == 1) {
        JSContextGroupRef contextGroup = JSContextGetGroup(ctx);
        JSContextGroupSetExecutionTimeLimit(contextGroup, .200f, extendTerminateCallback, 0);
        return false;
    }
    return true;
}
#endif /* PLATFORM(MAC) || PLATFORM(IOS) */


int main(int argc, char* argv[])
{
#if OS(WINDOWS)
    // Cygwin calls ::SetErrorMode(SEM_FAILCRITICALERRORS), which we will inherit. This is bad for
    // testing/debugging, as it causes the post-mortem debugger not to be invoked. We reset the
    // error mode here to work around Cygwin's behavior. See <http://webkit.org/b/55222>.
    ::SetErrorMode(0);
#endif

#if JSC_OBJC_API_ENABLED
    testObjectiveCAPI();
#endif

    const char *scriptPath = "testapi.js";
    if (argc > 1) {
        scriptPath = argv[1];
    }
    
    // Test garbage collection with a fresh context
    context = JSGlobalContextCreateInGroup(NULL, NULL);
    TestInitializeFinalize = true;
    testInitializeFinalize();
    JSGlobalContextRelease(context);
    TestInitializeFinalize = false;

    ASSERT(Base_didFinalize);

    JSClassDefinition globalObjectClassDefinition = kJSClassDefinitionEmpty;
    globalObjectClassDefinition.initialize = globalObject_initialize;
    globalObjectClassDefinition.staticValues = globalObject_staticValues;
    globalObjectClassDefinition.staticFunctions = globalObject_staticFunctions;
    globalObjectClassDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    JSClassRef globalObjectClass = JSClassCreate(&globalObjectClassDefinition);
    context = JSGlobalContextCreateInGroup(NULL, globalObjectClass);

    JSContextGroupRef contextGroup = JSContextGetGroup(context);
    
    JSGlobalContextRetain(context);
    JSGlobalContextRelease(context);
    ASSERT(JSContextGetGlobalContext(context) == context);
    
    JSReportExtraMemoryCost(context, 0);
    JSReportExtraMemoryCost(context, 1);
    JSReportExtraMemoryCost(context, 1024);

    JSObjectRef globalObject = JSContextGetGlobalObject(context);
    ASSERT(JSValueIsObject(context, globalObject));
    
    JSValueRef jsUndefined = JSValueMakeUndefined(context);
    JSValueRef jsNull = JSValueMakeNull(context);
    JSValueRef jsTrue = JSValueMakeBoolean(context, true);
    JSValueRef jsFalse = JSValueMakeBoolean(context, false);
    JSValueRef jsZero = JSValueMakeNumber(context, 0);
    JSValueRef jsOne = JSValueMakeNumber(context, 1);
    JSValueRef jsOneThird = JSValueMakeNumber(context, 1.0 / 3.0);
    JSObjectRef jsObjectNoProto = JSObjectMake(context, NULL, NULL);
    JSObjectSetPrototype(context, jsObjectNoProto, JSValueMakeNull(context));

    // FIXME: test funny utf8 characters
    JSStringRef jsEmptyIString = JSStringCreateWithUTF8CString("");
    JSValueRef jsEmptyString = JSValueMakeString(context, jsEmptyIString);
    
    JSStringRef jsOneIString = JSStringCreateWithUTF8CString("1");
    JSValueRef jsOneString = JSValueMakeString(context, jsOneIString);

    UniChar singleUniChar = 65; // Capital A
    CFMutableStringRef cfString = 
        CFStringCreateMutableWithExternalCharactersNoCopy(kCFAllocatorDefault,
                                                          &singleUniChar,
                                                          1,
                                                          1,
                                                          kCFAllocatorNull);

    JSStringRef jsCFIString = JSStringCreateWithCFString(cfString);
    JSValueRef jsCFString = JSValueMakeString(context, jsCFIString);
    
    CFStringRef cfEmptyString = CFStringCreateWithCString(kCFAllocatorDefault, "", kCFStringEncodingUTF8);
    
    JSStringRef jsCFEmptyIString = JSStringCreateWithCFString(cfEmptyString);
    JSValueRef jsCFEmptyString = JSValueMakeString(context, jsCFEmptyIString);

    CFIndex cfStringLength = CFStringGetLength(cfString);
    UniChar* buffer = (UniChar*)malloc(cfStringLength * sizeof(UniChar));
    CFStringGetCharacters(cfString, 
                          CFRangeMake(0, cfStringLength), 
                          buffer);
    JSStringRef jsCFIStringWithCharacters = JSStringCreateWithCharacters((JSChar*)buffer, cfStringLength);
    JSValueRef jsCFStringWithCharacters = JSValueMakeString(context, jsCFIStringWithCharacters);
    
    JSStringRef jsCFEmptyIStringWithCharacters = JSStringCreateWithCharacters((JSChar*)buffer, CFStringGetLength(cfEmptyString));
    free(buffer);
    JSValueRef jsCFEmptyStringWithCharacters = JSValueMakeString(context, jsCFEmptyIStringWithCharacters);

    JSChar constantString[] = { 'H', 'e', 'l', 'l', 'o', };
    JSStringRef constantStringRef = JSStringCreateWithCharactersNoCopy(constantString, sizeof(constantString) / sizeof(constantString[0]));
    ASSERT(JSStringGetCharactersPtr(constantStringRef) == constantString);
    JSStringRelease(constantStringRef);

    ASSERT(JSValueGetType(context, NULL) == kJSTypeNull);
    ASSERT(JSValueGetType(context, jsUndefined) == kJSTypeUndefined);
    ASSERT(JSValueGetType(context, jsNull) == kJSTypeNull);
    ASSERT(JSValueGetType(context, jsTrue) == kJSTypeBoolean);
    ASSERT(JSValueGetType(context, jsFalse) == kJSTypeBoolean);
    ASSERT(JSValueGetType(context, jsZero) == kJSTypeNumber);
    ASSERT(JSValueGetType(context, jsOne) == kJSTypeNumber);
    ASSERT(JSValueGetType(context, jsOneThird) == kJSTypeNumber);
    ASSERT(JSValueGetType(context, jsEmptyString) == kJSTypeString);
    ASSERT(JSValueGetType(context, jsOneString) == kJSTypeString);
    ASSERT(JSValueGetType(context, jsCFString) == kJSTypeString);
    ASSERT(JSValueGetType(context, jsCFStringWithCharacters) == kJSTypeString);
    ASSERT(JSValueGetType(context, jsCFEmptyString) == kJSTypeString);
    ASSERT(JSValueGetType(context, jsCFEmptyStringWithCharacters) == kJSTypeString);

    ASSERT(!JSValueIsBoolean(context, NULL));
    ASSERT(!JSValueIsObject(context, NULL));
    ASSERT(!JSValueIsString(context, NULL));
    ASSERT(!JSValueIsNumber(context, NULL));
    ASSERT(!JSValueIsUndefined(context, NULL));
    ASSERT(JSValueIsNull(context, NULL));
    ASSERT(!JSObjectCallAsFunction(context, NULL, NULL, 0, NULL, NULL));
    ASSERT(!JSObjectCallAsConstructor(context, NULL, 0, NULL, NULL));
    ASSERT(!JSObjectIsConstructor(context, NULL));
    ASSERT(!JSObjectIsFunction(context, NULL));

    JSStringRef nullString = JSStringCreateWithUTF8CString(0);
    const JSChar* characters = JSStringGetCharactersPtr(nullString);
    if (characters) {
        printf("FAIL: Didn't return null when accessing character pointer of a null String.\n");
        failed = 1;
    } else
        printf("PASS: returned null when accessing character pointer of a null String.\n");

    JSStringRef emptyString = JSStringCreateWithCFString(CFSTR(""));
    characters = JSStringGetCharactersPtr(emptyString);
    if (!characters) {
        printf("FAIL: Returned null when accessing character pointer of an empty String.\n");
        failed = 1;
    } else
        printf("PASS: returned empty when accessing character pointer of an empty String.\n");

    size_t length = JSStringGetLength(nullString);
    if (length) {
        printf("FAIL: Didn't return 0 length for null String.\n");
        failed = 1;
    } else
        printf("PASS: returned 0 length for null String.\n");
    JSStringRelease(nullString);

    length = JSStringGetLength(emptyString);
    if (length) {
        printf("FAIL: Didn't return 0 length for empty String.\n");
        failed = 1;
    } else
        printf("PASS: returned 0 length for empty String.\n");
    JSStringRelease(emptyString);

    JSObjectRef propertyCatchalls = JSObjectMake(context, PropertyCatchalls_class(context), NULL);
    JSStringRef propertyCatchallsString = JSStringCreateWithUTF8CString("PropertyCatchalls");
    JSObjectSetProperty(context, globalObject, propertyCatchallsString, propertyCatchalls, kJSPropertyAttributeNone, NULL);
    JSStringRelease(propertyCatchallsString);

    JSObjectRef myObject = JSObjectMake(context, MyObject_class(context), NULL);
    JSStringRef myObjectIString = JSStringCreateWithUTF8CString("MyObject");
    JSObjectSetProperty(context, globalObject, myObjectIString, myObject, kJSPropertyAttributeNone, NULL);
    JSStringRelease(myObjectIString);
    
    JSObjectRef EvilExceptionObject = JSObjectMake(context, EvilExceptionObject_class(context), NULL);
    JSStringRef EvilExceptionObjectIString = JSStringCreateWithUTF8CString("EvilExceptionObject");
    JSObjectSetProperty(context, globalObject, EvilExceptionObjectIString, EvilExceptionObject, kJSPropertyAttributeNone, NULL);
    JSStringRelease(EvilExceptionObjectIString);
    
    JSObjectRef EmptyObject = JSObjectMake(context, EmptyObject_class(context), NULL);
    JSStringRef EmptyObjectIString = JSStringCreateWithUTF8CString("EmptyObject");
    JSObjectSetProperty(context, globalObject, EmptyObjectIString, EmptyObject, kJSPropertyAttributeNone, NULL);
    JSStringRelease(EmptyObjectIString);
    
    JSStringRef lengthStr = JSStringCreateWithUTF8CString("length");
    JSObjectRef aStackRef = JSObjectMakeArray(context, 0, 0, 0);
    aHeapRef = aStackRef;
    JSObjectSetProperty(context, aHeapRef, lengthStr, JSValueMakeNumber(context, 10), 0, 0);
    JSStringRef privatePropertyName = JSStringCreateWithUTF8CString("privateProperty");
    if (!JSObjectSetPrivateProperty(context, myObject, privatePropertyName, aHeapRef)) {
        printf("FAIL: Could not set private property.\n");
        failed = 1;
    } else
        printf("PASS: Set private property.\n");
    aStackRef = 0;
    if (JSObjectSetPrivateProperty(context, aHeapRef, privatePropertyName, aHeapRef)) {
        printf("FAIL: JSObjectSetPrivateProperty should fail on non-API objects.\n");
        failed = 1;
    } else
        printf("PASS: Did not allow JSObjectSetPrivateProperty on a non-API object.\n");
    if (JSObjectGetPrivateProperty(context, myObject, privatePropertyName) != aHeapRef) {
        printf("FAIL: Could not retrieve private property.\n");
        failed = 1;
    } else
        printf("PASS: Retrieved private property.\n");
    if (JSObjectGetPrivateProperty(context, aHeapRef, privatePropertyName)) {
        printf("FAIL: JSObjectGetPrivateProperty should return NULL when called on a non-API object.\n");
        failed = 1;
    } else
        printf("PASS: JSObjectGetPrivateProperty return NULL.\n");

    if (JSObjectGetProperty(context, myObject, privatePropertyName, 0) == aHeapRef) {
        printf("FAIL: Accessed private property through ordinary property lookup.\n");
        failed = 1;
    } else
        printf("PASS: Cannot access private property through ordinary property lookup.\n");

    JSGarbageCollect(context);

    for (int i = 0; i < 10000; i++)
        JSObjectMake(context, 0, 0);

    aHeapRef = JSValueToObject(context, JSObjectGetPrivateProperty(context, myObject, privatePropertyName), 0);
    if (JSValueToNumber(context, JSObjectGetProperty(context, aHeapRef, lengthStr, 0), 0) != 10) {
        printf("FAIL: Private property has been collected.\n");
        failed = 1;
    } else
        printf("PASS: Private property does not appear to have been collected.\n");
    JSStringRelease(lengthStr);

    if (!JSObjectSetPrivateProperty(context, myObject, privatePropertyName, 0)) {
        printf("FAIL: Could not set private property to NULL.\n");
        failed = 1;
    } else
        printf("PASS: Set private property to NULL.\n");
    if (JSObjectGetPrivateProperty(context, myObject, privatePropertyName)) {
        printf("FAIL: Could not retrieve private property.\n");
        failed = 1;
    } else
        printf("PASS: Retrieved private property.\n");

    JSStringRef nullJSON = JSStringCreateWithUTF8CString(0);
    JSValueRef nullJSONObject = JSValueMakeFromJSONString(context, nullJSON);
    if (nullJSONObject) {
        printf("FAIL: Did not parse null String as JSON correctly\n");
        failed = 1;
    } else
        printf("PASS: Parsed null String as JSON correctly.\n");
    JSStringRelease(nullJSON);

    JSStringRef validJSON = JSStringCreateWithUTF8CString("{\"aProperty\":true}");
    JSValueRef jsonObject = JSValueMakeFromJSONString(context, validJSON);
    JSStringRelease(validJSON);
    if (!JSValueIsObject(context, jsonObject)) {
        printf("FAIL: Did not parse valid JSON correctly\n");
        failed = 1;
    } else
        printf("PASS: Parsed valid JSON string.\n");
    JSStringRef propertyName = JSStringCreateWithUTF8CString("aProperty");
    assertEqualsAsBoolean(JSObjectGetProperty(context, JSValueToObject(context, jsonObject, 0), propertyName, 0), true);
    JSStringRelease(propertyName);
    JSStringRef invalidJSON = JSStringCreateWithUTF8CString("fail!");
    if (JSValueMakeFromJSONString(context, invalidJSON)) {
        printf("FAIL: Should return null for invalid JSON data\n");
        failed = 1;
    } else
        printf("PASS: Correctly returned null for invalid JSON data.\n");
    JSValueRef exception;
    JSStringRef str = JSValueCreateJSONString(context, jsonObject, 0, 0);
    if (!JSStringIsEqualToUTF8CString(str, "{\"aProperty\":true}")) {
        printf("FAIL: Did not correctly serialise with indent of 0.\n");
        failed = 1;
    } else
        printf("PASS: Correctly serialised with indent of 0.\n");
    JSStringRelease(str);

    str = JSValueCreateJSONString(context, jsonObject, 4, 0);
    if (!JSStringIsEqualToUTF8CString(str, "{\n    \"aProperty\": true\n}")) {
        printf("FAIL: Did not correctly serialise with indent of 4.\n");
        failed = 1;
    } else
        printf("PASS: Correctly serialised with indent of 4.\n");
    JSStringRelease(str);
    JSStringRef src = JSStringCreateWithUTF8CString("({get a(){ throw '';}})");
    JSValueRef unstringifiableObj = JSEvaluateScript(context, src, NULL, NULL, 1, NULL);
    
    str = JSValueCreateJSONString(context, unstringifiableObj, 4, 0);
    if (str) {
        printf("FAIL: Didn't return null when attempting to serialize unserializable value.\n");
        JSStringRelease(str);
        failed = 1;
    } else
        printf("PASS: returned null when attempting to serialize unserializable value.\n");
    
    str = JSValueCreateJSONString(context, unstringifiableObj, 4, &exception);
    if (str) {
        printf("FAIL: Didn't return null when attempting to serialize unserializable value.\n");
        JSStringRelease(str);
        failed = 1;
    } else
        printf("PASS: returned null when attempting to serialize unserializable value.\n");
    if (!exception) {
        printf("FAIL: Did not set exception on serialisation error\n");
        failed = 1;
    } else
        printf("PASS: set exception on serialisation error\n");
    // Conversions that throw exceptions
    exception = NULL;
    ASSERT(NULL == JSValueToObject(context, jsNull, &exception));
    ASSERT(exception);
    
    exception = NULL;
    // FIXME <rdar://4668451> - On i386 the isnan(double) macro tries to map to the isnan(float) function,
    // causing a build break with -Wshorten-64-to-32 enabled.  The issue is known by the appropriate team.
    // After that's resolved, we can remove these casts
    ASSERT(isnan((float)JSValueToNumber(context, jsObjectNoProto, &exception)));
    ASSERT(exception);

    exception = NULL;
    ASSERT(!JSValueToStringCopy(context, jsObjectNoProto, &exception));
    ASSERT(exception);
    
    ASSERT(JSValueToBoolean(context, myObject));
    
    exception = NULL;
    ASSERT(!JSValueIsEqual(context, jsObjectNoProto, JSValueMakeNumber(context, 1), &exception));
    ASSERT(exception);
    
    exception = NULL;
    JSObjectGetPropertyAtIndex(context, myObject, 0, &exception);
    ASSERT(1 == JSValueToNumber(context, exception, NULL));

    assertEqualsAsBoolean(jsUndefined, false);
    assertEqualsAsBoolean(jsNull, false);
    assertEqualsAsBoolean(jsTrue, true);
    assertEqualsAsBoolean(jsFalse, false);
    assertEqualsAsBoolean(jsZero, false);
    assertEqualsAsBoolean(jsOne, true);
    assertEqualsAsBoolean(jsOneThird, true);
    assertEqualsAsBoolean(jsEmptyString, false);
    assertEqualsAsBoolean(jsOneString, true);
    assertEqualsAsBoolean(jsCFString, true);
    assertEqualsAsBoolean(jsCFStringWithCharacters, true);
    assertEqualsAsBoolean(jsCFEmptyString, false);
    assertEqualsAsBoolean(jsCFEmptyStringWithCharacters, false);
    
    assertEqualsAsNumber(jsUndefined, nan(""));
    assertEqualsAsNumber(jsNull, 0);
    assertEqualsAsNumber(jsTrue, 1);
    assertEqualsAsNumber(jsFalse, 0);
    assertEqualsAsNumber(jsZero, 0);
    assertEqualsAsNumber(jsOne, 1);
    assertEqualsAsNumber(jsOneThird, 1.0 / 3.0);
    assertEqualsAsNumber(jsEmptyString, 0);
    assertEqualsAsNumber(jsOneString, 1);
    assertEqualsAsNumber(jsCFString, nan(""));
    assertEqualsAsNumber(jsCFStringWithCharacters, nan(""));
    assertEqualsAsNumber(jsCFEmptyString, 0);
    assertEqualsAsNumber(jsCFEmptyStringWithCharacters, 0);
    ASSERT(sizeof(JSChar) == sizeof(UniChar));
    
    assertEqualsAsCharactersPtr(jsUndefined, "undefined");
    assertEqualsAsCharactersPtr(jsNull, "null");
    assertEqualsAsCharactersPtr(jsTrue, "true");
    assertEqualsAsCharactersPtr(jsFalse, "false");
    assertEqualsAsCharactersPtr(jsZero, "0");
    assertEqualsAsCharactersPtr(jsOne, "1");
    assertEqualsAsCharactersPtr(jsOneThird, "0.3333333333333333");
    assertEqualsAsCharactersPtr(jsEmptyString, "");
    assertEqualsAsCharactersPtr(jsOneString, "1");
    assertEqualsAsCharactersPtr(jsCFString, "A");
    assertEqualsAsCharactersPtr(jsCFStringWithCharacters, "A");
    assertEqualsAsCharactersPtr(jsCFEmptyString, "");
    assertEqualsAsCharactersPtr(jsCFEmptyStringWithCharacters, "");
    
    assertEqualsAsUTF8String(jsUndefined, "undefined");
    assertEqualsAsUTF8String(jsNull, "null");
    assertEqualsAsUTF8String(jsTrue, "true");
    assertEqualsAsUTF8String(jsFalse, "false");
    assertEqualsAsUTF8String(jsZero, "0");
    assertEqualsAsUTF8String(jsOne, "1");
    assertEqualsAsUTF8String(jsOneThird, "0.3333333333333333");
    assertEqualsAsUTF8String(jsEmptyString, "");
    assertEqualsAsUTF8String(jsOneString, "1");
    assertEqualsAsUTF8String(jsCFString, "A");
    assertEqualsAsUTF8String(jsCFStringWithCharacters, "A");
    assertEqualsAsUTF8String(jsCFEmptyString, "");
    assertEqualsAsUTF8String(jsCFEmptyStringWithCharacters, "");
    
    checkConstnessInJSObjectNames();
    
    ASSERT(JSValueIsStrictEqual(context, jsTrue, jsTrue));
    ASSERT(!JSValueIsStrictEqual(context, jsOne, jsOneString));

    ASSERT(JSValueIsEqual(context, jsOne, jsOneString, NULL));
    ASSERT(!JSValueIsEqual(context, jsTrue, jsFalse, NULL));
    
    CFStringRef cfJSString = JSStringCopyCFString(kCFAllocatorDefault, jsCFIString);
    CFStringRef cfJSEmptyString = JSStringCopyCFString(kCFAllocatorDefault, jsCFEmptyIString);
    ASSERT(CFEqual(cfJSString, cfString));
    ASSERT(CFEqual(cfJSEmptyString, cfEmptyString));
    CFRelease(cfJSString);
    CFRelease(cfJSEmptyString);

    CFRelease(cfString);
    CFRelease(cfEmptyString);
    
    jsGlobalValue = JSObjectMake(context, NULL, NULL);
    makeGlobalNumberValue(context);
    JSValueProtect(context, jsGlobalValue);
    JSGarbageCollect(context);
    ASSERT(JSValueIsObject(context, jsGlobalValue));
    JSValueUnprotect(context, jsGlobalValue);
    JSValueUnprotect(context, jsNumberValue);

    JSStringRef goodSyntax = JSStringCreateWithUTF8CString("x = 1;");
    const char* badSyntaxConstant = "x := 1;";
    JSStringRef badSyntax = JSStringCreateWithUTF8CString(badSyntaxConstant);
    ASSERT(JSCheckScriptSyntax(context, goodSyntax, NULL, 0, NULL));
    ASSERT(!JSCheckScriptSyntax(context, badSyntax, NULL, 0, NULL));
    ASSERT(!JSScriptCreateFromString(contextGroup, 0, 0, badSyntax, 0, 0));
    ASSERT(!JSScriptCreateReferencingImmortalASCIIText(contextGroup, 0, 0, badSyntaxConstant, strlen(badSyntaxConstant), 0, 0));

    JSValueRef result;
    JSValueRef v;
    JSObjectRef o;
    JSStringRef string;

    result = JSEvaluateScript(context, goodSyntax, NULL, NULL, 1, NULL);
    ASSERT(result);
    ASSERT(JSValueIsEqual(context, result, jsOne, NULL));

    exception = NULL;
    result = JSEvaluateScript(context, badSyntax, NULL, NULL, 1, &exception);
    ASSERT(!result);
    ASSERT(JSValueIsObject(context, exception));
    
    JSStringRef array = JSStringCreateWithUTF8CString("Array");
    JSObjectRef arrayConstructor = JSValueToObject(context, JSObjectGetProperty(context, globalObject, array, NULL), NULL);
    JSStringRelease(array);
    result = JSObjectCallAsConstructor(context, arrayConstructor, 0, NULL, NULL);
    ASSERT(result);
    ASSERT(JSValueIsObject(context, result));
    ASSERT(JSValueIsInstanceOfConstructor(context, result, arrayConstructor, NULL));
    ASSERT(!JSValueIsInstanceOfConstructor(context, JSValueMakeNull(context), arrayConstructor, NULL));

    o = JSValueToObject(context, result, NULL);
    exception = NULL;
    ASSERT(JSValueIsUndefined(context, JSObjectGetPropertyAtIndex(context, o, 0, &exception)));
    ASSERT(!exception);
    
    JSObjectSetPropertyAtIndex(context, o, 0, JSValueMakeNumber(context, 1), &exception);
    ASSERT(!exception);
    
    exception = NULL;
    ASSERT(1 == JSValueToNumber(context, JSObjectGetPropertyAtIndex(context, o, 0, &exception), &exception));
    ASSERT(!exception);

    JSStringRef functionBody;
    JSObjectRef function;
    
    exception = NULL;
    functionBody = JSStringCreateWithUTF8CString("rreturn Array;");
    JSStringRef line = JSStringCreateWithUTF8CString("line");
    ASSERT(!JSObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, &exception));
    ASSERT(JSValueIsObject(context, exception));
    v = JSObjectGetProperty(context, JSValueToObject(context, exception, NULL), line, NULL);
    assertEqualsAsNumber(v, 1);
    JSStringRelease(functionBody);
    JSStringRelease(line);

    exception = NULL;
    functionBody = JSStringCreateWithUTF8CString("return Array;");
    function = JSObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, &exception);
    JSStringRelease(functionBody);
    ASSERT(!exception);
    ASSERT(JSObjectIsFunction(context, function));
    v = JSObjectCallAsFunction(context, function, NULL, 0, NULL, NULL);
    ASSERT(v);
    ASSERT(JSValueIsEqual(context, v, arrayConstructor, NULL));
    
    exception = NULL;
    function = JSObjectMakeFunction(context, NULL, 0, NULL, jsEmptyIString, NULL, 0, &exception);
    ASSERT(!exception);
    v = JSObjectCallAsFunction(context, function, NULL, 0, NULL, &exception);
    ASSERT(v && !exception);
    ASSERT(JSValueIsUndefined(context, v));
    
    exception = NULL;
    v = NULL;
    JSStringRef foo = JSStringCreateWithUTF8CString("foo");
    JSStringRef argumentNames[] = { foo };
    functionBody = JSStringCreateWithUTF8CString("return foo;");
    function = JSObjectMakeFunction(context, foo, 1, argumentNames, functionBody, NULL, 1, &exception);
    ASSERT(function && !exception);
    JSValueRef arguments[] = { JSValueMakeNumber(context, 2) };
    JSObjectCallAsFunction(context, function, NULL, 1, arguments, &exception);
    JSStringRelease(foo);
    JSStringRelease(functionBody);
    
    string = JSValueToStringCopy(context, function, NULL);
    assertEqualsAsUTF8String(JSValueMakeString(context, string), "function foo(foo) { return foo;\n}");
    JSStringRelease(string);

    JSStringRef print = JSStringCreateWithUTF8CString("print");
    JSObjectRef printFunction = JSObjectMakeFunctionWithCallback(context, print, print_callAsFunction);
    JSObjectSetProperty(context, globalObject, print, printFunction, kJSPropertyAttributeNone, NULL); 
    JSStringRelease(print);
    
    ASSERT(!JSObjectSetPrivate(printFunction, (void*)1));
    ASSERT(!JSObjectGetPrivate(printFunction));

    JSStringRef myConstructorIString = JSStringCreateWithUTF8CString("MyConstructor");
    JSObjectRef myConstructor = JSObjectMakeConstructor(context, NULL, myConstructor_callAsConstructor);
    JSObjectSetProperty(context, globalObject, myConstructorIString, myConstructor, kJSPropertyAttributeNone, NULL);
    JSStringRelease(myConstructorIString);
    
    JSStringRef myBadConstructorIString = JSStringCreateWithUTF8CString("MyBadConstructor");
    JSObjectRef myBadConstructor = JSObjectMakeConstructor(context, NULL, myBadConstructor_callAsConstructor);
    JSObjectSetProperty(context, globalObject, myBadConstructorIString, myBadConstructor, kJSPropertyAttributeNone, NULL);
    JSStringRelease(myBadConstructorIString);
    
    ASSERT(!JSObjectSetPrivate(myConstructor, (void*)1));
    ASSERT(!JSObjectGetPrivate(myConstructor));
    
    string = JSStringCreateWithUTF8CString("Base");
    JSObjectRef baseConstructor = JSObjectMakeConstructor(context, Base_class(context), NULL);
    JSObjectSetProperty(context, globalObject, string, baseConstructor, kJSPropertyAttributeNone, NULL);
    JSStringRelease(string);
    
    string = JSStringCreateWithUTF8CString("Derived");
    JSObjectRef derivedConstructor = JSObjectMakeConstructor(context, Derived_class(context), NULL);
    JSObjectSetProperty(context, globalObject, string, derivedConstructor, kJSPropertyAttributeNone, NULL);
    JSStringRelease(string);
    
    string = JSStringCreateWithUTF8CString("Derived2");
    JSObjectRef derived2Constructor = JSObjectMakeConstructor(context, Derived2_class(context), NULL);
    JSObjectSetProperty(context, globalObject, string, derived2Constructor, kJSPropertyAttributeNone, NULL);
    JSStringRelease(string);

    o = JSObjectMake(context, NULL, NULL);
    JSObjectSetProperty(context, o, jsOneIString, JSValueMakeNumber(context, 1), kJSPropertyAttributeNone, NULL);
    JSObjectSetProperty(context, o, jsCFIString,  JSValueMakeNumber(context, 1), kJSPropertyAttributeDontEnum, NULL);
    JSPropertyNameArrayRef nameArray = JSObjectCopyPropertyNames(context, o);
    size_t expectedCount = JSPropertyNameArrayGetCount(nameArray);
    size_t count;
    for (count = 0; count < expectedCount; ++count)
        JSPropertyNameArrayGetNameAtIndex(nameArray, count);
    JSPropertyNameArrayRelease(nameArray);
    ASSERT(count == 1); // jsCFString should not be enumerated

    JSValueRef argumentsArrayValues[] = { JSValueMakeNumber(context, 10), JSValueMakeNumber(context, 20) };
    o = JSObjectMakeArray(context, sizeof(argumentsArrayValues) / sizeof(JSValueRef), argumentsArrayValues, NULL);
    string = JSStringCreateWithUTF8CString("length");
    v = JSObjectGetProperty(context, o, string, NULL);
    assertEqualsAsNumber(v, 2);
    v = JSObjectGetPropertyAtIndex(context, o, 0, NULL);
    assertEqualsAsNumber(v, 10);
    v = JSObjectGetPropertyAtIndex(context, o, 1, NULL);
    assertEqualsAsNumber(v, 20);

    o = JSObjectMakeArray(context, 0, NULL, NULL);
    v = JSObjectGetProperty(context, o, string, NULL);
    assertEqualsAsNumber(v, 0);
    JSStringRelease(string);

    JSValueRef argumentsDateValues[] = { JSValueMakeNumber(context, 0) };
    o = JSObjectMakeDate(context, 1, argumentsDateValues, NULL);
    if (timeZoneIsPST())
        assertEqualsAsUTF8String(o, "Wed Dec 31 1969 16:00:00 GMT-0800 (PST)");

    string = JSStringCreateWithUTF8CString("an error message");
    JSValueRef argumentsErrorValues[] = { JSValueMakeString(context, string) };
    o = JSObjectMakeError(context, 1, argumentsErrorValues, NULL);
    assertEqualsAsUTF8String(o, "Error: an error message");
    JSStringRelease(string);

    string = JSStringCreateWithUTF8CString("foo");
    JSStringRef string2 = JSStringCreateWithUTF8CString("gi");
    JSValueRef argumentsRegExpValues[] = { JSValueMakeString(context, string), JSValueMakeString(context, string2) };
    o = JSObjectMakeRegExp(context, 2, argumentsRegExpValues, NULL);
    assertEqualsAsUTF8String(o, "/foo/gi");
    JSStringRelease(string);
    JSStringRelease(string2);

    JSClassDefinition nullDefinition = kJSClassDefinitionEmpty;
    nullDefinition.attributes = kJSClassAttributeNoAutomaticPrototype;
    JSClassRef nullClass = JSClassCreate(&nullDefinition);
    JSClassRelease(nullClass);
    
    nullDefinition = kJSClassDefinitionEmpty;
    nullClass = JSClassCreate(&nullDefinition);
    JSClassRelease(nullClass);

    functionBody = JSStringCreateWithUTF8CString("return this;");
    function = JSObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, NULL);
    JSStringRelease(functionBody);
    v = JSObjectCallAsFunction(context, function, NULL, 0, NULL, NULL);
    ASSERT(JSValueIsEqual(context, v, globalObject, NULL));
    v = JSObjectCallAsFunction(context, function, o, 0, NULL, NULL);
    ASSERT(JSValueIsEqual(context, v, o, NULL));

    functionBody = JSStringCreateWithUTF8CString("return eval(\"this\");");
    function = JSObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, NULL);
    JSStringRelease(functionBody);
    v = JSObjectCallAsFunction(context, function, NULL, 0, NULL, NULL);
    ASSERT(JSValueIsEqual(context, v, globalObject, NULL));
    v = JSObjectCallAsFunction(context, function, o, 0, NULL, NULL);
    ASSERT(JSValueIsEqual(context, v, o, NULL));

    const char* thisScript = "this;";
    JSStringRef script = JSStringCreateWithUTF8CString(thisScript);
    v = JSEvaluateScript(context, script, NULL, NULL, 1, NULL);
    ASSERT(JSValueIsEqual(context, v, globalObject, NULL));
    v = JSEvaluateScript(context, script, o, NULL, 1, NULL);
    ASSERT(JSValueIsEqual(context, v, o, NULL));
    JSStringRelease(script);

    JSScriptRef scriptObject = JSScriptCreateReferencingImmortalASCIIText(contextGroup, 0, 0, thisScript, strlen(thisScript), 0, 0);
    v = JSScriptEvaluate(context, scriptObject, NULL, NULL);
    ASSERT(JSValueIsEqual(context, v, globalObject, NULL));
    v = JSScriptEvaluate(context, scriptObject, o, NULL);
    ASSERT(JSValueIsEqual(context, v, o, NULL));
    JSScriptRelease(scriptObject);

    script = JSStringCreateWithUTF8CString("eval(this);");
    v = JSEvaluateScript(context, script, NULL, NULL, 1, NULL);
    ASSERT(JSValueIsEqual(context, v, globalObject, NULL));
    v = JSEvaluateScript(context, script, o, NULL, 1, NULL);
    ASSERT(JSValueIsEqual(context, v, o, NULL));
    JSStringRelease(script);

    // Verify that creating a constructor for a class with no static functions does not trigger
    // an assert inside putDirect or lead to a crash during GC. <https://bugs.webkit.org/show_bug.cgi?id=25785>
    nullDefinition = kJSClassDefinitionEmpty;
    nullClass = JSClassCreate(&nullDefinition);
    JSObjectMakeConstructor(context, nullClass, 0);
    JSClassRelease(nullClass);

    char* scriptUTF8 = createStringWithContentsOfFile(scriptPath);
    if (!scriptUTF8) {
        printf("FAIL: Test script could not be loaded.\n");
        failed = 1;
    } else {
        JSStringRef url = JSStringCreateWithUTF8CString(scriptPath);
        JSStringRef script = JSStringCreateWithUTF8CString(scriptUTF8);
        JSStringRef errorMessage = 0;
        int errorLine = 0;
        JSScriptRef scriptObject = JSScriptCreateFromString(contextGroup, url, 1, script, &errorMessage, &errorLine);
        ASSERT((!scriptObject) != (!errorMessage));
        if (!scriptObject) {
            printf("FAIL: Test script did not parse\n\t%s:%d\n\t", scriptPath, errorLine);
            CFStringRef errorCF = JSStringCopyCFString(kCFAllocatorDefault, errorMessage);
            CFShow(errorCF);
            CFRelease(errorCF);
            JSStringRelease(errorMessage);
            failed = 1;
        }

        JSStringRelease(script);
        result = scriptObject ? JSScriptEvaluate(context, scriptObject, 0, &exception) : 0;
        if (result && JSValueIsUndefined(context, result))
            printf("PASS: Test script executed successfully.\n");
        else {
            printf("FAIL: Test script returned unexpected value:\n");
            JSStringRef exceptionIString = JSValueToStringCopy(context, exception, NULL);
            CFStringRef exceptionCF = JSStringCopyCFString(kCFAllocatorDefault, exceptionIString);
            CFShow(exceptionCF);
            CFRelease(exceptionCF);
            JSStringRelease(exceptionIString);
            failed = 1;
        }
        JSScriptRelease(scriptObject);
        free(scriptUTF8);
    }

#if PLATFORM(MAC) || PLATFORM(IOS)
    JSStringRef currentCPUTimeStr = JSStringCreateWithUTF8CString("currentCPUTime");
    JSObjectRef currentCPUTimeFunction = JSObjectMakeFunctionWithCallback(context, currentCPUTimeStr, currentCPUTime_callAsFunction);
    JSObjectSetProperty(context, globalObject, currentCPUTimeStr, currentCPUTimeFunction, kJSPropertyAttributeNone, NULL); 
    JSStringRelease(currentCPUTimeStr);

    /* Test script timeout: */
    JSContextGroupSetExecutionTimeLimit(contextGroup, .10f, shouldTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .150) break; } ";
        JSStringRef script = JSStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        shouldTerminateCallbackWasCalled = false;
        startTime = currentCPUTime();
        v = JSEvaluateScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) < .150f) && shouldTerminateCallbackWasCalled)
            printf("PASS: script timed out as expected.\n");
        else {
            if (!((endTime - startTime) < .150f))
                printf("FAIL: script did not timed out as expected.\n");
            if (!shouldTerminateCallbackWasCalled)
                printf("FAIL: script timeout callback was not called.\n");
            failed = true;
        }

        if (!exception) {
            printf("FAIL: TerminatedExecutionException was not thrown.\n");
            failed = true;
        }
    }

    /* Test the script timeout's TerminatedExecutionException should NOT be catchable: */
    JSContextGroupSetExecutionTimeLimit(contextGroup, 0.10f, shouldTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); try { while (true) { if (currentCPUTime() - startTime > .150) break; } } catch(e) { }";
        JSStringRef script = JSStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        shouldTerminateCallbackWasCalled = false;
        startTime = currentCPUTime();
        v = JSEvaluateScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) >= .150f) || !shouldTerminateCallbackWasCalled) {
            if (!((endTime - startTime) < .150f))
                printf("FAIL: script did not timed out as expected.\n");
            if (!shouldTerminateCallbackWasCalled)
                printf("FAIL: script timeout callback was not called.\n");
            failed = true;
        }

        if (exception)
            printf("PASS: TerminatedExecutionException was not catchable as expected.\n");
        else {
            printf("FAIL: TerminatedExecutionException was caught.\n");
            failed = true;
        }
    }

    /* Test script timeout with no callback: */
    JSContextGroupSetExecutionTimeLimit(contextGroup, .10f, 0, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .150) break; } ";
        JSStringRef script = JSStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        startTime = currentCPUTime();
        v = JSEvaluateScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) < .150f) && shouldTerminateCallbackWasCalled)
            printf("PASS: script timed out as expected when no callback is specified.\n");
        else {
            if (!((endTime - startTime) < .150f))
                printf("FAIL: script did not timed out as expected when no callback is specified.\n");
            failed = true;
        }

        if (!exception) {
            printf("FAIL: TerminatedExecutionException was not thrown.\n");
            failed = true;
        }
    }

    /* Test script timeout cancellation: */
    JSContextGroupSetExecutionTimeLimit(contextGroup, 0.10f, cancelTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .150) break; } ";
        JSStringRef script = JSStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        startTime = currentCPUTime();
        v = JSEvaluateScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) >= .150f) && cancelTerminateCallbackWasCalled && !exception)
            printf("PASS: script timeout was cancelled as expected.\n");
        else {
            if (((endTime - startTime) < .150) || exception)
                printf("FAIL: script timeout was not cancelled.\n");
            if (!cancelTerminateCallbackWasCalled)
                printf("FAIL: script timeout callback was not called.\n");
            failed = true;
        }

        if (exception) {
            printf("FAIL: Unexpected TerminatedExecutionException thrown.\n");
            failed = true;
        }
    }

    /* Test script timeout extension: */
    JSContextGroupSetExecutionTimeLimit(contextGroup, 0.100f, extendTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .500) break; } ";
        JSStringRef script = JSStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        double deltaTime;
        exception = NULL;
        startTime = currentCPUTime();
        v = JSEvaluateScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();
        deltaTime = endTime - startTime;

        if ((deltaTime >= .300f) && (deltaTime < .500f) && (extendTerminateCallbackCalled == 2) && exception)
            printf("PASS: script timeout was extended as expected.\n");
        else {
            if (deltaTime < .200f)
                printf("FAIL: script timeout was not extended as expected.\n");
            else if (deltaTime >= .500f)
                printf("FAIL: script did not timeout.\n");

            if (extendTerminateCallbackCalled < 1)
                printf("FAIL: script timeout callback was not called.\n");
            if (extendTerminateCallbackCalled < 2)
                printf("FAIL: script timeout callback was not called after timeout extension.\n");

            if (!exception)
                printf("FAIL: TerminatedExecutionException was not thrown during timeout extension test.\n");

            failed = true;
        }
    }
#endif /* PLATFORM(MAC) || PLATFORM(IOS) */

    // Clear out local variables pointing at JSObjectRefs to allow their values to be collected
    function = NULL;
    v = NULL;
    o = NULL;
    globalObject = NULL;
    myConstructor = NULL;

    JSStringRelease(jsEmptyIString);
    JSStringRelease(jsOneIString);
    JSStringRelease(jsCFIString);
    JSStringRelease(jsCFEmptyIString);
    JSStringRelease(jsCFIStringWithCharacters);
    JSStringRelease(jsCFEmptyIStringWithCharacters);
    JSStringRelease(goodSyntax);
    JSStringRelease(badSyntax);

    JSGlobalContextRelease(context);
    JSClassRelease(globalObjectClass);

    // Test for an infinite prototype chain that used to be created. This test
    // passes if the call to JSObjectHasProperty() does not hang.

    JSClassDefinition prototypeLoopClassDefinition = kJSClassDefinitionEmpty;
    prototypeLoopClassDefinition.staticFunctions = globalObject_staticFunctions;
    JSClassRef prototypeLoopClass = JSClassCreate(&prototypeLoopClassDefinition);
    JSGlobalContextRef prototypeLoopContext = JSGlobalContextCreateInGroup(NULL, prototypeLoopClass);

    JSStringRef nameProperty = JSStringCreateWithUTF8CString("name");
    JSObjectHasProperty(prototypeLoopContext, JSContextGetGlobalObject(prototypeLoopContext), nameProperty);

    JSGlobalContextRelease(prototypeLoopContext);
    JSClassRelease(prototypeLoopClass);

    printf("PASS: Infinite prototype chain does not occur.\n");

    if (checkForCycleInPrototypeChain())
        printf("PASS: A cycle in a prototype chain can't be created.\n");
    else {
        printf("FAIL: A cycle in a prototype chain can be created.\n");
        failed = true;
    }

    if (failed) {
        printf("FAIL: Some tests failed.\n");
        return 1;
    }

    printf("PASS: Program exited normally.\n");
    return 0;
}

static char* createStringWithContentsOfFile(const char* fileName)
{
    char* buffer;
    
    size_t buffer_size = 0;
    size_t buffer_capacity = 1024;
    buffer = (char*)malloc(buffer_capacity);
    
    FILE* f = fopen(fileName, "r");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", fileName);
        return 0;
    }
    
    while (!feof(f) && !ferror(f)) {
        buffer_size += fread(buffer + buffer_size, 1, buffer_capacity - buffer_size, f);
        if (buffer_size == buffer_capacity) { // guarantees space for trailing '\0'
            buffer_capacity *= 2;
            buffer = (char*)realloc(buffer, buffer_capacity);
            ASSERT(buffer);
        }
        
        ASSERT(buffer_size < buffer_capacity);
    }
    fclose(f);
    buffer[buffer_size] = '\0';
    
    return buffer;
}
