/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "BridgeJSC.h"
#include "JSObject.h"
#include "JSValue.h"
#include "interpreter.h"
#include "npruntime_internal.h"
#include "runtime_object.h"
#include "types.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


#define LOG(formatAndArgs...) { \
    fprintf (stderr, "%s:  ", __PRETTY_FUNCTION__); \
    fprintf(stderr, formatAndArgs); \
}


// ------------------ NP Interface definition --------------------
typedef struct
{
    NPObject object;
    double doubleValue;
    int intValue;
    NPVariant stringValue;
    bool boolValue;
} MyObject;


static bool identifiersInitialized = false;

#define ID_DOUBLE_VALUE                         0
#define ID_INT_VALUE                            1
#define ID_STRING_VALUE                         2
#define ID_BOOLEAN_VALUE                        3
#define ID_NULL_VALUE                           4
#define ID_UNDEFINED_VALUE                      5
#define NUM_PROPERTY_IDENTIFIERS                6

static NPIdentifier myPropertyIdentifiers[NUM_PROPERTY_IDENTIFIERS];
static const NPUTF8 *myPropertyIdentifierNames[NUM_PROPERTY_IDENTIFIERS] = {
    "doubleValue",
    "intValue",
    "stringValue",
    "booleanValue",
    "nullValue",
    "undefinedValue"
};

#define ID_LOG_MESSAGE                          0
#define ID_SET_DOUBLE_VALUE                     1
#define ID_SET_INT_VALUE                        2
#define ID_SET_STRING_VALUE                     3
#define ID_SET_BOOLEAN_VALUE                    4
#define ID_GET_DOUBLE_VALUE                     5
#define ID_GET_INT_VALUE                        6
#define ID_GET_STRING_VALUE                     7
#define ID_GET_BOOLEAN_VALUE                    8
#define NUM_METHOD_IDENTIFIERS                  9

static NPIdentifier myMethodIdentifiers[NUM_METHOD_IDENTIFIERS];
static const NPUTF8 *myMethodIdentifierNames[NUM_METHOD_IDENTIFIERS] = {
    "logMessage",
    "setDoubleValue",
    "setIntValue",
    "setStringValue",
    "setBooleanValue",
    "getDoubleValue",
    "getIntValue",
    "getStringValue",
    "getBooleanValue"
};

static void initializeIdentifiers()
{
    NPN_GetStringIdentifiers (myPropertyIdentifierNames, NUM_PROPERTY_IDENTIFIERS, myPropertyIdentifiers);
    NPN_GetStringIdentifiers (myMethodIdentifierNames, NUM_METHOD_IDENTIFIERS, myMethodIdentifiers);
};

bool myHasProperty (NPClass *theClass, NPIdentifier name)
{
    int i;
    for (i = 0; i < NUM_PROPERTY_IDENTIFIERS; i++) {
        if (name == myPropertyIdentifiers[i]){
            return true;
        }
    }
    return false;
}

bool myHasMethod (NPClass *theClass, NPIdentifier name)
{
    int i;
    for (i = 0; i < NUM_METHOD_IDENTIFIERS; i++) {
        if (name == myMethodIdentifiers[i]){
            return true;
        }
    }
    return false;
}


void logMessage (const NPVariant *message)
{
    if (message->type == NPVariantStringType) {
        char msgBuf[1024];
        strncpy (msgBuf, message->value.stringValue.UTF8Characters, message->value.stringValue.UTF8Length);
        msgBuf[message->value.stringValue.UTF8Length] = 0;
        printf ("%s\n", msgBuf);
    }
    else if (message->type == NPVariantDoubleType)
        printf ("%f\n", (float)message->value.doubleValue);
    else if (message->type == NPVariantInt32Type)
        printf ("%d\n", message->value.intValue);
    else if (message->type == NPVariantObjectType)
        printf ("%p\n", message->value.objectValue);
}

