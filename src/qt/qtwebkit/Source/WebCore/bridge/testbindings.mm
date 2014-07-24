/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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

#include "config.h"

#include "BridgeJSC.h"
#include <Foundation/Foundation.h>
#include "JSObject.h"
#include "JSCJSValue.h"
#import <WebKit/WebScriptObject.h>
#include "interpreter.h"
#include "runtime_object.h"
#include <stdio.h>
#include <string.h>
#include "types.h"


#define LOG(formatAndArgs...) { \
    fprintf (stderr, "%s:  ", __PRETTY_FUNCTION__); \
    fprintf(stderr, formatAndArgs); \
}

@interface MySecondInterface : NSObject
{
    double doubleValue;
}

- init;

@end

@implementation MySecondInterface

- init
{
    LOG ("\n");
    doubleValue = 666.666;
    return self;
}

@end

@interface MyFirstInterface : NSObject
{
    int myInt;
    MySecondInterface *mySecondInterface;
    id jsobject;
    NSString *string;
}

- (int)getInt;
- (void)setInt: (int)anInt;
- (MySecondInterface *)getMySecondInterface;
- (void)logMessage:(NSString *)message;
- (void)setJSObject:(id)jsobject;
@end

@implementation MyFirstInterface

+ (NSString *)webScriptNameForSelector:(SEL)aSelector
{
    if (aSelector == @selector(logMessage:))
        return @"logMessage";
    if (aSelector == @selector(logMessages:))
        return @"logMessages";
    if (aSelector == @selector(logMessage:prefix:))
        return @"logMessageWithPrefix";
    return nil;
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    return NO;
}

+ (BOOL)isKeyExcludedFromWebScript:(const char *)name
{
    return NO;
}

/*
- (id)invokeUndefinedMethodFromWebScript:(NSString *)name withArguments:(NSArray *)args
{
    NSLog (@"Call to undefined method %@", name);
    NSLog (@"%d args\n", [args count]);
    int i;
    for (i = 0; i < [args count]; i++) {
            NSLog (@"%d: %@\n", i, [args objectAtIndex:i]);
    }
    return @"success";
}
*/

/*
- (id)valueForUndefinedKey:(NSString *)key
{
    NSLog (@"%s:  key = %@", __PRETTY_FUNCTION__, key);
    return @"aValue";
}
*/

- (void)setValue:(id)value forUndefinedKey:(NSString *)key
{
    NSLog (@"%s:  key = %@", __PRETTY_FUNCTION__, key);
}

- init
{
    LOG ("\n");
    mySecondInterface = [[MySecondInterface alloc] init];
    return self;
}

- (void)dealloc
{
    LOG ("\n");
    [mySecondInterface release];
    [super dealloc];
}

- (int)getInt 
{
    LOG ("myInt = %d\n", myInt);
    return myInt;
}

- (void)setInt: (int)anInt 
{
    LOG ("anInt = %d\n", anInt);
    myInt = anInt;
}

- (NSString *)getString
{
    return string;
}

- (MySecondInterface *)getMySecondInterface 
{
    LOG ("\n");
    return mySecondInterface;
}

- (void)logMessage:(NSString *)message
{
    printf ("%s\n", [message lossyCString]);
}

- (void)logMessages:(id)messages
{
    int i, count = [[messages valueForKey:@"length"] intValue];
    for (i = 0; i < count; i++)
        printf ("%s\n", [[messages webScriptValueAtIndex:i] lossyCString]);
}

- (void)logMessage:(NSString *)message prefix:(NSString *)prefix
{
    printf ("%s:%s\n", [prefix lossyCString], [message lossyCString]);
}

- (void)setJSObject:(id)jso
{
    [jsobject autorelease];
    jsobject = [jso retain];
}

- (void)callJSObject:(int)arg1 :(int)arg2
{
    id foo1 = [jsobject callWebScriptMethod:@"call" withArguments:[NSArray arrayWithObjects:jsobject, [NSNumber numberWithInt:arg1], [NSNumber numberWithInt:arg2], nil]];
    printf ("foo (via call) = %s\n", [[foo1 description] lossyCString] );
    id foo2 = [jsobject callWebScriptMethod:@"apply" withArguments:[NSArray arrayWithObjects:jsobject, [NSArray arrayWithObjects:[NSNumber numberWithInt:arg1], [NSNumber numberWithInt:arg2], nil], nil]];
    printf ("foo (via apply) = %s\n", [[foo2 description] lossyCString] );
}

@end


using namespace JSC;
using namespace JSC::Bindings;

class GlobalImp : public ObjectImp {
public:
  virtual String className() const { return "global"; }
};

#define BufferSize 200000
static char code[BufferSize];

const char *readJavaScriptFromFile (const char *file)
{
    FILE *f = fopen(file, "r");
    if (!f) {
        fprintf(stderr, "Error opening %s.\n", file);
        return 0;
    }
    
    int num = fread(code, 1, BufferSize, f);
    code[num] = '\0';
    if(num >= BufferSize)
        fprintf(stderr, "Warning: File may have been too long.\n");

    fclose(f);
    
    return code;
}

int main(int argc, char **argv)
{
    // expecting a filename
    if (argc < 2) {
        fprintf(stderr, "You have to specify at least one filename\n");
        return -1;
    }
    
    bool ret = true;
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        
        JSLock lock;
        
        // create interpreter w/ global object
        Object global(new GlobalImp());
        Interpreter interp;
        interp.setGlobalObject(global);
        ExecState *exec = interp.globalExec();
        
        MyFirstInterface *myInterface = [[MyFirstInterface alloc] init];
        
        global.put(exec, Identifier("myInterface"), Instance::createRuntimeObject(Instance::ObjectiveCLanguage, (void *)myInterface));
        
        for (int i = 1; i < argc; i++) {
            const char *code = readJavaScriptFromFile(argv[i]);
            
            if (code) {
                // run
                Completion comp(interp.evaluate(code));
                
                if (comp.complType() == Throw) {
                    Value exVal = comp.value();
                    char *msg = exVal.toString(exec).ascii();
                    int lineno = -1;
                    if (exVal.type() == ObjectType) {
                        Value lineVal = Object::dynamicCast(exVal).get(exec,Identifier("line"));
                        if (lineVal.type() == NumberType)
                            lineno = int(lineVal.toNumber(exec));
                    }
                    if (lineno != -1)
                        fprintf(stderr,"Exception, line %d: %s\n",lineno,msg);
                    else
                        fprintf(stderr,"Exception: %s\n",msg);
                    ret = false;
                }
                else if (comp.complType() == ReturnValue) {
                    char *msg = comp.value().toString(interp.globalExec()).ascii();
                    fprintf(stderr,"Return value: %s\n",msg);
                }
            }
        }
        
        [myInterface release];
        [pool drain];
    } // end block, so that Interpreter and global get deleted
    
    return ret ? 0 : 3;
}
