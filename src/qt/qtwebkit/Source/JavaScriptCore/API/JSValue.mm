/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#import "APICast.h"
#import "APIShims.h"
#import "DateInstance.h"
#import "Error.h"
#import "JavaScriptCore.h"
#import "JSContextInternal.h"
#import "JSVirtualMachineInternal.h"
#import "JSValueInternal.h"
#import "JSWrapperMap.h"
#import "ObjcRuntimeExtras.h"
#import "Operations.h"
#import "JSCJSValue.h"
#import <wtf/HashMap.h>
#import <wtf/HashSet.h>
#import <wtf/Vector.h>
#import <wtf/TCSpinLock.h>
#import <wtf/text/WTFString.h>
#import <wtf/text/StringHash.h>

#if JSC_OBJC_API_ENABLED

NSString * const JSPropertyDescriptorWritableKey = @"writable";
NSString * const JSPropertyDescriptorEnumerableKey = @"enumerable";
NSString * const JSPropertyDescriptorConfigurableKey = @"configurable";
NSString * const JSPropertyDescriptorValueKey = @"value";
NSString * const JSPropertyDescriptorGetKey = @"get";
NSString * const JSPropertyDescriptorSetKey = @"set";

@implementation JSValue {
    JSValueRef m_value;
}

- (JSValueRef)JSValueRef
{
    return m_value;
}

+ (JSValue *)valueWithObject:(id)value inContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:objectToValue(context, value) inContext:context];
}

+ (JSValue *)valueWithBool:(BOOL)value inContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSValueMakeBoolean([context JSGlobalContextRef], value) inContext:context];
}

+ (JSValue *)valueWithDouble:(double)value inContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSValueMakeNumber([context JSGlobalContextRef], value) inContext:context];
}

+ (JSValue *)valueWithInt32:(int32_t)value inContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSValueMakeNumber([context JSGlobalContextRef], value) inContext:context];
}

+ (JSValue *)valueWithUInt32:(uint32_t)value inContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSValueMakeNumber([context JSGlobalContextRef], value) inContext:context];
}

+ (JSValue *)valueWithNewObjectInContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSObjectMake([context JSGlobalContextRef], 0, 0) inContext:context];
}

+ (JSValue *)valueWithNewArrayInContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSObjectMakeArray([context JSGlobalContextRef], 0, NULL, 0) inContext:context];
}

+ (JSValue *)valueWithNewRegularExpressionFromPattern:(NSString *)pattern flags:(NSString *)flags inContext:(JSContext *)context
{
    JSStringRef patternString = JSStringCreateWithCFString((CFStringRef)pattern);
    JSStringRef flagsString = JSStringCreateWithCFString((CFStringRef)flags);
    JSValueRef arguments[2] = { JSValueMakeString([context JSGlobalContextRef], patternString), JSValueMakeString([context JSGlobalContextRef], flagsString) };
    JSStringRelease(patternString);
    JSStringRelease(flagsString);

    return [JSValue valueWithJSValueRef:JSObjectMakeRegExp([context JSGlobalContextRef], 2, arguments, 0) inContext:context];
}

+ (JSValue *)valueWithNewErrorFromMessage:(NSString *)message inContext:(JSContext *)context
{
    JSStringRef string = JSStringCreateWithCFString((CFStringRef)message);
    JSValueRef argument = JSValueMakeString([context JSGlobalContextRef], string);
    JSStringRelease(string);

    return [JSValue valueWithJSValueRef:JSObjectMakeError([context JSGlobalContextRef], 1, &argument, 0) inContext:context];
}

+ (JSValue *)valueWithNullInContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSValueMakeNull([context JSGlobalContextRef]) inContext:context];
}

+ (JSValue *)valueWithUndefinedInContext:(JSContext *)context
{
    return [JSValue valueWithJSValueRef:JSValueMakeUndefined([context JSGlobalContextRef]) inContext:context];
}

- (id)toObject
{
    return valueToObject(_context, m_value);
}

- (id)toObjectOfClass:(Class)expectedClass
{
    id result = [self toObject];
    return [result isKindOfClass:expectedClass] ? result : nil;
}

- (BOOL)toBool
{
    return JSValueToBoolean([_context JSGlobalContextRef], m_value);
}

