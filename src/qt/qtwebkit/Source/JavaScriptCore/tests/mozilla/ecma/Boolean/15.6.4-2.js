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
    File Name:          15.6.4-2.js
    ECMA Section:       15.6.4 Properties of the Boolean Prototype Object

    Description:
    The Boolean prototype object is itself a Boolean object (its [[Class]] is
    "Boolean") whose value is false.

    The value of the internal [[Prototype]] property of the Boolean prototype object
    is the Object prototype object (15.2.3.1).

    Author:             christine@netscape.com
    Date:               30 september 1997

*/


    var VERSION = "ECMA_2"
    startTest();
    var SECTION = "15.6.4-2";

    writeHeaderToLog( SECTION + " Properties of the Boolean Prototype Object");
    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "Boolean.prototype.__proto__",               Object.prototype,       Boolean.prototype.__proto__ );

    return ( array );
}

function test() {
    for (tc = 0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
        testcases[tc].expect,
        testcases[tc].actual,
        testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value "
    }
    stopTest();
    return ( testcases );
}
