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
    File Name:          9.3-1.js
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


                        This tests ToNumber applied to the object type, except
                        if object is string.  See 9.3-2 for
                        ToNumber( String object).

    Author:             christine@netscape.com
    Date:               10 july 1997

*/
    var SECTION = "9.3-1";
    var VERSION = "ECMA_1";
    startTest();
    var TYPE = "number";
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " ToNumber");
    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

                    testcases[tc].passed = writeTestCaseResult(
                                TYPE,
                                typeof(testcases[tc].actual),
                                "typeof( " + testcases[tc].description +
                                " ) = " + typeof(testcases[tc].actual) )
                                ? testcases[tc].passed
                                : false;

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
function getTestCases() {
    var array = new Array();
    var item = 0;

    // object is Number
    array[item++] = new TestCase( SECTION,   "Number(new Number())",          0,              Number(new Number())  );
    array[item++] = new TestCase( SECTION,   "Number(new Number(Number.NaN))",Number.NaN,     Number(new Number(Number.NaN)) );
    array[item++] = new TestCase( SECTION,   "Number(new Number(0))",         0,              Number(new Number(0)) );
    array[item++] = new TestCase( SECTION,   "Number(new Number(null))",      0,              Number(new Number(null)) );
//    array[item++] = new TestCase( SECTION,   "Number(new Number(void 0))",    Number.NaN,     Number(new Number(void 0)) );
    array[item++] = new TestCase( SECTION,   "Number(new Number(true))",      1,              Number(new Number(true)) );
    array[item++] = new TestCase( SECTION,   "Number(new Number(false))",     0,              Number(new Number(false)) );

    // object is boolean

    array[item++] = new TestCase( SECTION,   "Number(new Boolean(true))",     1,  Number(new Boolean(true)) );
    array[item++] = new TestCase( SECTION,   "Number(new Boolean(false))",    0,  Number(new Boolean(false)) );

    // object is array
    array[item++] = new TestCase( SECTION,   "Number(new Array(2,4,8,16,32))",      Number.NaN,     Number(new Array(2,4,8,16,32)) );

    return ( array );
}
