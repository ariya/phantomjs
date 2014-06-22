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
    File Name:          15.8.2.14.js
    ECMA Section:       15.8.2.14 Math.random()
                        returns a number value x with a positive sign
                        with 1 > x >= 0 with approximately uniform
                        distribution over that range, using an
                        implementation-dependent algorithm or strategy.
                        This function takes no arguments.

    Description:
    Author:             christine@netscape.com
    Date:               16 september 1997
*/

    var SECTION = "15.8.2.14";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.random()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

    for ( item = 0; item < 100; item++ ) {
        array[item] = new TestCase( SECTION,  "Math.random()",    "pass",    null );
    }

    return ( array );
}
function getRandom( caseno ) {
    testcases[caseno].reason = Math.random();
    testcases[caseno].actual = "pass";

    if ( ! ( testcases[caseno].reason >= 0) ) {
        testcases[caseno].actual = "fail";
    }

    if ( ! (testcases[caseno].reason < 1) ) {
        testcases[caseno].actual = "fail";
    }
}

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        getRandom( tc );
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
