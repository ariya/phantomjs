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
    File Name:          15.8.1.1-2.js
    ECMA Section:       15.8.1.1.js
    Description:        All value properties of the Math object should have
                        the attributes [DontEnum, DontDelete, ReadOnly]

                        this test checks the DontDelete attribute of Math.E

    Author:             christine@netscape.com
    Date:               16 september 1997
*/
    var SECTION = "15.8.1.1-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.E";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var MATH_E = 2.7182818284590452354
    array[item++] = new TestCase( SECTION, "delete(Math.E)",                false,    "delete Math.E" );
    array[item++] = new TestCase( SECTION, "delete(Math.E); Math.E",        MATH_E,   "delete Math.E; Math.E" );
    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual = eval( testcases[tc].actual );
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "should not be able to delete property";
    }
    stopTest();
    return ( testcases );
}
