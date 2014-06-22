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

#ifndef JSValue_h
#define JSValue_h

#if JSC_OBJC_API_ENABLED

@class JSContext;

// A JSValue is a reference to a value within the JavaScript object space of a
// JSVirtualMachine. All instances of JSValue originate from a JSContext and
// hold a strong reference to this JSContext. As long as any value associated with 
// a particular JSContext is retained, that JSContext will remain alive. 
// Where an instance method is invoked upon a JSValue, and this returns another 
// JSValue, the returned JSValue will originate from the same JSContext as the 
// JSValue on which the method was invoked.
//
// For all methods taking arguments of type id, arguments will be converted
// into a JavaScript value according to the conversion specified below.
// All JavaScript values are associated with a particular JSVirtualMachine
// (the associated JSVirtualMachine is available indirectly via the context
// property). An instance of JSValue may only be passed as an argument to
// methods on instances of JSValue and JSContext that belong to the same
// JSVirtualMachine - passing a JSValue to a method on an object originating
// from a different JSVirtualMachine will result in an Objective-C exception
// being raised.
//
// Conversion between Objective-C and JavaScript types.
//
// When converting between JavaScript values and Objective-C objects a copy is
// performed. Values of types listed below are copied to the corresponding
// types on conversion in each direction. For NSDictionaries, entries in the
// dictionary that are keyed by strings are copied onto a JavaScript object.
// For dictionaries and arrays, conversion is recursive, with the same object
// conversion being applied to all entries in the collection.
//
//   Objective-C type  |   JavaScript type
// --------------------+---------------------
//         nil         |     undefined
//        NSNull       |        null
//       NSString      |       string
//       NSNumber      |   number, boolean
//     NSDictionary    |   Object object
//       NSArray       |    Array object
//        NSDate       |     Date object
//       NSBlock *     |   Function object *
//          id **      |   Wrapper object **
//        Class ***    | Constructor object ***
//
// * Instances of NSBlock with supported arguments types will be presented to
// JavaScript as a callable Function object. For more information on supported
// argument types see JSExport.h. If a JavaScript Function originating from an
// Objective-C block is converted back to an Objective-C object the block will
// be returned. All other JavaScript functions will be converted in the same
// manner as a JavaScript object of type Object.
//
// ** For Objective-C instances that do not derive from the set of types listed
// above, a wrapper object to provide a retaining handle to the Objective-C
// instance from JavaScript. For more information on these wrapper objects, see
// JSExport.h. When a JavaScript wrapper object is converted back to Objective-C
// the Objective-C instance being retained by the wrapper is returned.
//
// *** For Objective-C Class objects a constructor object containing exported
// class methods will be returned. See JSExport.h for more information on
// constructor objects.

NS_CLASS_AVAILABLE(10_9, NA)
@interface JSValue : NSObject

// Create a JSValue by converting an Objective-C object.
+ (JSValue *)valueWithObject:(id)value inContext:(JSContext *)context;
// Create a JavaScript value from an Objective-C primitive type.
+ (JSValue *)valueWithBool:(BOOL)value inContext:(JSContext *)context;
+ (JSValue *)valueWithDouble:(double)value inContext:(JSContext *)context;
+ (JSValue *)valueWithInt32:(int32_t)value inContext:(JSContext *)context;
+ (JSValue *)valueWithUInt32:(uint32_t)value inContext:(JSContext *)context;
// Create a JavaScript value in this context.
+ (JSValue *)valueWithNewObjectInContext:(JSContext *)context;
+ (JSValue *)valueWithNewArrayInContext:(JSContext *)context;
+ (JSValue *)valueWithNewRegularExpressionFromPattern:(NSString *)pattern flags:(NSString *)flags inContext:(JSContext *)context;
+ (JSValue *)valueWithNewErrorFromMessage:(NSString *)message inContext:(JSContext *)context;
+ (JSValue *)valueWithNullInContext:(JSContext *)context;
+ (JSValue *)valueWithUndefinedInContext:(JSContext *)context;

