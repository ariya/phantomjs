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
#import "JavaScriptCore.h"

#if JSC_OBJC_API_ENABLED

#import "APICast.h"
#import "APIShims.h"
#import "JSAPIWrapperObject.h"
#import "JSCallbackObject.h"
#import "JSContextInternal.h"
#import "JSWrapperMap.h"
#import "ObjCCallbackFunction.h"
#import "ObjcRuntimeExtras.h"
#import "Operations.h"
#import "WeakGCMap.h"
#import <wtf/TCSpinLock.h>
#import <wtf/Vector.h>

@class JSObjCClassInfo;

@interface JSWrapperMap () 

- (JSObjCClassInfo*)classInfoForClass:(Class)cls;

@end

// Default conversion of selectors to property names.
// All semicolons are removed, lowercase letters following a semicolon are capitalized.
static NSString *selectorToPropertyName(const char* start)
{
    // Use 'index' to check for colons, if there are none, this is easy!
    const char* firstColon = index(start, ':');
    if (!firstColon)
        return [NSString stringWithUTF8String:start];

    // 'header' is the length of string up to the first colon.
    size_t header = firstColon - start;
    // The new string needs to be long enough to hold 'header', plus the remainder of the string, excluding
    // at least one ':', but including a '\0'. (This is conservative if there are more than one ':').
    char* buffer = static_cast<char*>(malloc(header + strlen(firstColon + 1) + 1));
    // Copy 'header' characters, set output to point to the end of this & input to point past the first ':'.
    memcpy(buffer, start, header);
    char* output = buffer + header;
    const char* input = start + header + 1;

    // On entry to the loop, we have already skipped over a ':' from the input.
    while (true) {
        char c;
        // Skip over any additional ':'s. We'll leave c holding the next character after the
        // last ':', and input pointing past c.
        while ((c = *(input++)) == ':');
        // Copy the character, converting to upper case if necessary.
        // If the character we copy is '\0', then we're done!
        if (!(*(output++) = toupper(c)))
            goto done;
        // Loop over characters other than ':'.
        while ((c = *(input++)) != ':') {
            // Copy the character.
            // If the character we copy is '\0', then we're done!
            if (!(*(output++) = c))
                goto done;
        }
        // If we get here, we've consumed a ':' - wash, rinse, repeat.
    }
done:
    NSString *result = [NSString stringWithUTF8String:buffer];
    free(buffer);
    return result;
}

static JSObjectRef makeWrapper(JSContextRef ctx, JSClassRef jsClass, id wrappedObject)
{
    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    ASSERT(jsClass);
    JSC::JSCallbackObject<JSC::JSAPIWrapperObject>* object = JSC::JSCallbackObject<JSC::JSAPIWrapperObject>::create(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->objcWrapperObjectStructure(), jsClass, 0);
    object->setWrappedObject(wrappedObject);
    if (JSC::JSObject* prototype = jsClass->prototype(exec))
        object->setPrototype(exec->vm(), prototype);

    return toRef(object);
}

// Make an object that is in all ways a completely vanilla JavaScript object,
// other than that it has a native brand set that will be displayed by the default
// Object.prototype.toString conversion.
static JSValue *objectWithCustomBrand(JSContext *context, NSString *brand, Class cls = 0)
{
    JSClassDefinition definition;
    definition = kJSClassDefinitionEmpty;
    definition.className = [brand UTF8String];
    JSClassRef classRef = JSClassCreate(&definition);
    JSObjectRef result = makeWrapper([context JSGlobalContextRef], classRef, cls);
    JSClassRelease(classRef);
    return [JSValue valueWithJSValueRef:result inContext:context];
}

// Look for @optional properties in the prototype containing a selector to property
// name mapping, separated by a __JS_EXPORT_AS__ delimiter.
static NSMutableDictionary *createRenameMap(Protocol *protocol, BOOL isInstanceMethod)
{
    NSMutableDictionary *renameMap = [[NSMutableDictionary alloc] init];

    forEachMethodInProtocol(protocol, NO, isInstanceMethod, ^(SEL sel, const char*){
        NSString *rename = @(sel_getName(sel));
        NSRange range = [rename rangeOfString:@"__JS_EXPORT_AS__"];
        if (range.location == NSNotFound)
            return;
        NSString *selector = [rename substringToIndex:range.location];
        NSUInteger begin = range.location + range.length;
        NSUInteger length = [rename length] - begin - 1;
        NSString *name = [rename substringWithRange:(NSRange){ begin, length }];
        renameMap[selector] = name;
    });

    return renameMap;
}