void setDoubleValue (MyObject *obj, const NPVariant *variant)
{
    if (!NPN_VariantToDouble (variant, &obj->doubleValue)) {
        NPUTF8 *msg = "Attempt to set double value with invalid type.";
        NPString aString;
        aString.UTF8Characters = msg;
        aString.UTF8Length = strlen (msg);
        NPN_SetException ((NPObject *)obj, &aString);
    }
}

void setIntValue (MyObject *obj, const NPVariant *variant)
{
    if (!NPN_VariantToInt32 (variant, &obj->intValue)) {
        NPUTF8 *msg = "Attempt to set int value with invalid type.";
        NPString aString;
        aString.UTF8Characters = msg;
        aString.UTF8Length = strlen (msg);
        NPN_SetException ((NPObject *)obj, &aString);
    }
}

void setStringValue (MyObject *obj, const NPVariant *variant)
{
    NPN_ReleaseVariantValue (&obj->stringValue);
    NPN_InitializeVariantWithVariant (&obj->stringValue, variant);
}

void setBooleanValue (MyObject *obj, const NPVariant *variant)
{
    if (!NPN_VariantToBool (variant, (NPBool *)&obj->boolValue)) {
        NPUTF8 *msg = "Attempt to set bool value with invalid type.";
        NPString aString;
        aString.UTF8Characters = msg;
        aString.UTF8Length = strlen (msg);
        NPN_SetException ((NPObject *)obj, &aString);
    }
}

void getDoubleValue (MyObject *obj, NPVariant *variant)
{
    NPN_InitializeVariantWithDouble (variant, obj->doubleValue);
}

void getIntValue (MyObject *obj, NPVariant *variant)
{
    NPN_InitializeVariantWithInt32 (variant, obj->intValue);
}

void getStringValue (MyObject *obj, NPVariant *variant)
{
    NPN_InitializeVariantWithVariant (variant, &obj->stringValue);
}

void getBooleanValue (MyObject *obj, NPVariant *variant)
{
    NPN_InitializeVariantWithBool (variant, obj->boolValue);
}

void myGetProperty (MyObject *obj, NPIdentifier name, NPVariant *variant)
{
    if (name == myPropertyIdentifiers[ID_DOUBLE_VALUE]){
        getDoubleValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_INT_VALUE]){
        getIntValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_STRING_VALUE]){
        getStringValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_BOOLEAN_VALUE]){
        getBooleanValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_NULL_VALUE]){
        return NPN_InitializeVariantAsNull (variant);
    }
    else if (name == myPropertyIdentifiers[ID_UNDEFINED_VALUE]){
        return NPN_InitializeVariantAsUndefined (variant); 
    }
    else
        NPN_InitializeVariantAsUndefined(variant);
}

void mySetProperty (MyObject *obj, NPIdentifier name, const NPVariant *variant)
{
    if (name == myPropertyIdentifiers[ID_DOUBLE_VALUE]) {
        setDoubleValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_INT_VALUE]) {
        setIntValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_STRING_VALUE]) {
        setStringValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_BOOLEAN_VALUE]) {
        setBooleanValue (obj, variant);
    }
    else if (name == myPropertyIdentifiers[ID_NULL_VALUE]) {
        // Do nothing!
    }
    else if (name == myPropertyIdentifiers[ID_UNDEFINED_VALUE]) {
        // Do nothing!
    }
}

