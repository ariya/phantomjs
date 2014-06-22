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
    File Name:          15.7.4.js
    ECMA Section:       15.7.4

    Description:

    The Number prototype object is itself a Number object (its [[Class]] is
    "Number") whose value is +0.

    The value of the internal [[Prototype]] property of the Number prototype
    object is the Object prototype object (15.2.3.1).

    In following descriptions of functions that are properties of the Number
    prototype object, the phrase "this Number object" refers to the object
    that is the this value for the invocation of the function; it is an error
    if this does not refer to an object for which the value of the internal
    [[Class]] property is "Number". Also, the phrase "this number value" refers
    to the number value represented by this Number object, that is, the value
    of the internal [[Value]] property of this Number object.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "15.7.4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Properties of the Number Prototype Object";

    writeHeaderToLog( SECTION + " "+TITLE);

    var testcases = getTestCases();
    test( testcases );

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase( SECTION,
                                  "Number.prototype.toString=Object.prototype.toString;Number.prototype.toString()",
                                  "[object Number]",
                                  eval("Number.prototype.toString=Object.prototype.toString;Number.prototype.toString()") );
    array[item++] = new TestCase( SECTION, "typeof Number.prototype",                           "object",    typeof Number.prototype );
    array[item++] = new TestCase( SECTION, "Number.prototype.valueOf()",                        0,          Number.prototype.valueOf() );


//    The __proto__ property cannot be used in ECMA_1 tests.
//    array[item++] = new TestCase( SECTION, "Number.prototype.__proto__",                        Object.prototype,   Number.prototype.__proto__ );
//    array[item++] = new TestCase( SECTION, "Number.prototype.__proto__ == Object.prototype",    true,       Number.prototype.__proto__ == Object.prototype );


    return ( array );
}
function test( ) {
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