// Convert this value to a corresponding Objective-C object, according to the
// conversion specified above.
- (id)toObject;
// Convert this value to a corresponding Objective-C object, if the result is
// not of the specified class then nil will be returned.
- (id)toObjectOfClass:(Class)expectedClass;
// The value is copied to a boolean according to the conversion specified by the
// JavaScript language.
- (BOOL)toBool;
// The value is copied to a number according to the conversion specified by the
// JavaScript language.
- (double)toDouble;
// The value is copied to an integer according to the conversion specified by
// the JavaScript language.
- (int32_t)toInt32;
// The value is copied to an integer according to the conversion specified by
// the JavaScript language.
- (uint32_t)toUInt32;
// If the value is a boolean, a NSNumber value of @YES or @NO will be returned.
// For all other types the value will be copied to a number according to the
// conversion specified by the JavaScript language.
- (NSNumber *)toNumber;
// The value is copied to a string according to the conversion specified by the
// JavaScript language.
- (NSString *)toString;
// The value is converted to a number representing a time interval since 1970,
// and a new NSDate instance is returned.
- (NSDate *)toDate;
// If the value is null or undefined then nil is returned.
// If the value is not an object then a JavaScript TypeError will be thrown.
// The property "length" is read from the object, converted to an unsigned
// integer, and an NSArray of this size is allocated. Properties corresponding
// to indicies within the array bounds will be copied to the array, with
// Objective-C objects converted to equivalent JSValues as specified.
- (NSArray *)toArray;
// If the value is null or undefined then nil is returned.
// If the value is not an object then a JavaScript TypeError will be thrown.
// All enumerable properties of the object are copied to the dictionary, with
// Objective-C objects converted to equivalent JSValues as specified.
- (NSDictionary *)toDictionary;

// Access a property from the value. This method will return the JavaScript value
// 'undefined' if the property does not exist.
- (JSValue *)valueForProperty:(NSString *)property;
// Set a property on the value.
- (void)setValue:(id)value forProperty:(NSString *)property;
// Delete a property from the value, returns YES if deletion is successful.
- (BOOL)deleteProperty:(NSString *)property;
// Returns YES if property is present on the value.
// This method has the same function as the JavaScript operator "in".
- (BOOL)hasProperty:(NSString *)property;
// This method may be used to create a data or accessor property on an object;
// this method operates in accordance with the Object.defineProperty method in
// the JavaScript language.
- (void)defineProperty:(NSString *)property descriptor:(id)descriptor;

// Access an indexed property from the value. This method will return the
// JavaScript value 'undefined' if no property exists at that index. 
- (JSValue *)valueAtIndex:(NSUInteger)index;
// Set an indexed property on the value. For JSValues that are JavaScript arrays, 
// indices greater than UINT_MAX - 1 will not affect the length of the array.
- (void)setValue:(id)value atIndex:(NSUInteger)index;

// All JavaScript values are precisely one of these types.
- (BOOL)isUndefined;
- (BOOL)isNull;
- (BOOL)isBoolean;
- (BOOL)isNumber;
- (BOOL)isString;
- (BOOL)isObject;

// This method has the same function as the JavaScript operator "===".
- (BOOL)isEqualToObject:(id)value;
// This method has the same function as the JavaScript operator "==".
- (BOOL)isEqualWithTypeCoercionToObject:(id)value;
// This method has the same function as the JavaScript operator "instanceof".
- (BOOL)isInstanceOf:(id)value;

// Call this value as a function passing the specified arguments.
- (JSValue *)callWithArguments:(NSArray *)arguments;
// Call this value as a constructor passing the specified arguments.
- (JSValue *)constructWithArguments:(NSArray *)arguments;
// Access the property named "method" from this value; call the value resulting
// from the property access as a function, passing this value as the "this"
// value, and the specified arguments.
- (JSValue *)invokeMethod:(NSString *)method withArguments:(NSArray *)arguments;

// The JSContext that this value originates from.
@property(readonly, retain) JSContext *context;

@end

// Objective-C methods exported to JavaScript may have argument and/or return
// values of struct types, provided that conversion to and from the struct is
// supported by JSValue. Support is provided for any types where JSValue
// contains both a class method "valueWith<Type>:inContext:", and and instance
// method "to<Type>" - where the string "<Type>" in these selector names match,
// with the first argument to the former being of the same struct type as the
// return type of the latter.
// Support is provided for structs of type CGPoint, NSRange, CGRect and CGSize.
@interface JSValue(StructSupport)

