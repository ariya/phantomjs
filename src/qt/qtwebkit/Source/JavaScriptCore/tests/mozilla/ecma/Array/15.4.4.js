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
    File Name:          15.4.4.js
    ECMA Section:       15.4.4 Properties of the Array Prototype Object
    Description:        The value of the internal [[Prototype]] property of
                        the Array prototype object is the Object prototype
                        object.

                        Note that the Array prototype object is itself an
                        array; it has a length property (whose initial value
                        is (0) and the special [[Put]] method.

    Author:             christine@netscape.com
    Date:               7 october 1997
*/

    var SECTION = "15.4.4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Properties of the Array Prototype Object";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

//  these testcases are ECMA_2
//    array[item++] = new TestCase( SECTION,	"Array.prototype.__proto__",          Object.prototype,           Array.prototype.__proto__ );
//    array[item++] = new TestCase( SECTION,	"Array.__proto__.valueOf == Object.__proto__.valueOf",  true, (Array.__proto__.valueOf == Object.__proto__.valueOf) );

    array[item++] = new TestCase( SECTION,	"Array.prototype.length",   0,          Array.prototype.length );

//  verify that prototype object is an Array object.
    array[item++] = new TestCase( SECTION,	"typeof Array.prototype",    "object",   typeof Array.prototype );

    array[item++] = new TestCase( SECTION,
                                    "Array.prototype.toString = Object.prototype.toString; Array.prototype.toString()",
                                    "[object Array]",
                                    eval("Array.prototype.toString = Object.prototype.toString; Array.prototype.toString()") );

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
