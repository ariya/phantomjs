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
    File Name:          15.8.2.17.js
    ECMA Section:       15.8.2.17  Math.sqrt(x)
    Description:        return an approximation to the squareroot of the argument.
                        special cases:
                        -   if x is NaN         return NaN
                        -   if x < 0            return NaN
                        -   if x == 0           return 0
                        -   if x == -0          return -0
                        -   if x == Infinity    return Infinity
    Author:             christine@netscape.com
    Date:               7 july 1997
*/

    var SECTION = "15.8.2.17";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.sqrt(x)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Math.sqrt.length",     1,              Math.sqrt.length );

    array[item++] = new TestCase( SECTION,  "Math.sqrt()",          Number.NaN,     Math.sqrt() );
    array[item++] = new TestCase( SECTION,  "Math.sqrt(void 0)",    Number.NaN,     Math.sqrt(void 0) );
    array[item++] = new TestCase( SECTION,  "Math.sqrt(null)",      0,              Math.sqrt(null) );
    array[item++] = new TestCase( SECTION,  "Math.sqrt(true)",      1,              Math.sqrt(1) );
    array[item++] = new TestCase( SECTION,  "Math.sqrt(false)",     0,              Math.sqrt(false) );
    array[item++] = new TestCase( SECTION,  "Math.sqrt('225')",     15,             Math.sqrt('225') );

    array[item++] = new TestCase( SECTION,  "Math.sqrt(NaN)",       Number.NaN,     Math.sqrt(Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.sqrt(-Infinity)", Number.NaN,     Math.sqrt(Number.NEGATIVE_INFINITY));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(-1)",        Number.NaN,     Math.sqrt(-1));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(-0.5)",      Number.NaN,     Math.sqrt(-0.5));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(0)",         0,              Math.sqrt(0));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(-0)",        -0,             Math.sqrt(-0));
    array[item++] = new TestCase( SECTION,  "Infinity/Math.sqrt(-0)",   -Infinity,  Infinity/Math.sqrt(-0) );
    array[item++] = new TestCase( SECTION,  "Math.sqrt(Infinity)",  Number.POSITIVE_INFINITY,   Math.sqrt(Number.POSITIVE_INFINITY));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(1)",         1,              Math.sqrt(1));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(2)",         Math.SQRT2,     Math.sqrt(2));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(0.5)",       Math.SQRT1_2,   Math.sqrt(0.5));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(4)",         2,              Math.sqrt(4));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(9)",         3,              Math.sqrt(9));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(16)",        4,              Math.sqrt(16));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(25)",        5,              Math.sqrt(25));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(36)",        6,              Math.sqrt(36));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(49)",        7,              Math.sqrt(49));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(64)",        8,              Math.sqrt(64));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(256)",       16,             Math.sqrt(256));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(10000)",     100,            Math.sqrt(10000));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(65536)",     256,            Math.sqrt(65536));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(0.09)",      0.3,            Math.sqrt(0.09));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(0.01)",      0.1,            Math.sqrt(0.01));
    array[item++] = new TestCase( SECTION,  "Math.sqrt(0.00000001)",0.0001,         Math.sqrt(0.00000001));

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