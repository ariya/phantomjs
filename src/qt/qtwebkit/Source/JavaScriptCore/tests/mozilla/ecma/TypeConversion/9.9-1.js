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
    File Name:          9.9-1.js
    ECMA Section:       9.9  Type Conversion:  ToObject
    Description:

                        undefined   generate a runtime error
                        null        generate a runtime error
                        boolean     create a new Boolean object whose default
                                    value is the value of the boolean.
                        number      Create a new Number object whose default
                                    value is the value of the number.
                        string      Create a new String object whose default
                                    value is the value of the string.
                        object      Return the input argument (no conversion).
    Author:             christine@netscape.com
    Date:               17 july 1997
*/

    var VERSION = "ECMA_1";
    startTest();
    var SECTION = "9.9-1";

    writeHeaderToLog( SECTION + " Type Conversion: ToObject" );
    var tc= 0;
    var testcases = getTestCases();

//  all tests must call a function that returns an array of TestCase objects.
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "Object(true).valueOf()",    true,                   (Object(true)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(true)",       "object",               typeof Object(true) );
    array[item++] = new TestCase( SECTION, "(Object(true)).__proto__",  Boolean.prototype,      (Object(true)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(false).valueOf()",    false,                  (Object(false)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(false)",      "object",               typeof Object(false) );
    array[item++] = new TestCase( SECTION, "(Object(true)).__proto__",  Boolean.prototype,      (Object(true)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(0).valueOf()",       0,                      (Object(0)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(0)",          "object",               typeof Object(0) );
    array[item++] = new TestCase( SECTION, "(Object(0)).__proto__",     Number.prototype,      (Object(0)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(-0).valueOf()",      -0,                     (Object(-0)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(-0)",         "object",               typeof Object(-0) );
    array[item++] = new TestCase( SECTION, "(Object(-0)).__proto__",    Number.prototype,      (Object(-0)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(1).valueOf()",       1,                      (Object(1)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(1)",          "object",               typeof Object(1) );
    array[item++] = new TestCase( SECTION, "(Object(1)).__proto__",     Number.prototype,      (Object(1)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(-1).valueOf()",      -1,                     (Object(-1)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(-1)",         "object",               typeof Object(-1) );
    array[item++] = new TestCase( SECTION, "(Object(-1)).__proto__",    Number.prototype,      (Object(-1)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(Number.MAX_VALUE).valueOf()",    1.7976931348623157e308,         (Object(Number.MAX_VALUE)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(Number.MAX_VALUE)",       "object",                       typeof Object(Number.MAX_VALUE) );
    array[item++] = new TestCase( SECTION, "(Object(Number.MAX_VALUE)).__proto__",  Number.prototype,               (Object(Number.MAX_VALUE)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(Number.MIN_VALUE).valueOf()",     5e-324,           (Object(Number.MIN_VALUE)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(Number.MIN_VALUE)",       "object",         typeof Object(Number.MIN_VALUE) );
    array[item++] = new TestCase( SECTION, "(Object(Number.MIN_VALUE)).__proto__",  Number.prototype, (Object(Number.MIN_VALUE)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(Number.POSITIVE_INFINITY).valueOf()",    Number.POSITIVE_INFINITY,       (Object(Number.POSITIVE_INFINITY)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(Number.POSITIVE_INFINITY)",       "object",                       typeof Object(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "(Object(Number.POSITIVE_INFINITY)).__proto__",  Number.prototype,               (Object(Number.POSITIVE_INFINITY)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(Number.NEGATIVE_INFINITY).valueOf()",    Number.NEGATIVE_INFINITY,       (Object(Number.NEGATIVE_INFINITY)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(Number.NEGATIVE_INFINITY)",       "object",            typeof Object(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "(Object(Number.NEGATIVE_INFINITY)).__proto__",  Number.prototype,   (Object(Number.NEGATIVE_INFINITY)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object(Number.NaN).valueOf()",      Number.NaN,                (Object(Number.NaN)).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object(Number.NaN)",         "object",                  typeof Object(Number.NaN) );
    array[item++] = new TestCase( SECTION, "(Object(Number.NaN)).__proto__",    Number.prototype,          (Object(Number.NaN)).__proto__ );

    array[item++] = new TestCase( SECTION, "Object('a string').valueOf()",      "a string",         (Object("a string")).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object('a string')",         "object",           typeof (Object("a string")) );
    array[item++] = new TestCase( SECTION, "(Object('a string')).__proto__",    String.prototype,   (Object("a string")).__proto__ );

    array[item++] = new TestCase( SECTION, "Object('').valueOf()",              "",                 (Object("")).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object('')",                 "object",           typeof (Object("")) );
    array[item++] = new TestCase( SECTION, "(Object('')).__proto__",            String.prototype,   (Object("")).__proto__ );

    array[item++] = new TestCase( SECTION, "Object('\\r\\t\\b\\n\\v\\f').valueOf()",   "\r\t\b\n\v\f",   (Object("\r\t\b\n\v\f")).valueOf() );
    array[item++] = new TestCase( SECTION, "typeof Object('\\r\\t\\b\\n\\v\\f')",      "object",           typeof (Object("\\r\\t\\b\\n\\v\\f")) );
    array[item++] = new TestCase( SECTION, "(Object('\\r\\t\\b\\n\\v\\f')).__proto__", String.prototype,   (Object("\\r\\t\\b\\n\\v\\f")).__proto__ );

    array[item++] = new TestCase( SECTION,  "Object( '\\\'\\\"\\' ).valueOf()",      "\'\"\\",          (Object("\'\"\\")).valueOf() );
    array[item++] = new TestCase( SECTION,  "typeof Object( '\\\'\\\"\\' )",        "object",           typeof Object("\'\"\\") );
    array[item++] = new TestCase( SECTION,  "Object( '\\\'\\\"\\' ).__proto__",      String.prototype,   (Object("\'\"\\")).__proto__ );

    array[item++] = new TestCase( SECTION, "Object( new MyObject(true) ).valueOf()",    true,           eval("Object( new MyObject(true) ).valueOf()") );
    array[item++] = new TestCase( SECTION, "typeof Object( new MyObject(true) )",       "object",       eval("typeof Object( new MyObject(true) )") );
    array[item++] = new TestCase( SECTION, "(Object( new MyObject(true) )).toString()",  "[object Object]",       eval("(Object( new MyObject(true) )).toString()") );

    return ( array );
}

function test() {
        for ( tc = 0; tc < testcases.length; tc++ ) {

            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+
                    testcases[tc].actual );

            testcases[tc].reason +=
                    ( testcases[tc].passed ) ? "" : "wrong value ";

        }
        stopTest();

    //  all tests must return an array of TestCase objects
        return ( testcases );
}
function MyObject( value ) {
    this.value = value;
    this.valueOf = new Function ( "return this.value" );
}