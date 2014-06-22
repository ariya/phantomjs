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
    File Name:          15.8.1.3-3.js
    ECMA Section:       15.8.1.3.js
    Description:        All value properties of the Math object should have
                        the attributes [DontEnum, DontDelete, ReadOnly]

                        this test checks the DontDelete attribute of Math.LN2

    Author:             christine@netscape.com
    Date:               16 september 1997
*/

    var SECTION = "15.8.1.3-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.LN2";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

    var MATH_LN2 = 0.6931471805599453;

    array[item++] = new TestCase( SECTION, "delete(Math.LN2)",              false,          "delete(Math.LN2)" );
    array[item++] = new TestCase( SECTION, "delete(Math.LN2); Math.LN2",    MATH_LN2,       "delete(Math.LN2); Math.LN2" );

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

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "property should be read-only ";
    }
    stopTest();
    return ( testcases );
}