void myInvoke (MyObject *obj, NPIdentifier name, NPVariant *args, unsigned argCount, NPVariant *result)
{
    if (name == myMethodIdentifiers[ID_LOG_MESSAGE]) {
        if (argCount == 1 && NPN_VariantIsString(&args[0]))
            logMessage (&args[0]);
        NPN_InitializeVariantAsVoid (result);
    }
    else if (name == myMethodIdentifiers[ID_SET_DOUBLE_VALUE]) {
        if (argCount == 1 && NPN_VariantIsDouble (&args[0]))
            setDoubleValue (obj, &args[0]);
        NPN_InitializeVariantAsVoid (result);
    }
    else if (name == myMethodIdentifiers[ID_SET_INT_VALUE]) {
        if (argCount == 1 && (NPN_VariantIsDouble (&args[0]) || NPN_VariantIsInt32 (&args[0])))
            setIntValue (obj, &args[0]);
        NPN_InitializeVariantAsVoid (result);
    }
    else if (name == myMethodIdentifiers[ID_SET_STRING_VALUE]) {
        if (argCount == 1 && NPN_VariantIsString (&args[0]))
            setStringValue (obj, &args[0]);
        NPN_InitializeVariantAsVoid (result);
    }
    else if (name == myMethodIdentifiers[ID_SET_BOOLEAN_VALUE]) {
        if (argCount == 1 && NPN_VariantIsBool (&args[0]))
            setBooleanValue (obj, &args[0]);
        NPN_InitializeVariantAsVoid (result);
    }
    else if (name == myMethodIdentifiers[ID_GET_DOUBLE_VALUE]) {
        getDoubleValue (obj, result);
    }
    else if (name == myMethodIdentifiers[ID_GET_INT_VALUE]) {
        getIntValue (obj, result);
    }
    else if (name == myMethodIdentifiers[ID_GET_STRING_VALUE]) {
        getStringValue (obj, result);
    }
    else if (name == myMethodIdentifiers[ID_GET_BOOLEAN_VALUE]) {
        getBooleanValue (obj, result);
    }
    else 
        NPN_InitializeVariantAsUndefined (result);
}

NPObject *myAllocate ()
{
    MyObject *newInstance = (MyObject *)malloc (sizeof(MyObject));
    
    if (!identifiersInitialized) {
        identifiersInitialized = true;
        initializeIdentifiers();
    }
    
    
    newInstance->doubleValue = 666.666;
    newInstance->intValue = 1234;
    newInstance->boolValue = true;
    newInstance->stringValue.type = NPVariantType_String;
    newInstance->stringValue.value.stringValue.UTF8Length = strlen ("Hello world");
    newInstance->stringValue.value.stringValue.UTF8Characters = strdup ("Hello world");
    
    return (NPObject *)newInstance;
}

void myInvalidate ()
{
    // Make sure we've released any remaining references to JavaScript objects.
}

void myDeallocate (MyObject *obj) 
{
    free ((void *)obj);
}

static NPClass _myFunctionPtrs = { 
    kNPClassStructVersionCurrent,
    (NPAllocateFunctionPtr) myAllocate, 
    (NPDeallocateFunctionPtr) myDeallocate, 
    (NPInvalidateFunctionPtr) myInvalidate,
    (NPHasMethodFunctionPtr) myHasMethod,
    (NPInvokeFunctionPtr) myInvoke,
    (NPHasPropertyFunctionPtr) myHasProperty,
    (NPGetPropertyFunctionPtr) myGetProperty,
    (NPSetPropertyFunctionPtr) mySetProperty,
};
static NPClass *myFunctionPtrs = &_myFunctionPtrs;

// --------------------------------------------------------

using namespace JSC;
using namespace JSC::Bindings;

class GlobalImp : public ObjectImp {
public:
  virtual UString className() const { return "global"; }
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
        JSLock lock;
        
        // create interpreter w/ global object
        Object global(new GlobalImp());
        Interpreter interp;
        interp.setGlobalObject(global);
        ExecState *exec = interp.globalExec();
        
        MyObject *myObject = (MyObject *)NPN_CreateObject (myFunctionPtrs);
        
        global.put(exec, Identifier("myInterface"), Instance::createRuntimeObject(Instance::CLanguage, (void *)myObject));
        
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
                
        NPN_ReleaseObject ((NPObject *)myObject);
        
    } // end block, so that Interpreter and global get deleted
    
    return ret ? 0 : 3;
}
