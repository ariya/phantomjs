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
    File Name:          9.4-1.js
    ECMA Section:       9.4 ToInteger
    Description:        1.  Call ToNumber on the input argument
                        2.  If Result(1) is NaN, return +0
                        3.  If Result(1) is +0, -0, Infinity, or -Infinity,
                            return Result(1).
                        4.  Compute sign(Result(1)) * floor(abs(Result(1))).
                        5.  Return Result(4).

                        To test ToInteger, this test uses new Date(value),
                        15.9.3.7.  The Date constructor sets the [[Value]]
                        property of the new object to TimeClip(value), which
                        uses the rules:

                        TimeClip(time)
                        1. If time is not finite, return NaN
                        2. If abs(Result(1)) > 8.64e15, return NaN
                        3. Return an implementation dependent choice of either
                        ToInteger(Result(2)) or ToInteger(Result(2)) + (+0)
                        (Adding a positive 0 converts -0 to +0).

                        This tests ToInteger for values -8.64e15 > value > 8.64e15,
                        not including -0 and +0.

                        For additional special cases (0, +0, Infinity, -Infinity,
                        and NaN, see 9.4-2.js).  For value is String, see 9.4-3.js.

    Author:             christine@netscape.com
    Date:               10 july 1997

*/
    var SECTION = "9.4-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "ToInteger";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );


            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
function getTestCases() {
    var array = new Array();
    var item = 0;

    // some special cases

    array[item++] = new TestCase( SECTION,  "td = new Date(Number.NaN); td.valueOf()",  Number.NaN, eval("td = new Date(Number.NaN); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(Infinity); td.valueOf()",    Number.NaN, eval("td = new Date(Number.POSITIVE_INFINITY); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(-Infinity); td.valueOf()",   Number.NaN, eval("td = new Date(Number.NEGATIVE_INFINITY); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(-0); td.valueOf()",          -0,         eval("td = new Date(-0); td.valueOf()" ) );
    array[item++] = new TestCase( SECTION,  "td = new Date(0); td.valueOf()",           0,          eval("td = new Date(0); td.valueOf()") );

    // value is not an integer

    array[item++] = new TestCase( SECTION,  "td = new Date(3.14159); td.valueOf()",     3,          eval("td = new Date(3.14159); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(Math.PI); td.valueOf()",     3,          eval("td = new Date(Math.PI); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(-Math.PI);td.valueOf()",     -3,         eval("td = new Date(-Math.PI);td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(3.14159e2); td.valueOf()",   314,        eval("td = new Date(3.14159e2); td.valueOf()") );

    array[item++] = new TestCase( SECTION,  "td = new Date(.692147e1); td.valueOf()",   6,          eval("td = new Date(.692147e1);td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(-.692147e1);td.valueOf()",   -6,         eval("td = new Date(-.692147e1);td.valueOf()") );

    // value is not a number

    array[item++] = new TestCase( SECTION,  "td = new Date(true); td.valueOf()",        1,          eval("td = new Date(true); td.valueOf()" ) );
    array[item++] = new TestCase( SECTION,  "td = new Date(false); td.valueOf()",       0,          eval("td = new Date(false); td.valueOf()") );

    array[item++] = new TestCase( SECTION,  "td = new Date(new Number(Math.PI)); td.valueOf()",  3, eval("td = new Date(new Number(Math.PI)); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(new Number(Math.PI)); td.valueOf()",  3, eval("td = new Date(new Number(Math.PI)); td.valueOf()") );

    // edge cases
    array[item++] = new TestCase( SECTION,  "td = new Date(8.64e15); td.valueOf()",     8.64e15,    eval("td = new Date(8.64e15); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(-8.64e15); td.valueOf()",    -8.64e15,   eval("td = new Date(-8.64e15); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(8.64e-15); td.valueOf()",    0,          eval("td = new Date(8.64e-15); td.valueOf()") );
    array[item++] = new TestCase( SECTION,  "td = new Date(-8.64e-15); td.valueOf()",   0,          eval("td = new Date(-8.64e-15); td.valueOf()") );

    return ( array );
}
