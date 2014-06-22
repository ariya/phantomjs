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
    File Name:          9.3.1-2.js
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

                        This tests special cases of ToNumber(string) that are
                        not covered in 9.3.1-1.js.

    Author:             christine@netscape.com
    Date:               10 july 1997

*/
    var SECTION = "9.3.1-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "ToNumber applied to the String type";

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

    // A StringNumericLiteral may not use octal notation

    array[item++] = new TestCase( SECTION,  "Number(00)",        0,         Number("00"));
    array[item++] = new TestCase( SECTION,  "Number(01)",        1,         Number("01"));
    array[item++] = new TestCase( SECTION,  "Number(02)",        2,         Number("02"));
    array[item++] = new TestCase( SECTION,  "Number(03)",        3,         Number("03"));
    array[item++] = new TestCase( SECTION,  "Number(04)",        4,         Number("04"));
    array[item++] = new TestCase( SECTION,  "Number(05)",        5,         Number("05"));
    array[item++] = new TestCase( SECTION,  "Number(06)",        6,         Number("06"));
    array[item++] = new TestCase( SECTION,  "Number(07)",        7,         Number("07"));
    array[item++] = new TestCase( SECTION,  "Number(010)",       10,        Number("010"));
    array[item++] = new TestCase( SECTION,  "Number(011)",       11,        Number("011"));

    // A StringNumericLIteral may have any number of leading 0 digits

    array[item++] = new TestCase( SECTION,  "Number(001)",        1,         Number("001"));
    array[item++] = new TestCase( SECTION,  "Number(0001)",       1,         Number("0001"));

    return ( array );
}