- (double)toDouble
{
    JSValueRef exception = 0;
    double result = JSValueToNumber([_context JSGlobalContextRef], m_value, &exception);
    if (exception) {
        [_context notifyException:exception];
        return std::numeric_limits<double>::quiet_NaN();
    }

    return result;
}

- (int32_t)toInt32
{
    return JSC::toInt32([self toDouble]);
}

- (uint32_t)toUInt32
{
    return JSC::toUInt32([self toDouble]);
}

- (NSNumber *)toNumber
{
    JSValueRef exception = 0;
    id result = valueToNumber([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSString *)toString
{
    JSValueRef exception = 0;
    id result = valueToString([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSDate *)toDate
{
    JSValueRef exception = 0;
    id result = valueToDate([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSArray *)toArray
{
    JSValueRef exception = 0;
    id result = valueToArray([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSDictionary *)toDictionary
{
    JSValueRef exception = 0;
    id result = valueToDictionary([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (JSValue *)valueForProperty:(NSString *)propertyName
{
    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    JSStringRef name = JSStringCreateWithCFString((CFStringRef)propertyName);
    JSValueRef result = JSObjectGetProperty([_context JSGlobalContextRef], object, name, &exception);
    JSStringRelease(name);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [JSValue valueWithJSValueRef:result inContext:_context];
}

- (void)setValue:(id)value forProperty:(NSString *)propertyName
{
    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception) {
        [_context notifyException:exception];
        return;
    }

    JSStringRef name = JSStringCreateWithCFString((CFStringRef)propertyName);
    JSObjectSetProperty([_context JSGlobalContextRef], object, name, objectToValue(_context, value), 0, &exception);
    JSStringRelease(name);
    if (exception) {
        [_context notifyException:exception];
        return;
    }
}

- (BOOL)deleteProperty:(NSString *)propertyName
{
    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    JSStringRef name = JSStringCreateWithCFString((CFStringRef)propertyName);
    BOOL result = JSObjectDeleteProperty([_context JSGlobalContextRef], object, name, &exception);
    JSStringRelease(name);
    if (exception)
        return [_context boolFromNotifyException:exception];

    return result;
}

- (BOOL)hasProperty:(NSString *)propertyName
{
    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    JSStringRef name = JSStringCreateWithCFString((CFStringRef)propertyName);
    BOOL result = JSObjectHasProperty([_context JSGlobalContextRef], object, name);
    JSStringRelease(name);
    return result;
}

- (void)defineProperty:(NSString *)property descriptor:(id)descriptor
{
    [[_context globalObject][@"Object"] invokeMethod:@"defineProperty" withArguments:@[ self, property, descriptor ]];
}

- (JSValue *)valueAtIndex:(NSUInteger)index
{
    // Properties that are higher than an unsigned value can hold are converted to a double then inserted as a normal property.
    // Indices that are bigger than the max allowed index size (UINT_MAX - 1) will be handled internally in get().
    if (index != (unsigned)index)
        return [self valueForProperty:[[JSValue valueWithDouble:index inContext:_context] toString]];

    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    JSValueRef result = JSObjectGetPropertyAtIndex([_context JSGlobalContextRef], object, (unsigned)index, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [JSValue valueWithJSValueRef:result inContext:_context];
}

- (void)setValue:(id)value atIndex:(NSUInteger)index
{
    // Properties that are higher than an unsigned value can hold are converted to a double, then inserted as a normal property.
    // Indices that are bigger than the max allowed index size (UINT_MAX - 1) will be handled internally in putByIndex().
    if (index != (unsigned)index)
        return [self setValue:value forProperty:[[JSValue valueWithDouble:index inContext:_context] toString]];

    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception) {
        [_context notifyException:exception];
        return;
    }

    JSObjectSetPropertyAtIndex([_context JSGlobalContextRef], object, (unsigned)index, objectToValue(_context, value), &exception);
    if (exception) {
        [_context notifyException:exception];
        return;
    }
}

- (BOOL)isUndefined
{
    return JSValueIsUndefined([_context JSGlobalContextRef], m_value);
}

- (BOOL)isNull
{
    return JSValueIsNull([_context JSGlobalContextRef], m_value);
}

- (BOOL)isBoolean
{
    return JSValueIsBoolean([_context JSGlobalContextRef], m_value);
}

- (BOOL)isNumber
{
    return JSValueIsNumber([_context JSGlobalContextRef], m_value);
}

- (BOOL)isString
{
    return JSValueIsString([_context JSGlobalContextRef], m_value);
}

- (BOOL)isObject
{
    return JSValueIsObject([_context JSGlobalContextRef], m_value);
}

- (BOOL)isEqualToObject:(id)value
{
    return JSValueIsStrictEqual([_context JSGlobalContextRef], m_value, objectToValue(_context, value));
}

- (BOOL)isEqualWithTypeCoercionToObject:(id)value
{
    JSValueRef exception = 0;
    BOOL result = JSValueIsEqual([_context JSGlobalContextRef], m_value, objectToValue(_context, value), &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    return result;
}

- (BOOL)isInstanceOf:(id)value
{
    JSValueRef exception = 0;
    JSObjectRef constructor = JSValueToObject([_context JSGlobalContextRef], objectToValue(_context, value), &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    BOOL result = JSValueIsInstanceOfConstructor([_context JSGlobalContextRef], m_value, constructor, &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    return result;
}

- (JSValue *)callWithArguments:(NSArray *)argumentArray
{
    NSUInteger argumentCount = [argumentArray count];
    JSValueRef arguments[argumentCount];
    for (unsigned i = 0; i < argumentCount; ++i)
        arguments[i] = objectToValue(_context, [argumentArray objectAtIndex:i]);

    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    JSValueRef result = JSObjectCallAsFunction([_context JSGlobalContextRef], object, 0, argumentCount, arguments, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [JSValue valueWithJSValueRef:result inContext:_context];
}

- (JSValue *)constructWithArguments:(NSArray *)argumentArray
{
    NSUInteger argumentCount = [argumentArray count];
    JSValueRef arguments[argumentCount];
    for (unsigned i = 0; i < argumentCount; ++i)
        arguments[i] = objectToValue(_context, [argumentArray objectAtIndex:i]);

    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    JSObjectRef result = JSObjectCallAsConstructor([_context JSGlobalContextRef], object, argumentCount, arguments, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [JSValue valueWithJSValueRef:result inContext:_context];
}

- (JSValue *)invokeMethod:(NSString *)method withArguments:(NSArray *)arguments
{
    NSUInteger argumentCount = [arguments count];
    JSValueRef argumentArray[argumentCount];
    for (unsigned i = 0; i < argumentCount; ++i)
        argumentArray[i] = objectToValue(_context, [arguments objectAtIndex:i]);

    JSValueRef exception = 0;
    JSObjectRef thisObject = JSValueToObject([_context JSGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    JSStringRef name = JSStringCreateWithCFString((CFStringRef)method);
    JSValueRef function = JSObjectGetProperty([_context JSGlobalContextRef], thisObject, name, &exception);
    JSStringRelease(name);
    if (exception)
        return [_context valueFromNotifyException:exception];

    JSObjectRef object = JSValueToObject([_context JSGlobalContextRef], function, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    JSValueRef result = JSObjectCallAsFunction([_context JSGlobalContextRef], object, thisObject, argumentCount, argumentArray, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [JSValue valueWithJSValueRef:result inContext:_context];
}

@end

@implementation JSValue(StructSupport)

- (CGPoint)toPoint
{
    return (CGPoint){
        [self[@"x"] toDouble],
        [self[@"y"] toDouble]
    };
}

- (NSRange)toRange
{
    return (NSRange){
        [[self[@"location"] toNumber] unsignedIntegerValue],
        [[self[@"length"] toNumber] unsignedIntegerValue]
    };
}

- (CGRect)toRect
{
    return (CGRect){
        [self toPoint],
        [self toSize]
    };
}

- (CGSize)toSize
{
    return (CGSize){
        [self[@"width"] toDouble],
        [self[@"height"] toDouble]
    };
}

+ (JSValue *)valueWithPoint:(CGPoint)point inContext:(JSContext *)context
{
    return [JSValue valueWithObject:@{
        @"x":@(point.x),
        @"y":@(point.y)
    } inContext:context];
}

+ (JSValue *)valueWithRange:(NSRange)range inContext:(JSContext *)context
{
    return [JSValue valueWithObject:@{
        @"location":@(range.location),
        @"length":@(range.length)
    } inContext:context];
}

+ (JSValue *)valueWithRect:(CGRect)rect inContext:(JSContext *)context
{
    return [JSValue valueWithObject:@{
        @"x":@(rect.origin.x),
        @"y":@(rect.origin.y),
        @"width":@(rect.size.width),
        @"height":@(rect.size.height)
    } inContext:context];
}

+ (JSValue *)valueWithSize:(CGSize)size inContext:(JSContext *)context
{
    return [JSValue valueWithObject:@{
        @"width":@(size.width),
        @"height":@(size.height)
    } inContext:context];
}

@end

@implementation JSValue(SubscriptSupport)

- (JSValue *)objectForKeyedSubscript:(id)key
{
    if (![key isKindOfClass:[NSString class]]) {
        key = [[JSValue valueWithObject:key inContext:_context] toString];
        if (!key)
            return [JSValue valueWithUndefinedInContext:_context];
    }

    return [self valueForProperty:(NSString *)key];
}

- (JSValue *)objectAtIndexedSubscript:(NSUInteger)index
{
    return [self valueAtIndex:index];
}

- (void)setObject:(id)object forKeyedSubscript:(NSObject <NSCopying> *)key
{
    if (![key isKindOfClass:[NSString class]]) {
        key = [[JSValue valueWithObject:key inContext:_context] toString];
        if (!key)
            return;
    }

    [self setValue:object forProperty:(NSString *)key];
}

- (void)setObject:(id)object atIndexedSubscript:(NSUInteger)index
{
    [self setValue:object atIndex:index];
}

@end

inline bool isDate(JSObjectRef object, JSGlobalContextRef context)
{
    JSC::APIEntryShim entryShim(toJS(context));
    return toJS(object)->inherits(&JSC::DateInstance::s_info);
}

inline bool isArray(JSObjectRef object, JSGlobalContextRef context)
{
    JSC::APIEntryShim entryShim(toJS(context));
    return toJS(object)->inherits(&JSC::JSArray::s_info);
}

@implementation JSValue(Internal)

enum ConversionType {
    ContainerNone,
    ContainerArray,
    ContainerDictionary
};

class JSContainerConvertor {
public:
    struct Task {
        JSValueRef js;
        id objc;
        ConversionType type;
    };

    JSContainerConvertor(JSGlobalContextRef context)
        : m_context(context)
    {
    }

    id convert(JSValueRef property);
    void add(Task);
    Task take();
    bool isWorkListEmpty() const { return !m_worklist.size(); }

private:
    JSGlobalContextRef m_context;
    HashMap<JSValueRef, id> m_objectMap;
    Vector<Task> m_worklist;
};

inline id JSContainerConvertor::convert(JSValueRef value)
{
    HashMap<JSValueRef, id>::iterator iter = m_objectMap.find(value);
    if (iter != m_objectMap.end())
        return iter->value;

    Task result = valueToObjectWithoutCopy(m_context, value);
    if (result.js)
        add(result);
    return result.objc;
}

void JSContainerConvertor::add(Task task)
{
    m_objectMap.add(task.js, task.objc);
    if (task.type != ContainerNone)
        m_worklist.append(task);
}

JSContainerConvertor::Task JSContainerConvertor::take()
{
    ASSERT(!isWorkListEmpty());
    Task last = m_worklist.last();
    m_worklist.removeLast();
    return last;
}

static JSContainerConvertor::Task valueToObjectWithoutCopy(JSGlobalContextRef context, JSValueRef value)
{
    if (!JSValueIsObject(context, value)) {
        id primitive;
        if (JSValueIsBoolean(context, value))
            primitive = JSValueToBoolean(context, value) ? @YES : @NO;
        else if (JSValueIsNumber(context, value)) {
            // Normalize the number, so it will unique correctly in the hash map -
            // it's nicer not to leak this internal implementation detail!
            value = JSValueMakeNumber(context, JSValueToNumber(context, value, 0));
            primitive = [NSNumber numberWithDouble:JSValueToNumber(context, value, 0)];
        } else if (JSValueIsString(context, value)) {
            // Would be nice to unique strings, too.
            JSStringRef jsstring = JSValueToStringCopy(context, value, 0);
            NSString * stringNS = (NSString *)JSStringCopyCFString(kCFAllocatorDefault, jsstring);
            JSStringRelease(jsstring);
            primitive = [stringNS autorelease];
        } else if (JSValueIsNull(context, value))
            primitive = [NSNull null];
        else {
            ASSERT(JSValueIsUndefined(context, value));
            primitive = nil;
        }
        return (JSContainerConvertor::Task){ value, primitive, ContainerNone };
    }

    JSObjectRef object = JSValueToObject(context, value, 0);

    if (id wrapped = tryUnwrapObjcObject(context, object))
        return (JSContainerConvertor::Task){ object, wrapped, ContainerNone };

    if (isDate(object, context))
        return (JSContainerConvertor::Task){ object, [NSDate dateWithTimeIntervalSince1970:JSValueToNumber(context, object, 0)], ContainerNone };

    if (isArray(object, context))
        return (JSContainerConvertor::Task){ object, [NSMutableArray array], ContainerArray };

    return (JSContainerConvertor::Task){ object, [NSMutableDictionary dictionary], ContainerDictionary };
}

static id containerValueToObject(JSGlobalContextRef context, JSContainerConvertor::Task task)
{
    ASSERT(task.type != ContainerNone);
    JSContainerConvertor convertor(context);
    convertor.add(task);
    ASSERT(!convertor.isWorkListEmpty());
    
    do {
        JSContainerConvertor::Task current = convertor.take();
        ASSERT(JSValueIsObject(context, current.js));
        JSObjectRef js = JSValueToObject(context, current.js, 0);

        if (current.type == ContainerArray) {
            ASSERT([current.objc isKindOfClass:[NSMutableArray class]]);
            NSMutableArray *array = (NSMutableArray *)current.objc;
        
            JSStringRef lengthString = JSStringCreateWithUTF8CString("length");
            unsigned length = JSC::toUInt32(JSValueToNumber(context, JSObjectGetProperty(context, js, lengthString, 0), 0));
            JSStringRelease(lengthString);

            for (unsigned i = 0; i < length; ++i) {
                id objc = convertor.convert(JSObjectGetPropertyAtIndex(context, js, i, 0));
                [array addObject:objc ? objc : [NSNull null]];
            }
        } else {
            ASSERT([current.objc isKindOfClass:[NSMutableDictionary class]]);
            NSMutableDictionary *dictionary = (NSMutableDictionary *)current.objc;

            JSPropertyNameArrayRef propertyNameArray = JSObjectCopyPropertyNames(context, js);
            size_t length = JSPropertyNameArrayGetCount(propertyNameArray);

            for (size_t i = 0; i < length; ++i) {
                JSStringRef propertyName = JSPropertyNameArrayGetNameAtIndex(propertyNameArray, i);
                if (id objc = convertor.convert(JSObjectGetProperty(context, js, propertyName, 0)))
                    dictionary[[(NSString *)JSStringCopyCFString(kCFAllocatorDefault, propertyName) autorelease]] = objc;
            }

            JSPropertyNameArrayRelease(propertyNameArray);
        }

    } while (!convertor.isWorkListEmpty());

    return task.objc;
}

id valueToObject(JSContext *context, JSValueRef value)
{
    JSContainerConvertor::Task result = valueToObjectWithoutCopy([context JSGlobalContextRef], value);
    if (result.type == ContainerNone)
        return result.objc;
    return containerValueToObject([context JSGlobalContextRef], result);
}

id valueToNumber(JSGlobalContextRef context, JSValueRef value, JSValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSNumber class]])
            return wrapped;
    }

    if (JSValueIsBoolean(context, value))
        return JSValueToBoolean(context, value) ? @YES : @NO;

    double result = JSValueToNumber(context, value, exception);
    return [NSNumber numberWithDouble:*exception ? std::numeric_limits<double>::quiet_NaN() : result];
}

id valueToString(JSGlobalContextRef context, JSValueRef value, JSValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSString class]])
            return wrapped;
    }

    JSStringRef jsstring = JSValueToStringCopy(context, value, exception);
    if (*exception) {
        ASSERT(!jsstring);
        return nil;
    }

    NSString *stringNS = [(NSString *)JSStringCopyCFString(kCFAllocatorDefault, jsstring) autorelease];
    JSStringRelease(jsstring);
    return stringNS;
}

id valueToDate(JSGlobalContextRef context, JSValueRef value, JSValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSDate class]])
            return wrapped;
    }

    double result = JSValueToNumber(context, value, exception);
    return *exception ? nil : [NSDate dateWithTimeIntervalSince1970:result];
}

id valueToArray(JSGlobalContextRef context, JSValueRef value, JSValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSArray class]])
            return wrapped;
    }

    if (JSValueIsObject(context, value))
        return containerValueToObject(context, (JSContainerConvertor::Task){ value, [NSMutableArray array], ContainerArray});

    if (!(JSValueIsNull(context, value) || JSValueIsUndefined(context, value)))
        *exception = toRef(JSC::createTypeError(toJS(context), "Cannot convert primitive to NSArray"));
    return nil;
}

id valueToDictionary(JSGlobalContextRef context, JSValueRef value, JSValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSDictionary class]])
            return wrapped;
    }

    if (JSValueIsObject(context, value))
        return containerValueToObject(context, (JSContainerConvertor::Task){ value, [NSMutableDictionary dictionary], ContainerDictionary});

    if (!(JSValueIsNull(context, value) || JSValueIsUndefined(context, value)))
        *exception = toRef(JSC::createTypeError(toJS(context), "Cannot convert primitive to NSDictionary"));
    return nil;
}

