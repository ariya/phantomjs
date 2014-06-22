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
    File Name:          15-2.js
    ECMA Section:       15 Native ECMAScript Objects

    Description:        Every built-in function and every built-in constructor
                        has the Function prototype object, which is the value of
                        the expression Function.prototype as the value of its
                        internal [[Prototype]] property, except the Function
                        prototype object itself.

                        That is, the __proto__ property of builtin functions and
                        constructors should be the Function.prototype object.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Native ECMAScript Objects";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();
function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Object.__proto__",   Function.prototype,   Object.__proto__ );
    array[item++] = new TestCase( SECTION,  "Array.__proto__",    Function.prototype,   Array.__proto__ );
    array[item++] = new TestCase( SECTION,  "String.__proto__",   Function.prototype,   String.__proto__ );
    array[item++] = new TestCase( SECTION,  "Boolean.__proto__",  Function.prototype,   Boolean.__proto__ );
    array[item++] = new TestCase( SECTION,  "Number.__proto__",   Function.prototype,   Number.__proto__ );
    array[item++] = new TestCase( SECTION,  "Date.__proto__",     Function.prototype,   Date.__proto__ );
    array[item++] = new TestCase( SECTION,  "TestCase.__proto__", Function.prototype,   TestCase.__proto__ );

    array[item++] = new TestCase( SECTION,  "eval.__proto__",     Function.prototype,   eval.__proto__ );
    array[item++] = new TestCase( SECTION,  "Math.pow.__proto__", Function.prototype,   Math.pow.__proto__ );
    array[item++] = new TestCase( SECTION,  "String.prototype.indexOf.__proto__", Function.prototype,   String.prototype.indexOf.__proto__ );

    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual = eval( testcases[tc].actual );
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