inline void putNonEnumerable(JSValue *base, NSString *propertyName, JSValue *value)
{
    [base defineProperty:propertyName descriptor:@{
        JSPropertyDescriptorValueKey: value,
        JSPropertyDescriptorWritableKey: @YES,
        JSPropertyDescriptorEnumerableKey: @NO,
        JSPropertyDescriptorConfigurableKey: @YES
    }];
}

// This method will iterate over the set of required methods in the protocol, and:
//  * Determine a property name (either via a renameMap or default conversion).
//  * If an accessorMap is provided, and contains this name, store the method in the map.
//  * Otherwise, if the object doesn't already contain a property with name, create it.
static void copyMethodsToObject(JSContext *context, Class objcClass, Protocol *protocol, BOOL isInstanceMethod, JSValue *object, NSMutableDictionary *accessorMethods = nil)
{
    NSMutableDictionary *renameMap = createRenameMap(protocol, isInstanceMethod);

    forEachMethodInProtocol(protocol, YES, isInstanceMethod, ^(SEL sel, const char* types){
        const char* nameCStr = sel_getName(sel);
        NSString *name = @(nameCStr);
        if (accessorMethods && accessorMethods[name]) {
            JSObjectRef method = objCCallbackFunctionForMethod(context, objcClass, protocol, isInstanceMethod, sel, types);
            if (!method)
                return;
            accessorMethods[name] = [JSValue valueWithJSValueRef:method inContext:context];
        } else {
            name = renameMap[name];
            if (!name)
                name = selectorToPropertyName(nameCStr);
            if ([object hasProperty:name])
                return;
            JSObjectRef method = objCCallbackFunctionForMethod(context, objcClass, protocol, isInstanceMethod, sel, types);
            if (method)
                putNonEnumerable(object, name, [JSValue valueWithJSValueRef:method inContext:context]);
        }
    });

    [renameMap release];
}

static bool parsePropertyAttributes(objc_property_t property, char*& getterName, char*& setterName)
{
    bool readonly = false;
    unsigned attributeCount;
    objc_property_attribute_t* attributes = property_copyAttributeList(property, &attributeCount);
    if (attributeCount) {
        for (unsigned i = 0; i < attributeCount; ++i) {
            switch (*(attributes[i].name)) {
            case 'G':
                getterName = strdup(attributes[i].value);
                break;
            case 'S':
                setterName = strdup(attributes[i].value);
                break;
            case 'R':
                readonly = true;
                break;
            default:
                break;
            }
        }
        free(attributes);
    }
    return readonly;
}

static char* makeSetterName(const char* name)
{
    size_t nameLength = strlen(name);
    char* setterName = (char*)malloc(nameLength + 5); // "set" Name ":\0"
    setterName[0] = 's';
    setterName[1] = 'e';
    setterName[2] = 't';
    setterName[3] = toupper(*name);
    memcpy(setterName + 4, name + 1, nameLength - 1);
    setterName[nameLength + 3] = ':';
    setterName[nameLength + 4] = '\0';
    return setterName;
}

static void copyPrototypeProperties(JSContext *context, Class objcClass, Protocol *protocol, JSValue *prototypeValue)
{
    // First gather propreties into this list, then handle the methods (capturing the accessor methods).
    struct Property {
        const char* name;
        char* getterName;
        char* setterName;
    };
    __block Vector<Property> propertyList;

    // Map recording the methods used as getters/setters.
    NSMutableDictionary *accessorMethods = [NSMutableDictionary dictionary];

    // Useful value.
    JSValue *undefined = [JSValue valueWithUndefinedInContext:context];

    forEachPropertyInProtocol(protocol, ^(objc_property_t property){
        char* getterName = 0;
        char* setterName = 0;
        bool readonly = parsePropertyAttributes(property, getterName, setterName);
        const char* name = property_getName(property);

        // Add the names of the getter & setter methods to 
        if (!getterName)
            getterName = strdup(name);
        accessorMethods[@(getterName)] = undefined;
        if (!readonly) {
            if (!setterName)
                setterName = makeSetterName(name);
            accessorMethods[@(setterName)] = undefined;
        }

        // Add the properties to a list.
        propertyList.append((Property){ name, getterName, setterName });
    });

    // Copy methods to the prototype, capturing accessors in the accessorMethods map.
    copyMethodsToObject(context, objcClass, protocol, YES, prototypeValue, accessorMethods);

    // Iterate the propertyList & generate accessor properties.
    for (size_t i = 0; i < propertyList.size(); ++i) {
        Property& property = propertyList[i];

        JSValue *getter = accessorMethods[@(property.getterName)];
        free(property.getterName);
        ASSERT(![getter isUndefined]);

        JSValue *setter = undefined;
        if (property.setterName) {
            setter = accessorMethods[@(property.setterName)];
            free(property.setterName);
            ASSERT(![setter isUndefined]);
        }
        
        [prototypeValue defineProperty:@(property.name) descriptor:@{
            JSPropertyDescriptorGetKey: getter,
            JSPropertyDescriptorSetKey: setter,
            JSPropertyDescriptorEnumerableKey: @NO,
            JSPropertyDescriptorConfigurableKey: @YES
        }];
    }
}