class ObjcContainerConvertor {
public:
    struct Task {
        id objc;
        JSValueRef js;
        ConversionType type;
    };

    ObjcContainerConvertor(JSContext *context)
        : m_context(context)
    {
    }

    JSValueRef convert(id object);
    void add(Task);
    Task take();
    bool isWorkListEmpty() const { return !m_worklist.size(); }

private:
    JSContext *m_context;
    HashMap<id, JSValueRef> m_objectMap;
    Vector<Task> m_worklist;
};

JSValueRef ObjcContainerConvertor::convert(id object)
{
    ASSERT(object);

    auto it = m_objectMap.find(object);
    if (it != m_objectMap.end())
        return it->value;

    ObjcContainerConvertor::Task task = objectToValueWithoutCopy(m_context, object);
    add(task);
    return task.js;
}

void ObjcContainerConvertor::add(ObjcContainerConvertor::Task task)
{
    m_objectMap.add(task.objc, task.js);
    if (task.type != ContainerNone)
        m_worklist.append(task);
}

ObjcContainerConvertor::Task ObjcContainerConvertor::take()
{
    ASSERT(!isWorkListEmpty());
    Task last = m_worklist.last();
    m_worklist.removeLast();
    return last;
}

inline bool isNSBoolean(id object)
{
    ASSERT([@YES class] == [@NO class]);
    ASSERT([@YES class] != [NSNumber class]);
    ASSERT([[@YES class] isSubclassOfClass:[NSNumber class]]);
    return [object isKindOfClass:[@YES class]];
}

