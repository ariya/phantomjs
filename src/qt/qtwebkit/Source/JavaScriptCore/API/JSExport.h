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

#import <JavaScriptCore/JavaScriptCore.h>

#if JSC_OBJC_API_ENABLED

// When a JavaScript value is created from an instance of an Objective-C class
// for which no copying conversion is specified a JavaScript wrapper object will
// be created.
//
// In JavaScript inheritance is supported via a chain of prototype objects, and
// for each Objective-C class (and per JSContext) an object appropriate for use
// as a prototype will be provided. For the class NSObject the prototype object
// will be the JavaScript context's Object Prototype. For all other Objective-C
// classes a Prototype object will be created. The Prototype object for a given
// Objective-C class will have its internal [Prototype] property set to point to
// the Prototype object of the Objective-C class's superclass. As such the
// prototype chain for a JavaScript wrapper object will reflect the wrapped
// Objective-C type's inheritance hierarchy.
//
// In addition to the Prototype object a JavaScript Constructor object will also
// be produced for each Objective-C class. The Constructor object has a property
// named 'prototype' that references the Prototype object, and the Prototype
// object has a property named 'constructor' that references the Constructor.
// The Constructor object is not callable.
//
// By default no methods or properties of the Objective-C class will be exposed
// to JavaScript, however methods and properties may explicitly be exported.
// For each protocol that a class conforms to, if the protocol incorporates the
// protocol JSExport, then the protocol will be interpreted as a list of methods
// and properties to be exported to JavaScript.
//
// For each instance method being exported, a corresponding JavaScript function
// will be assigned as a property of the Prototype object, for each Objective-C
// property being exported a JavaScript accessor property will be created on the
// Prototype, and for each class method exported a JavaScript function will be
// created on the Constructor object. For example:
//
//    @protocol MyClassJavaScriptMethods <JSExport>
//    - (void)foo;
//    @end
//
//    @interface MyClass : NSObject <MyClassJavaScriptMethods>
//    - (void)foo;
//    - (void)bar;
//    @end
//
// Data properties that are created on the prototype or constructor objects have
// the attributes: writable:true, enumerable:false, configurable:true. Accessor
// properties have the attributes: enumerable:false and configurable:true.
//
// If an instance of MyClass is converted to a JavaScript value, the resulting
// wrapper object will (via its prototype) export the method "foo" to JavaScript,
// since the class conforms to the MyClassJavaScriptMethods protocol, and this
// protocol incorporates JSExport. "bar" will not be exported.
//
// Properties, arguments, and return values of the following types are
// supported:
//
// Primitive numbers: signed values of up to 32-bits are converted in a manner
//    consistent with valueWithInt32/toInt32, unsigned values of up to 32-bits
//    are converted in a manner consistent with valueWithUInt32/toUInt32, all
//    other numeric values are converted consistently with valueWithDouble/
//    toDouble.
// BOOL: values are converted consistently with valueWithBool/toBool.
// id: values are converted consistently with valueWithObject/toObject.
// <Objective-C Class>: - where the type is a pointer to a specified Objective-C
//    class, conversion is consistent with valueWithObjectOfClass/toObject.
// struct types: C struct types are supported, where JSValue provides support
//    for the given type. Support is built in for CGPoint, NSRange, CGRect, and
//    CGSize.
// block types: In addition to support provided by valueWithObject/toObject for
//    block types, if a JavaScript Function is passed as an argument, where the
//    type required is a block with a void return value (and where the block's
//    arguments are all of supported types), then a special adaptor block
//    will be created, allowing the JavaScript function to be used in the place
//    of a block.
//
// For any interface that conforms to JSExport the normal copying conversion for
// built in types will be inhibited - so, for example, if an instance that
// derives from NSString but conforms to JSExport is passed to valueWithObject:
// then a wrapper object for the Objective-C object will be returned rather than
// a JavaScript string primitive.
@protocol JSExport
@end

// When a selector that takes one or more arguments is converted to a JavaScript
// property name, by default a property name will be generated by performing the
// following conversion:
//  - All colons are removed from the selector
//  - Any lowercase letter that had followed a colon will be capitalized.
// Under the default conversion a selector "doFoo:withBar:" will be exported as
// "doFooWithBar". The default conversion may be overriden using the JSExportAs
// macro, for example to export a method "doFoo:withBar:" as "doFoo":
//
//    @protocol MyClassJavaScriptMethods <JSExport>
//    JSExportAs(doFoo,
//    - (void)doFoo:(id)foo withBar:(id)bar
//    );
//    @end
//
// Note that the JSExport macro may only be applied to a selector that takes one
// or more argument.
#define JSExportAs(PropertyName, Selector) \
    @optional Selector __JS_EXPORT_AS__##PropertyName:(id)argument; @required Selector

#endif
