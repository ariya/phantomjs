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
    File Name:          9.3.js
    ECMA Section:       9.3  Type Conversion:  ToNumber
    Description:        rules for converting an argument to a number.
                        see 9.3.1 for cases for converting strings to numbers.
                        special cases:
                        undefined           NaN
                        Null                NaN
                        Boolean             1 if true; +0 if false
                        Number              the argument ( no conversion )
                        String              see test 9.3.1
                        Object              see test 9.3-1

                        For ToNumber applied to the String type, see test 9.3.1.
                        For ToNumber applied to the object type, see test 9.3-1.

    Author:             christine@netscape.com
    Date:               10 july 1997

*/
    var SECTION = "9.3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "ToNumber";

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
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
function getTestCases() {
    var array = new Array();
    var item = 0;

    // special cases here

    array[item++] = new TestCase( SECTION,   "Number()",                      0,              Number() );
    array[item++] = new TestCase( SECTION,   "Number(eval('var x'))",         Number.NaN,     Number(eval("var x")) );
    array[item++] = new TestCase( SECTION,   "Number(void 0)",                Number.NaN,     Number(void 0) );
    array[item++] = new TestCase( SECTION,   "Number(null)",                  0,              Number(null) );
    array[item++] = new TestCase( SECTION,   "Number(true)",                  1,              Number(true) );
    array[item++] = new TestCase( SECTION,   "Number(false)",                 0,              Number(false) );
    array[item++] = new TestCase( SECTION,   "Number(0)",                     0,              Number(0) );
    array[item++] = new TestCase( SECTION,   "Number(-0)",                    -0,             Number(-0) );
    array[item++] = new TestCase( SECTION,   "Number(1)",                     1,              Number(1) );
    array[item++] = new TestCase( SECTION,   "Number(-1)",                    -1,             Number(-1) );
    array[item++] = new TestCase( SECTION,   "Number(Number.MAX_VALUE)",      1.7976931348623157e308, Number(Number.MAX_VALUE) );
    array[item++] = new TestCase( SECTION,   "Number(Number.MIN_VALUE)",      5e-324,         Number(Number.MIN_VALUE) );

    array[item++] = new TestCase( SECTION,   "Number(Number.NaN)",                Number.NaN,                 Number(Number.NaN) );
    array[item++] = new TestCase( SECTION,   "Number(Number.POSITIVE_INFINITY)",  Number.POSITIVE_INFINITY,   Number(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Number(Number.NEGATIVE_INFINITY)",  Number.NEGATIVE_INFINITY,   Number(Number.NEGATIVE_INFINITY) );

    return ( array );
}