static ObjcContainerConvertor::Task objectToValueWithoutCopy(JSContext *context, id object)
{
    JSGlobalContextRef contextRef = [context JSGlobalContextRef];

    if (!object)
        return (ObjcContainerConvertor::Task){ object, JSValueMakeUndefined(contextRef), ContainerNone };

    if (!class_conformsToProtocol(object_getClass(object), getJSExportProtocol())) {
        if ([object isKindOfClass:[NSArray class]])
            return (ObjcContainerConvertor::Task){ object, JSObjectMakeArray(contextRef, 0, NULL, 0), ContainerArray };

        if ([object isKindOfClass:[NSDictionary class]])
            return (ObjcContainerConvertor::Task){ object, JSObjectMake(contextRef, 0, 0), ContainerDictionary };

        if ([object isKindOfClass:[NSNull class]])
            return (ObjcContainerConvertor::Task){ object, JSValueMakeNull(contextRef), ContainerNone };

        if ([object isKindOfClass:[JSValue class]])
            return (ObjcContainerConvertor::Task){ object, ((JSValue *)object)->m_value, ContainerNone };

        if ([object isKindOfClass:[NSString class]]) {
            JSStringRef string = JSStringCreateWithCFString((CFStringRef)object);
            JSValueRef js = JSValueMakeString(contextRef, string);
            JSStringRelease(string);
            return (ObjcContainerConvertor::Task){ object, js, ContainerNone };
        }

        if ([object isKindOfClass:[NSNumber class]]) {
            if (isNSBoolean(object))
                return (ObjcContainerConvertor::Task){ object, JSValueMakeBoolean(contextRef, [object boolValue]), ContainerNone };
            return (ObjcContainerConvertor::Task){ object, JSValueMakeNumber(contextRef, [object doubleValue]), ContainerNone };
        }

        if ([object isKindOfClass:[NSDate class]]) {
            JSValueRef argument = JSValueMakeNumber(contextRef, [object timeIntervalSince1970]);
            JSObjectRef result = JSObjectMakeDate(contextRef, 1, &argument, 0);
            return (ObjcContainerConvertor::Task){ object, result, ContainerNone };
        }

        if ([object isKindOfClass:[JSManagedValue class]]) {
            JSValue *value = [static_cast<JSManagedValue *>(object) value];
            if (!value)
                return (ObjcContainerConvertor::Task) { object, JSValueMakeUndefined(contextRef), ContainerNone };
            return (ObjcContainerConvertor::Task){ object, value->m_value, ContainerNone };
        }
    }

    return (ObjcContainerConvertor::Task){ object, valueInternalValue([context wrapperForObjCObject:object]), ContainerNone };
}

