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
    File Name:          15.8.2.13.js
    ECMA Section:       15.8.2.13 Math.pow(x, y)
    Description:        return an approximation to the result of x
                        to the power of y.  there are many special cases;
                        refer to the spec.
    Author:             christine@netscape.com
    Date:               9 july 1997
*/

    var SECTION = "15.8.2.13";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.pow(x, y)";
    var BUGNUMBER="77141";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();
function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Math.pow.length",                  2,                          Math.pow.length );

    array[item++] = new TestCase( SECTION,  "Math.pow()",                       Number.NaN,                 Math.pow() );
    array[item++] = new TestCase( SECTION,  "Math.pow(null, null)",             1,                          Math.pow(null,null) );
    array[item++] = new TestCase( SECTION,  "Math.pow(void 0, void 0)",         Number.NaN,                 Math.pow(void 0, void 0));
    array[item++] = new TestCase( SECTION,  "Math.pow(true, false)",            1,                          Math.pow(true, false) );
    array[item++] = new TestCase( SECTION,  "Math.pow(false,true)",             0,                          Math.pow(false,true) );
    array[item++] = new TestCase( SECTION,  "Math.pow('2','32')",               4294967296,                 Math.pow('2','32') );

    array[item++] = new TestCase( SECTION,  "Math.pow(1,NaN)",                  Number.NaN,                 Math.pow(1,Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0,NaN)",	                Number.NaN,                 Math.pow(0,Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.pow(NaN,0)",                  1,                          Math.pow(Number.NaN,0) );
    array[item++] = new TestCase( SECTION,  "Math.pow(NaN,-0)",                 1,                          Math.pow(Number.NaN,-0) );
    array[item++] = new TestCase( SECTION,  "Math.pow(NaN,1)",                  Number.NaN,                 Math.pow(Number.NaN, 1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(NaN,.5)",                 Number.NaN,                 Math.pow(Number.NaN, .5) );
    array[item++] = new TestCase( SECTION,  "Math.pow(1.00000001, Infinity)",   Number.POSITIVE_INFINITY,   Math.pow(1.00000001, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(1.00000001, -Infinity)",  0,                          Math.pow(1.00000001, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1.00000001, Infinity)",  Number.POSITIVE_INFINITY,   Math.pow(-1.00000001,Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1.00000001, -Infinity)", 0,                          Math.pow(-1.00000001,Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(1, Infinity)",            Number.NaN,                 Math.pow(1, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(1, -Infinity)",           Number.NaN,                 Math.pow(1, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1, Infinity)",           Number.NaN,                 Math.pow(-1, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1, -Infinity)",          Number.NaN,                 Math.pow(-1, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(.0000000009, Infinity)",  0,                          Math.pow(.0000000009, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-.0000000009, Infinity)", 0,                          Math.pow(-.0000000009, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(.0000000009, -Infinity)", Number.POSITIVE_INFINITY,   Math.pow(-.0000000009, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(Infinity, .00000000001)", Number.POSITIVE_INFINITY,   Math.pow(Number.POSITIVE_INFINITY,.00000000001) );
    array[item++] = new TestCase( SECTION,  "Math.pow(Infinity, 1)",            Number.POSITIVE_INFINITY,   Math.pow(Number.POSITIVE_INFINITY, 1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(Infinity, -.00000000001)",0,                          Math.pow(Number.POSITIVE_INFINITY, -.00000000001) );
    array[item++] = new TestCase( SECTION,  "Math.pow(Infinity, -1)",           0,                          Math.pow(Number.POSITIVE_INFINITY, -1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, 1)",           Number.NEGATIVE_INFINITY,   Math.pow(Number.NEGATIVE_INFINITY, 1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, 333)",         Number.NEGATIVE_INFINITY,   Math.pow(Number.NEGATIVE_INFINITY, 333) );
    array[item++] = new TestCase( SECTION,  "Math.pow(Infinity, 2)",            Number.POSITIVE_INFINITY,   Math.pow(Number.POSITIVE_INFINITY, 2) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, 666)",         Number.POSITIVE_INFINITY,   Math.pow(Number.NEGATIVE_INFINITY, 666) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, 0.5)",         Number.POSITIVE_INFINITY,   Math.pow(Number.NEGATIVE_INFINITY, 0.5) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, Infinity)",    Number.POSITIVE_INFINITY,   Math.pow(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY) );

    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, -1)",          -0,                         Math.pow(Number.NEGATIVE_INFINITY, -1) );
    array[item++] = new TestCase( SECTION,  "Infinity/Math.pow(-Infinity, -1)", -Infinity,                  Infinity/Math.pow(Number.NEGATIVE_INFINITY, -1) );

    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, -3)",          -0,                         Math.pow(Number.NEGATIVE_INFINITY, -3) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, -2)",          0,                          Math.pow(Number.NEGATIVE_INFINITY, -2) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, -0.5)",        0,                          Math.pow(Number.NEGATIVE_INFINITY,-0.5) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-Infinity, -Infinity)",   0,                          Math.pow(Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, 1)",                   0,                          Math.pow(0,1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, 0)",                   1,                          Math.pow(0,0) );
    array[item++] = new TestCase( SECTION,  "Math.pow(1, 0)",                   1,                          Math.pow(1,0) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1, 0)",                  1,                          Math.pow(-1,0) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, 0.5)",                 0,                          Math.pow(0,0.5) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, 1000)",                0,                          Math.pow(0,1000) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, Infinity)",            0,                          Math.pow(0, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, -1)",                  Number.POSITIVE_INFINITY,   Math.pow(0, -1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, -0.5)",                Number.POSITIVE_INFINITY,   Math.pow(0, -0.5) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, -1000)",               Number.POSITIVE_INFINITY,   Math.pow(0, -1000) );
    array[item++] = new TestCase( SECTION,  "Math.pow(0, -Infinity)",           Number.POSITIVE_INFINITY,   Math.pow(0, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, 1)",                  -0,                         Math.pow(-0, 1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, 3)",                  -0,                         Math.pow(-0,3) );

    array[item++] = new TestCase( SECTION,  "Infinity/Math.pow(-0, 1)",         -Infinity,                  Infinity/Math.pow(-0, 1) );
    array[item++] = new TestCase( SECTION,  "Infinity/Math.pow(-0, 3)",         -Infinity,                  Infinity/Math.pow(-0,3) );

    array[item++] = new TestCase( SECTION,  "Math.pow(-0, 2)",                  0,                          Math.pow(-0,2) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, Infinity)",           0,                          Math.pow(-0, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, -1)",                 Number.NEGATIVE_INFINITY,   Math.pow(-0, -1) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, -10001)",             Number.NEGATIVE_INFINITY,   Math.pow(-0, -10001) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, -2)",                 Number.POSITIVE_INFINITY,   Math.pow(-0, -2) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, 0.5)",                0,                          Math.pow(-0, 0.5) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-0, Infinity)",           0,                          Math.pow(-0, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1, 0.5)",                Number.NaN,                 Math.pow(-1, 0.5) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1, NaN)",                Number.NaN,                 Math.pow(-1, Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.pow(-1, -0.5)",               Number.NaN,                 Math.pow(-1, -0.5) );

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