@interface JSObjCClassInfo : NSObject {
    JSContext *m_context;
    Class m_class;
    bool m_block;
    JSClassRef m_classRef;
    JSC::Weak<JSC::JSObject> m_prototype;
    JSC::Weak<JSC::JSObject> m_constructor;
}

- (id)initWithContext:(JSContext *)context forClass:(Class)cls superClassInfo:(JSObjCClassInfo*)superClassInfo;
- (JSValue *)wrapperForObject:(id)object;
- (JSValue *)constructor;

@end

@implementation JSObjCClassInfo

- (id)initWithContext:(JSContext *)context forClass:(Class)cls superClassInfo:(JSObjCClassInfo*)superClassInfo
{
    self = [super init];
    if (!self)
        return nil;

    const char* className = class_getName(cls);
    m_context = context;
    m_class = cls;
    m_block = [cls isSubclassOfClass:getNSBlockClass()];
    JSClassDefinition definition;
    definition = kJSClassDefinitionEmpty;
    definition.className = className;
    m_classRef = JSClassCreate(&definition);

    [self allocateConstructorAndPrototypeWithSuperClassInfo:superClassInfo];

    return self;
}

- (void)dealloc
{
    JSClassRelease(m_classRef);
    [super dealloc];
}

- (void)allocateConstructorAndPrototypeWithSuperClassInfo:(JSObjCClassInfo*)superClassInfo
{
    ASSERT(!m_constructor || !m_prototype);
    ASSERT((m_class == [NSObject class]) == !superClassInfo);
    if (!superClassInfo) {
        JSContextRef cContext = [m_context JSGlobalContextRef];
        JSValue *constructor = m_context[@"Object"];
        if (!m_constructor)
            m_constructor = toJS(JSValueToObject(cContext, valueInternalValue(constructor), 0));

        if (!m_prototype) {
            JSValue *prototype = constructor[@"prototype"];
            m_prototype = toJS(JSValueToObject(cContext, valueInternalValue(prototype), 0));
        }
    } else {
        const char* className = class_getName(m_class);

        // Create or grab the prototype/constructor pair.
        JSValue *prototype;
        JSValue *constructor;
        if (m_prototype)
            prototype = [JSValue valueWithJSValueRef:toRef(m_prototype.get()) inContext:m_context];
        else
            prototype = objectWithCustomBrand(m_context, [NSString stringWithFormat:@"%sPrototype", className]);

        if (m_constructor)
            constructor = [JSValue valueWithJSValueRef:toRef(m_constructor.get()) inContext:m_context];
        else
            constructor = objectWithCustomBrand(m_context, [NSString stringWithFormat:@"%sConstructor", className], m_class);

        JSContextRef cContext = [m_context JSGlobalContextRef];
        m_prototype = toJS(JSValueToObject(cContext, valueInternalValue(prototype), 0));
        m_constructor = toJS(JSValueToObject(cContext, valueInternalValue(constructor), 0));

        putNonEnumerable(prototype, @"constructor", constructor);
        putNonEnumerable(constructor, @"prototype", prototype);

        Protocol *exportProtocol = getJSExportProtocol();
        forEachProtocolImplementingProtocol(m_class, exportProtocol, ^(Protocol *protocol){
            copyPrototypeProperties(m_context, m_class, protocol, prototype);
            copyMethodsToObject(m_context, m_class, protocol, NO, constructor);
        });

        // Set [Prototype].
        JSObjectSetPrototype([m_context JSGlobalContextRef], toRef(m_prototype.get()), toRef(superClassInfo->m_prototype.get()));
    }
}

- (void)reallocateConstructorAndOrPrototype
{
    [self allocateConstructorAndPrototypeWithSuperClassInfo:[m_context.wrapperMap classInfoForClass:class_getSuperclass(m_class)]];
}

- (JSValue *)wrapperForObject:(id)object
{
    ASSERT([object isKindOfClass:m_class]);
    ASSERT(m_block == [object isKindOfClass:getNSBlockClass()]);
    if (m_block) {
        if (JSObjectRef method = objCCallbackFunctionForBlock(m_context, object))
            return [JSValue valueWithJSValueRef:method inContext:m_context];
    }

    if (!m_prototype)
        [self reallocateConstructorAndOrPrototype];
    ASSERT(!!m_prototype);

    JSObjectRef wrapper = makeWrapper([m_context JSGlobalContextRef], m_classRef, object);
    JSObjectSetPrototype([m_context JSGlobalContextRef], wrapper, toRef(m_prototype.get()));
    return [JSValue valueWithJSValueRef:wrapper inContext:m_context];
}