JSValueRef objectToValue(JSContext *context, id object)
{
    JSGlobalContextRef contextRef = [context JSGlobalContextRef];

    ObjcContainerConvertor::Task task = objectToValueWithoutCopy(context, object);
    if (task.type == ContainerNone)
        return task.js;

    ObjcContainerConvertor convertor(context);
    convertor.add(task);
    ASSERT(!convertor.isWorkListEmpty());

    do {
        ObjcContainerConvertor::Task current = convertor.take();
        ASSERT(JSValueIsObject(contextRef, current.js));
        JSObjectRef js = JSValueToObject(contextRef, current.js, 0);

        if (current.type == ContainerArray) {
            ASSERT([current.objc isKindOfClass:[NSArray class]]);
            NSArray *array = (NSArray *)current.objc;
            NSUInteger count = [array count];
            for (NSUInteger index = 0; index < count; ++index)
                JSObjectSetPropertyAtIndex(contextRef, js, index, convertor.convert([array objectAtIndex:index]), 0);
        } else {
            ASSERT(current.type == ContainerDictionary);
            ASSERT([current.objc isKindOfClass:[NSDictionary class]]);
            NSDictionary *dictionary = (NSDictionary *)current.objc;
            for (id key in [dictionary keyEnumerator]) {
                if ([key isKindOfClass:[NSString class]]) {
                    JSStringRef propertyName = JSStringCreateWithCFString((CFStringRef)key);
                    JSObjectSetProperty(contextRef, js, propertyName, convertor.convert([dictionary objectForKey:key]), 0, 0);
                    JSStringRelease(propertyName);
                }
            }
        }
        
    } while (!convertor.isWorkListEmpty());

    return task.js;
}

