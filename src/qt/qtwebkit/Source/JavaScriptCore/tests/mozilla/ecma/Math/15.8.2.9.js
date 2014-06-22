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
    File Name:          15.8.2.9.js
    ECMA Section:       15.8.2.9  Math.floor(x)
    Description:        return the greatest number value that is not greater
                        than the argument and is equal to a mathematical integer.
                        if the number is already an integer, return the number
                        itself.  special cases:
                            - if x is NaN       return NaN
                            - if x = +0         return +0
                            - if x = -0          return -0
                            - if x = Infinity   return Infinity
                            - if x = -Infinity  return -Infinity
                            - if ( -1 < x < 0 ) return -0
                        also:
                            -   the value of Math.floor(x) == -Math.ceil(-x)
    Author:             christine@netscape.com
    Date:               7 july 1997
*/

    var SECTION = "15.8.2.9";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.floor(x)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Math.floor.length",                    1,              Math.floor.length );

    array[item++] = new TestCase( SECTION,  "Math.floor()",                         Number.NaN,     Math.floor() );
    array[item++] = new TestCase( SECTION,  "Math.floor(void 0)",                   Number.NaN,     Math.floor(void 0) );
    array[item++] = new TestCase( SECTION,  "Math.floor(null)",                     0,              Math.floor(null) );
    array[item++] = new TestCase( SECTION,  "Math.floor(true)",                     1,              Math.floor(true) );
    array[item++] = new TestCase( SECTION,  "Math.floor(false)",                    0,              Math.floor(false) );

    array[item++] = new TestCase( SECTION,  "Math.floor('1.1')",                    1,              Math.floor("1.1") );
    array[item++] = new TestCase( SECTION,  "Math.floor('-1.1')",                   -2,             Math.floor("-1.1") );
    array[item++] = new TestCase( SECTION,  "Math.floor('0.1')",                    0,              Math.floor("0.1") );
    array[item++] = new TestCase( SECTION,  "Math.floor('-0.1')",                   -1,             Math.floor("-0.1") );

    array[item++] = new TestCase( SECTION,  "Math.floor(NaN)",                      Number.NaN,     Math.floor(Number.NaN)  );
    array[item++] = new TestCase( SECTION,  "Math.floor(NaN)==-Math.ceil(-NaN)",    false,          Math.floor(Number.NaN) == -Math.ceil(-Number.NaN) );

    array[item++] = new TestCase( SECTION,  "Math.floor(0)",                        0,              Math.floor(0)           );
    array[item++] = new TestCase( SECTION,  "Math.floor(0)==-Math.ceil(-0)",        true,           Math.floor(0) == -Math.ceil(-0) );

    array[item++] = new TestCase( SECTION,  "Math.floor(-0)",           -0,                          Math.floor(-0)          );
    array[item++] = new TestCase( SECTION,  "Infinity/Math.floor(-0)",           -Infinity,         Infinity/Math.floor(-0)          );
    array[item++] = new TestCase( SECTION,  "Math.floor(-0)==-Math.ceil(0)",        true,           Math.floor(-0)== -Math.ceil(0) );

    array[item++] = new TestCase( SECTION,  "Math.floor(Infinity)",     Number.POSITIVE_INFINITY,   Math.floor(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.floor(Infinity)==-Math.ceil(-Infinity)",  true,   Math.floor(Number.POSITIVE_INFINITY) == -Math.ceil(Number.NEGATIVE_INFINITY) );

    array[item++] = new TestCase( SECTION,  "Math.floor(-Infinity)",    Number.NEGATIVE_INFINITY,   Math.floor(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.floor(-Infinity)==-Math.ceil(Infinity)",  true,   Math.floor(Number.NEGATIVE_INFINITY) == -Math.ceil(Number.POSITIVE_INFINITY) );

    array[item++] = new TestCase( SECTION,  "Math.floor(0.0000001)",    0,                          Math.floor(0.0000001) );
    array[item++] = new TestCase( SECTION,  "Math.floor(0.0000001)==-Math.ceil(0.0000001)", true,   Math.floor(0.0000001)==-Math.ceil(-0.0000001) );

    array[item++] = new TestCase( SECTION,  "Math.floor(-0.0000001)",   -1,                         Math.floor(-0.0000001) );
    array[item++] = new TestCase( SECTION,  "Math.floor(0.0000001)==-Math.ceil(0.0000001)",  true,  Math.floor(-0.0000001)==-Math.ceil(0.0000001) );

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