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
    File Name:          15.7.4.3-3.js
    ECMA Section:       15.7.4.3.1 Number.prototype.valueOf()
    Description:
    Returns this number value.

    The valueOf function is not generic; it generates a runtime error if its
    this value is not a Number object. Therefore it cannot be transferred to
    other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               16 september 1997
*/
    var SECTION = "15.7.4.3-3-n";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Number.prototype.valueOf()");
    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

//    array[item++] = new TestCase("15.7.4.1", "v = Number.prototype.valueOf; num = 3; num.valueOf = v; num.valueOf()", "error",  "v = Number.prototype.valueOf; num = 3; num.valueOf = v; num.valueOf()" );
    array[item++] = new TestCase("15.7.4.1", "v = Number.prototype.valueOf; o = new String('Infinity'); o.valueOf = v; o.valueOf()", "error",  "v = Number.prototype.valueOf; o = new String('Infinity'); o.valueOf = v; o.valueOf()" );
//    array[item++] = new TestCase("15.7.4.1", "v = Number.prototype.valueOf; o = new Object(); o.valueOf = v; o.valueOf()", "error",  "v = Number.prototype.valueOf; o = new Object(); o.valueOf = v; o.valueOf()" );

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