- (JSValue *)constructor
{
    if (!m_constructor)
        [self reallocateConstructorAndOrPrototype];
    ASSERT(!!m_constructor);
    return [JSValue valueWithJSValueRef:toRef(m_constructor.get()) inContext:m_context];
}

@end

@implementation JSWrapperMap {
    JSContext *m_context;
    NSMutableDictionary *m_classMap;
    JSC::WeakGCMap<id, JSC::JSObject> m_cachedJSWrappers;
    NSMapTable *m_cachedObjCWrappers;
}

- (id)initWithContext:(JSContext *)context
{
    self = [super init];
    if (!self)
        return nil;

    NSPointerFunctionsOptions keyOptions = NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality;
    NSPointerFunctionsOptions valueOptions = NSPointerFunctionsWeakMemory | NSPointerFunctionsObjectPersonality;
    m_cachedObjCWrappers = [[NSMapTable alloc] initWithKeyOptions:keyOptions valueOptions:valueOptions capacity:0];
    
    m_context = context;
    m_classMap = [[NSMutableDictionary alloc] init];
    return self;
}

- (void)dealloc
{
    [m_cachedObjCWrappers release];
    [m_classMap release];
    [super dealloc];
}

- (JSObjCClassInfo*)classInfoForClass:(Class)cls
{
    if (!cls)
        return nil;

    // Check if we've already created a JSObjCClassInfo for this Class.
    if (JSObjCClassInfo* classInfo = (JSObjCClassInfo*)m_classMap[cls])
        return classInfo;

    // Skip internal classes beginning with '_' - just copy link to the parent class's info.
    if ('_' == *class_getName(cls))
        return m_classMap[cls] = [self classInfoForClass:class_getSuperclass(cls)];

    return m_classMap[cls] = [[[JSObjCClassInfo alloc] initWithContext:m_context forClass:cls superClassInfo:[self classInfoForClass:class_getSuperclass(cls)]] autorelease];
}

- (JSValue *)jsWrapperForObject:(id)object
{
    JSC::JSObject* jsWrapper = m_cachedJSWrappers.get(object);
    if (jsWrapper)
        return [JSValue valueWithJSValueRef:toRef(jsWrapper) inContext:m_context];

    JSValue *wrapper;
    if (class_isMetaClass(object_getClass(object)))
        wrapper = [[self classInfoForClass:(Class)object] constructor];
    else {
        JSObjCClassInfo* classInfo = [self classInfoForClass:[object class]];
        wrapper = [classInfo wrapperForObject:object];
    }

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=105891
    // This general approach to wrapper caching is pretty effective, but there are a couple of problems:
    // (1) For immortal objects JSValues will effectively leak and this results in error output being logged - we should avoid adding associated objects to immortal objects.
    // (2) A long lived object may rack up many JSValues. When the contexts are released these will unprotect the associated JavaScript objects,
    //     but still, would probably nicer if we made it so that only one associated object was required, broadcasting object dealloc.
    JSC::ExecState* exec = toJS([m_context JSGlobalContextRef]);
    jsWrapper = toJS(exec, valueInternalValue(wrapper)).toObject(exec);
    m_cachedJSWrappers.set(object, jsWrapper);
    return wrapper;
}

- (JSValue *)objcWrapperForJSValueRef:(JSValueRef)value
{
    JSValue *wrapper = static_cast<JSValue *>(NSMapGet(m_cachedObjCWrappers, value));
    if (!wrapper) {
        wrapper = [[[JSValue alloc] initWithValue:value inContext:m_context] autorelease];
        NSMapInsert(m_cachedObjCWrappers, value, wrapper);
    }
    return wrapper;
}

@end

id tryUnwrapObjcObject(JSGlobalContextRef context, JSValueRef value)
{
    if (!JSValueIsObject(context, value))
        return nil;
    JSValueRef exception = 0;
    JSObjectRef object = JSValueToObject(context, value, &exception);
    ASSERT(!exception);
    if (toJS(object)->inherits(&JSC::JSCallbackObject<JSC::JSAPIWrapperObject>::s_info))
        return (id)JSC::jsCast<JSC::JSAPIWrapperObject*>(toJS(object))->wrappedObject();
    if (id target = tryUnwrapBlock(object))
        return target;
    return nil;
}

Protocol *getJSExportProtocol()
{
    static Protocol *protocol = objc_getProtocol("JSExport");
    return protocol;
}

Class getNSBlockClass()
{
    static Class cls = objc_getClass("NSBlock");
    return cls;
}

#endif