JSValueRef valueInternalValue(JSValue * value)
{
    return value->m_value;
}

+ (JSValue *)valueWithJSValueRef:(JSValueRef)value inContext:(JSContext *)context
{
    return [context wrapperForJSObject:value];
}

- (JSValue *)init
{
    return nil;
}

- (JSValue *)initWithValue:(JSValueRef)value inContext:(JSContext *)context
{
    if (!value || !context)
        return nil;

    self = [super init];
    if (!self)
        return nil;

    _context = [context retain];
    m_value = value;
    JSValueProtect([_context JSGlobalContextRef], m_value);
    return self;
}

struct StructTagHandler {
    SEL typeToValueSEL;
    SEL valueToTypeSEL;
};
typedef HashMap<String, StructTagHandler> StructHandlers;

static StructHandlers* createStructHandlerMap()
{
    StructHandlers* structHandlers = new StructHandlers();

    size_t valueWithXinContextLength = strlen("valueWithX:inContext:");
    size_t toXLength = strlen("toX");

    // Step 1: find all valueWith<Foo>:inContext: class methods in JSValue.
    forEachMethodInClass(object_getClass([JSValue class]), ^(Method method){
        SEL selector = method_getName(method);
        const char* name = sel_getName(selector);
        size_t nameLength = strlen(name);
        // Check for valueWith<Foo>:context:
        if (nameLength < valueWithXinContextLength || memcmp(name, "valueWith", 9) || memcmp(name + nameLength - 11, ":inContext:", 11))
            return;
        // Check for [ id, SEL, <type>, <contextType> ]
        if (method_getNumberOfArguments(method) != 4)
            return;
        char idType[3];
        // Check 2nd argument type is "@"
        char* secondType = method_copyArgumentType(method, 3);
        if (strcmp(secondType, "@") != 0) {
            free(secondType);
            return;
        }
        free(secondType);
        // Check result type is also "@"
        method_getReturnType(method, idType, 3);
        if (strcmp(idType, "@") != 0)
            return;
        char* type = method_copyArgumentType(method, 2);
        structHandlers->add(StringImpl::create(type), (StructTagHandler){ selector, 0 });
        free(type);
    });

    // Step 2: find all to<Foo> instance methods in JSValue.
    forEachMethodInClass([JSValue class], ^(Method method){
        SEL selector = method_getName(method);
        const char* name = sel_getName(selector);
        size_t nameLength = strlen(name);
        // Check for to<Foo>
        if (nameLength < toXLength || memcmp(name, "to", 2))
            return;
        // Check for [ id, SEL ]
        if (method_getNumberOfArguments(method) != 2)
            return;
        // Try to find a matching valueWith<Foo>:context: method.
        char* type = method_copyReturnType(method);

        StructHandlers::iterator iter = structHandlers->find(type);
        free(type);
        if (iter == structHandlers->end())
            return;
        StructTagHandler& handler = iter->value;

        // check that strlen(<foo>) == strlen(<Foo>)
        const char* valueWithName = sel_getName(handler.typeToValueSEL);
        size_t valueWithLength = strlen(valueWithName);
        if (valueWithLength - valueWithXinContextLength != nameLength - toXLength)
            return;
        // Check that <Foo> == <Foo>
        if (memcmp(valueWithName + 9, name + 2, nameLength - toXLength - 1))
            return;
        handler.valueToTypeSEL = selector;
    });

    // Step 3: clean up - remove entries where we found prospective valueWith<Foo>:inContext: conversions, but no matching to<Foo> methods.
    typedef HashSet<String> RemoveSet;
    RemoveSet removeSet;
    for (StructHandlers::iterator iter = structHandlers->begin(); iter != structHandlers->end(); ++iter) {
        StructTagHandler& handler = iter->value;
        if (!handler.valueToTypeSEL)
            removeSet.add(iter->key);
    }

    for (RemoveSet::iterator iter = removeSet.begin(); iter != removeSet.end(); ++iter)
        structHandlers->remove(*iter);

    return structHandlers;
}

