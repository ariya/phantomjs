/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          11.2.1-1.js
    ECMA Section:       11.2.1 Property Accessors
    Description:

    Properties are accessed by name, using either the dot notation:
                                MemberExpression . Identifier
                                CallExpression . Identifier

    or the bracket notation:    MemberExpression [ Expression ]
                                CallExpression [ Expression ]

    The dot notation is explained by the following syntactic conversion:
                                MemberExpression . Identifier
    is identical in its behavior to
                                MemberExpression [ <identifier-string> ]
    and similarly
                                CallExpression . Identifier
    is identical in its behavior to
                                CallExpression [ <identifier-string> ]
    where <identifier-string> is a string literal containing the same sequence
    of characters as the Identifier.

    The production MemberExpression : MemberExpression [ Expression ] is
    evaluated as follows:

    1.  Evaluate MemberExpression.
    2.  Call GetValue(Result(1)).
    3.  Evaluate Expression.
    4.  Call GetValue(Result(3)).
    5.  Call ToObject(Result(2)).
    6.  Call ToString(Result(4)).
    7.  Return a value of type Reference whose base object is Result(5) and
        whose property name is Result(6).

    The production CallExpression : CallExpression [ Expression ] is evaluated
    in exactly the same manner, except that the contained CallExpression is
    evaluated in step 1.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.2.1-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Property Accessors";
    writeHeaderToLog( SECTION + " "+TITLE );

    var testcases = new Array();

    // go through all Native Function objects, methods, and properties and get their typeof.

    var PROPERTY = new Array();
    var p = 0;

    // properties and functions of the global object

    PROPERTY[p++] = new Property( "this",   "NaN",          "number" );
    PROPERTY[p++] = new Property( "this",   "Infinity",     "number" );
    PROPERTY[p++] = new Property( "this",   "eval",         "function" );
    PROPERTY[p++] = new Property( "this",   "parseInt",     "function" );
    PROPERTY[p++] = new Property( "this",   "parseFloat",   "function" );
    PROPERTY[p++] = new Property( "this",   "escape",       "function" );
    PROPERTY[p++] = new Property( "this",   "unescape",     "function" );
    PROPERTY[p++] = new Property( "this",   "isNaN",        "function" );
    PROPERTY[p++] = new Property( "this",   "isFinite",     "function" );
    PROPERTY[p++] = new Property( "this",   "Object",       "function" );
    PROPERTY[p++] = new Property( "this",   "Number",       "function" );
    PROPERTY[p++] = new Property( "this",   "Function",     "function" );
    PROPERTY[p++] = new Property( "this",   "Array",        "function" );
    PROPERTY[p++] = new Property( "this",   "String",       "function" );
    PROPERTY[p++] = new Property( "this",   "Boolean",      "function" );
    PROPERTY[p++] = new Property( "this",   "Date",         "function" );
    PROPERTY[p++] = new Property( "this",   "Math",         "object" );

    // properties and  methods of Object objects

    PROPERTY[p++] = new Property( "Object", "prototype",    "object" );
    PROPERTY[p++] = new Property( "Object", "toString",     "function" );
    PROPERTY[p++] = new Property( "Object", "valueOf",      "function" );
    PROPERTY[p++] = new Property( "Object", "constructor",  "function" );

    // properties of the Function object

    PROPERTY[p++] = new Property( "Function",   "prototype",    "function" );
    PROPERTY[p++] = new Property( "Function.prototype",   "toString",     "function" );
    PROPERTY[p++] = new Property( "Function.prototype",   "length",       "number" );
    PROPERTY[p++] = new Property( "Function.prototype",   "valueOf",      "function" );

    Function.prototype.myProperty = "hi";

    PROPERTY[p++] = new Property( "Function.prototype",   "myProperty",   "string" );

    // properties of the Array object
    PROPERTY[p++] = new Property( "Array",      "prototype",    "object" );
    PROPERTY[p++] = new Property( "Array",      "length",       "number" );
    PROPERTY[p++] = new Property( "Array.prototype",      "constructor",  "function" );
    PROPERTY[p++] = new Property( "Array.prototype",      "toString",     "function" );
    PROPERTY[p++] = new Property( "Array.prototype",      "join",         "function" );
    PROPERTY[p++] = new Property( "Array.prototype",      "reverse",      "function" );
    PROPERTY[p++] = new Property( "Array.prototype",      "sort",         "function" );

    // properties of the String object
    PROPERTY[p++] = new Property( "String",     "prototype",    "object" );
    PROPERTY[p++] = new Property( "String",     "fromCharCode", "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "toString",     "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "constructor",  "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "valueOf",      "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "charAt",       "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "charCodeAt",   "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "indexOf",      "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "lastIndexOf",  "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "split",        "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "substring",    "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "toLowerCase",  "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "toUpperCase",  "function" );
    PROPERTY[p++] = new Property( "String.prototype",     "length",       "number" );

    // properties of the Boolean object
    PROPERTY[p++] = new Property( "Boolean",    "prototype",    "object" );
    PROPERTY[p++] = new Property( "Boolean",    "constructor",  "function" );
    PROPERTY[p++] = new Property( "Boolean.prototype",    "valueOf",      "function" );
    PROPERTY[p++] = new Property( "Boolean.prototype",    "toString",     "function" );

    // properties of the Number object

    PROPERTY[p++] = new Property( "Number",     "MAX_VALUE",    "number" );
    PROPERTY[p++] = new Property( "Number",     "MIN_VALUE",    "number" );
    PROPERTY[p++] = new Property( "Number",     "NaN",          "number" );
    PROPERTY[p++] = new Property( "Number",     "NEGATIVE_INFINITY",    "number" );
    PROPERTY[p++] = new Property( "Number",     "POSITIVE_INFINITY",    "number" );
    PROPERTY[p++] = new Property( "Number.prototype",     "toString",     "function" );
    PROPERTY[p++] = new Property( "Number.prototype",     "constructor",  "function" );
    PROPERTY[p++] = new Property( "Number.prototype",     "valueOf",        "function" );

    // properties of the Math Object.
    PROPERTY[p++] = new Property( "Math",   "E",        "number" );
    PROPERTY[p++] = new Property( "Math",   "LN10",     "number" );
    PROPERTY[p++] = new Property( "Math",   "LN2",      "number" );
    PROPERTY[p++] = new Property( "Math",   "LOG2E",    "number" );
    PROPERTY[p++] = new Property( "Math",   "LOG10E",   "number" );
    PROPERTY[p++] = new Property( "Math",   "PI",       "number" );
    PROPERTY[p++] = new Property( "Math",   "SQRT1_2",  "number" );
    PROPERTY[p++] = new Property( "Math",   "SQRT2",    "number" );
    PROPERTY[p++] = new Property( "Math",   "abs",      "function" );
    PROPERTY[p++] = new Property( "Math",   "acos",     "function" );
    PROPERTY[p++] = new Property( "Math",   "asin",     "function" );
    PROPERTY[p++] = new Property( "Math",   "atan",     "function" );
    PROPERTY[p++] = new Property( "Math",   "atan2",    "function" );
    PROPERTY[p++] = new Property( "Math",   "ceil",     "function" );
    PROPERTY[p++] = new Property( "Math",   "cos",      "function" );
    PROPERTY[p++] = new Property( "Math",   "exp",      "function" );
    PROPERTY[p++] = new Property( "Math",   "floor",    "function" );
    PROPERTY[p++] = new Property( "Math",   "log",      "function" );
    PROPERTY[p++] = new Property( "Math",   "max",      "function" );
    PROPERTY[p++] = new Property( "Math",   "min",      "function" );
    PROPERTY[p++] = new Property( "Math",   "pow",      "function" );
    PROPERTY[p++] = new Property( "Math",   "random",   "function" );
    PROPERTY[p++] = new Property( "Math",   "round",    "function" );
    PROPERTY[p++] = new Property( "Math",   "sin",      "function" );
    PROPERTY[p++] = new Property( "Math",   "sqrt",     "function" );
    PROPERTY[p++] = new Property( "Math",   "tan",      "function" );

    // properties of the Date object
    PROPERTY[p++] = new Property( "Date",   "parse",        "function" );
    PROPERTY[p++] = new Property( "Date",   "prototype",    "object" );
    PROPERTY[p++] = new Property( "Date",   "UTC",          "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "constructor",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "toString",       "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "valueOf",        "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getTime",        "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getYear",        "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getFullYear",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCFullYear", "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getMonth",       "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCMonth",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getDate",        "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCDate",     "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getDay",         "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCDay",      "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getHours",       "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCHours",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getMinutes",     "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCMinutes",  "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getSeconds",     "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCSeconds",  "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getMilliseconds","function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "getUTCMilliseconds", "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setTime",        "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setMilliseconds","function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setUTCMilliseconds", "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setSeconds",     "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setUTCSeconds",  "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setMinutes",     "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setUTCMinutes",  "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setHours",       "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setUTCHours",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setDate",        "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setUTCDate",     "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setMonth",       "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setUTCMonth",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setFullYear",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setUTCFullYear", "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "setYear",        "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "toLocaleString", "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "toUTCString",    "function" );
    PROPERTY[p++] = new Property( "Date.prototype",   "toGMTString",    "function" );

    for ( var i = 0, RESULT; i < PROPERTY.length; i++ ) {
        RESULT = eval("typeof " + PROPERTY[i].object + "." + PROPERTY[i].name );

        testcases[tc++] = new TestCase( SECTION,
                                        "typeof " + PROPERTY[i].object + "." + PROPERTY[i].name,
                                        PROPERTY[i].type,
                                        RESULT );

        RESULT = eval("typeof " + PROPERTY[i].object + "['" + PROPERTY[i].name +"']");

        testcases[tc++] = new TestCase( SECTION,
                                        "typeof " + PROPERTY[i].object + "['" + PROPERTY[i].name +"']",
                                        PROPERTY[i].type,
                                        RESULT );
    }

    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
function MyObject( arg0, arg1, arg2, arg3, arg4 ) {
    this.name   = arg0;
}
function Property( object, name, type ) {
    this.object = object;
    this.name = name;
    this.type = type;
}