// This method returns a newly allocated JavaScript object containing properties
// named "x" and "y", with values from the CGPoint.
+ (JSValue *)valueWithPoint:(CGPoint)point inContext:(JSContext *)context;
// This method returns a newly allocated JavaScript object containing properties
// named "location" and "length", with values from the NSRange.
+ (JSValue *)valueWithRange:(NSRange)range inContext:(JSContext *)context;
// This method returns a newly allocated JavaScript object containing properties
// named "x", "y", "width", and "height", with values from the CGRect.
+ (JSValue *)valueWithRect:(CGRect)rect inContext:(JSContext *)context;
// This method returns a newly allocated JavaScript object containing properties
// named "width" and "height", with values from the CGSize.
+ (JSValue *)valueWithSize:(CGSize)size inContext:(JSContext *)context;

// Convert a value to type CGPoint by reading properties named "x" and "y" from
// this value, and converting the results to double.
- (CGPoint)toPoint;
// Convert a value to type NSRange by accessing properties named "location" and
// "length" from this value converting the results to double.
- (NSRange)toRange;
// Convert a value to type CGRect by reading properties named "x", "y", "width",
// and "height" from this value, and converting the results to double.
- (CGRect)toRect;
// Convert a value to type CGSize by accessing properties named "width" and
// "height" from this value converting the results to double.
- (CGSize)toSize;

@end

// Instances of JSValue implement the following methods in order to enable
// support for subscript access by key and index, for example:
//
//    JSValue *objectA, *objectB;
//    JSValue *v1 = object[@"X"]; // Get value for property "X" from 'object'.
//    JSValue *v2 = object[42];   // Get value for index 42 from 'object'.
//    object[@"Y"] = v1;          // Assign 'v1' to property "Y" of 'object'.
//    object[101] = v2;           // Assign 'v2' to index 101 of 'object'.
//
// An object key passed as a subscript will be converted to a JavaScript value,
// and then the value converted to a string used as a property name.
@interface JSValue(SubscriptSupport)

- (JSValue *)objectForKeyedSubscript:(id)key;
- (JSValue *)objectAtIndexedSubscript:(NSUInteger)index;
- (void)setObject:(id)object forKeyedSubscript:(NSObject <NSCopying> *)key;
- (void)setObject:(id)object atIndexedSubscript:(NSUInteger)index;

@end

// These functions are for bridging between the C API and the Objective-C API.
@interface JSValue(JSValueRefSupport)
// Creates a JSValue, wrapping its C API counterpart.
+ (JSValue *)valueWithJSValueRef:(JSValueRef)value inContext:(JSContext *)context;
// Returns the C API counterpart wrapped by a JSContext.
- (JSValueRef)JSValueRef;
@end

#ifdef __cplusplus
extern "C" {
#endif

// These keys may assist in creating a property descriptor for use with the
// defineProperty method on JSValue.
// Property descriptors must fit one of three descriptions:
// Data Descriptor:
//  - A descriptor containing one or both of the keys "value" and "writable",
//    and optionally containing one or both of the keys "enumerable" and
//    "configurable". A data descriptor may not contain either the "get" or
//    "set" key.
//    A data descriptor may be used to create or modify the attributes of a
//    data property on an object (replacing any existing accessor property).
// Accessor Descriptor:
//  - A descriptor containing one or both of the keys "get" and "set", and
//    optionally containing one or both of the keys "enumerable" and
//    "configurable". An accessor descriptor may not contain either the "value"
//    or "writable" key.
//    An accessor descriptor may be used to create or modify the attributes of
//    an accessor property on an object (replacing any existing data property).
// Generic Descriptor:
//  - A descriptor containing one or both of the keys "enumerable" and
//    "configurable". A generic descriptor may not contain any of the keys
//    "value", " writable", "get", or "set".
//    A generic descriptor may be used to modify the attributes of an existing
//    data or accessor property, or to create a new data property.
JS_EXPORT extern NSString * const JSPropertyDescriptorWritableKey;
JS_EXPORT extern NSString * const JSPropertyDescriptorEnumerableKey;
JS_EXPORT extern NSString * const JSPropertyDescriptorConfigurableKey;
JS_EXPORT extern NSString * const JSPropertyDescriptorValueKey;
JS_EXPORT extern NSString * const JSPropertyDescriptorGetKey;
JS_EXPORT extern NSString * const JSPropertyDescriptorSetKey;

#ifdef __cplusplus
} // extern "C"
#endif

#endif

#endif // JSValue_h