static StructTagHandler* handerForStructTag(const char* encodedType)
{
    static SpinLock handerForStructTagLock = SPINLOCK_INITIALIZER;
    SpinLockHolder lockHolder(&handerForStructTagLock);

    static StructHandlers* structHandlers = createStructHandlerMap();

    StructHandlers::iterator iter = structHandlers->find(encodedType);
    if (iter == structHandlers->end())
        return 0;
    return &iter->value;
}

+ (SEL)selectorForStructToValue:(const char *)structTag
{
    StructTagHandler* handler = handerForStructTag(structTag);
    return handler ? handler->typeToValueSEL : nil;
}

+ (SEL)selectorForValueToStruct:(const char *)structTag
{
    StructTagHandler* handler = handerForStructTag(structTag);
    return handler ? handler->valueToTypeSEL : nil;
}

- (void)dealloc
{
    JSValueUnprotect([_context JSGlobalContextRef], m_value);
    [_context release];
    _context = nil;
    [super dealloc];
}

- (NSString *)description
{
    if (id wrapped = tryUnwrapObjcObject([_context JSGlobalContextRef], m_value))
        return [wrapped description];
    return [self toString];
}

NSInvocation *typeToValueInvocationFor(const char* encodedType)
{
    SEL selector = [JSValue selectorForStructToValue:encodedType];
    if (!selector)
        return 0;

    const char* methodTypes = method_getTypeEncoding(class_getClassMethod([JSValue class], selector));
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[NSMethodSignature signatureWithObjCTypes:methodTypes]];
    [invocation setSelector:selector];
    return invocation;
}

NSInvocation *valueToTypeInvocationFor(const char* encodedType)
{
    SEL selector = [JSValue selectorForValueToStruct:encodedType];
    if (!selector)
        return 0;

    const char* methodTypes = method_getTypeEncoding(class_getInstanceMethod([JSValue class], selector));
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[NSMethodSignature signatureWithObjCTypes:methodTypes]];
    [invocation setSelector:selector];
    return invocation;
}

@end

#endif
