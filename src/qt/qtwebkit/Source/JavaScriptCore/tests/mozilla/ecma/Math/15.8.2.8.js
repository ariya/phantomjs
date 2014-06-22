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
    File Name:          15.8.2.8.js
    ECMA Section:       15.8.2.8  Math.exp(x)
    Description:        return an approximation to the exponential function of
                        the argument (e raised to the power of the argument)
                        special cases:
                        -   if x is NaN         return NaN
                        -   if x is 0           return 1
                        -   if x is -0          return 1
                        -   if x is Infinity    return Infinity
                        -   if x is -Infinity   return 0
    Author:             christine@netscape.com
    Date:               7 july 1997
*/


    var SECTION = "15.8.2.8";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.exp(x)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "Math.exp.length",     1,              Math.exp.length );

    array[item++] = new TestCase( SECTION,   "Math.exp()",          Number.NaN,     Math.exp() );
    array[item++] = new TestCase( SECTION,   "Math.exp(null)",      1,              Math.exp(null) );
    array[item++] = new TestCase( SECTION,   "Math.exp(void 0)",    Number.NaN,     Math.exp(void 0) );
    array[item++] = new TestCase( SECTION,   "Math.exp(1)",          Math.E,         Math.exp(1) );
    array[item++] = new TestCase( SECTION,   "Math.exp(true)",      Math.E,         Math.exp(true) );
    array[item++] = new TestCase( SECTION,   "Math.exp(false)",     1,              Math.exp(false) );

    array[item++] = new TestCase( SECTION,   "Math.exp('1')",       Math.E,         Math.exp('1') );
    array[item++] = new TestCase( SECTION,   "Math.exp('0')",       1,              Math.exp('0') );

    array[item++] = new TestCase( SECTION,   "Math.exp(NaN)",       Number.NaN,      Math.exp(Number.NaN) );
    array[item++] = new TestCase( SECTION,   "Math.exp(0)",          1,              Math.exp(0)          );
    array[item++] = new TestCase( SECTION,   "Math.exp(-0)",         1,              Math.exp(-0)         );
    array[item++] = new TestCase( SECTION,   "Math.exp(Infinity)",   Number.POSITIVE_INFINITY,   Math.exp(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.exp(-Infinity)",  0,              Math.exp(Number.NEGATIVE_INFINITY) );

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
