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
    File Name:          15.js
    ECMA Section:       15 Native ECMAScript Objects
    Description:        Every built-in prototype object has the Object prototype
                        object, which is the value of the expression
                        Object.prototype (15.2.3.1) as the value of its internal
                        [[Prototype]] property, except the Object prototype
                        object itself.

                        Every native object associated with a program-created
                        function also has the Object prototype object as the
                        value of its internal [[Prototype]] property.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Native ECMAScript Objects";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
/*
    array[item++] = new TestCase( SECTION,  "Function.prototype.__proto__", Object.prototype,   Function.prototype.__proto__ );
    array[item++] = new TestCase( SECTION,  "Array.prototype.__proto__",    Object.prototype,   Array.prototype.__proto__ );
    array[item++] = new TestCase( SECTION,  "String.prototype.__proto__",   Object.prototype,   String.prototype.__proto__ );
    array[item++] = new TestCase( SECTION,  "Boolean.prototype.__proto__",  Object.prototype,   Boolean.prototype.__proto__ );
    array[item++] = new TestCase( SECTION,  "Number.prototype.__proto__",   Object.prototype,   Number.prototype.__proto__ );
//    array[item++] = new TestCase( SECTION,  "Math.prototype.__proto__",     Object.prototype,   Math.prototype.__proto__ );
    array[item++] = new TestCase( SECTION,  "Date.prototype.__proto__",     Object.prototype,   Date.prototype.__proto__ );
    array[item++] = new TestCase( SECTION,  "TestCase.prototype.__proto__", Object.prototype,   TestCase.prototype.__proto__ );

    array[item++] = new TestCase( SECTION,  "MyObject.prototype.__proto__", Object.prototype,   MyObject.prototype.__proto__ );
*/
    array[item++] = new TestCase( SECTION,  "Function.prototype.__proto__ == Object.prototype", true,   Function.prototype.__proto__ == Object.prototype );
    array[item++] = new TestCase( SECTION,  "Array.prototype.__proto__ == Object.prototype",    true,   Array.prototype.__proto__ == Object.prototype );
    array[item++] = new TestCase( SECTION,  "String.prototype.__proto__ == Object.prototype",   true,   String.prototype.__proto__ == Object.prototype );
    array[item++] = new TestCase( SECTION,  "Boolean.prototype.__proto__ == Object.prototype",  true,   Boolean.prototype.__proto__ == Object.prototype );
    array[item++] = new TestCase( SECTION,  "Number.prototype.__proto__ == Object.prototype",   true,   Number.prototype.__proto__ == Object.prototype );
//    array[item++] = new TestCase( SECTION,  "Math.prototype.__proto__ == Object.prototype",     true,   Math.prototype.__proto__ == Object.prototype );
    array[item++] = new TestCase( SECTION,  "Date.prototype.__proto__ == Object.prototype",     true,   Date.prototype.__proto__ == Object.prototype );
    array[item++] = new TestCase( SECTION,  "TestCase.prototype.__proto__ == Object.prototype", true,   TestCase.prototype.__proto__ == Object.prototype );

    array[item++] = new TestCase( SECTION,  "MyObject.prototype.__proto__ == Object.prototype", true,   MyObject.prototype.__proto__ == Object.prototype );


    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {

        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }

    stopTest();
    return ( testcases );
}

function MyObject( value ) {
    this.value = value;
    this.valueOf = new Function( "return this.value" );
}
