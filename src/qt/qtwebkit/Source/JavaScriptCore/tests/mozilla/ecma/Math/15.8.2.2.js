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
    File Name:          15.8.2.2.js
    ECMA Section:       15.8.2.2 acos( x )
    Description:        return an approximation to the arc cosine of the
                        argument.  the result is expressed in radians and
                        range is from +0 to +PI.  special cases:
                        - if x is NaN, return NaN
                        - if x > 1, the result is NaN
                        - if x < -1, the result is NaN
                        - if x == 1, the result is +0
    Author:             christine@netscape.com
    Date:               7 july 1997
*/
    var SECTION = "15.8.2.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.acos()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Math.acos.length",         1,              Math.acos.length );

    array[item++] = new TestCase( SECTION,  "Math.acos(void 0)",        Number.NaN,     Math.acos(void 0) );
    array[item++] = new TestCase( SECTION,  "Math.acos()",              Number.NaN,     Math.acos() );
    array[item++] = new TestCase( SECTION,  "Math.acos(null)",          Math.PI/2,      Math.acos(null) );
    array[item++] = new TestCase( SECTION,  "Math.acos(NaN)",           Number.NaN,     Math.acos(Number.NaN) );

    array[item++] = new TestCase( SECTION,  "Math.acos(a string)",      Number.NaN,     Math.acos("a string") );
    array[item++] = new TestCase( SECTION,  "Math.acos('0')",           Math.PI/2,      Math.acos('0') );
    array[item++] = new TestCase( SECTION,  "Math.acos('1')",           0,              Math.acos('1') );
    array[item++] = new TestCase( SECTION,  "Math.acos('-1')",          Math.PI,        Math.acos('-1') );

    array[item++] = new TestCase( SECTION,  "Math.acos(1.00000001)",    Number.NaN,     Math.acos(1.00000001) );
    array[item++] = new TestCase( SECTION,  "Math.acos(11.00000001)",   Number.NaN,     Math.acos(-1.00000001) );
    array[item++] = new TestCase( SECTION,  "Math.acos(1)",    	        0,              Math.acos(1)          );
    array[item++] = new TestCase( SECTION,  "Math.acos(-1)",            Math.PI,        Math.acos(-1)         );
    array[item++] = new TestCase( SECTION,  "Math.acos(0)",    	        Math.PI/2,      Math.acos(0)          );
    array[item++] = new TestCase( SECTION,  "Math.acos(-0)",   	        Math.PI/2,      Math.acos(-0)         );
    array[item++] = new TestCase( SECTION,  "Math.acos(Math.SQRT1_2)",	Math.PI/4,      Math.acos(Math.SQRT1_2));
    array[item++] = new TestCase( SECTION,  "Math.acos(-Math.SQRT1_2)", Math.PI/4*3,    Math.acos(-Math.SQRT1_2));
    array[item++] = new TestCase( SECTION,  "Math.acos(0.9999619230642)",	Math.PI/360,    Math.acos(0.9999619230642));

    return ( array );
}

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
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
