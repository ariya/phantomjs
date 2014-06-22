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
    File Name:          15.7.4.2.js
    ECMA Section:       15.7.4.2.1 Number.prototype.toString()
    Description:
    If the radix is the number 10 or not supplied, then this number value is
    given as an argument to the ToString operator; the resulting string value
    is returned.

    If the radix is supplied and is an integer from 2 to 36, but not 10, the
    result is a string, the choice of which is implementation dependent.

    The toString function is not generic; it generates a runtime error if its
    this value is not a Number object. Therefore it cannot be transferred to
    other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               16 september 1997
*/
    var SECTION = "15.7.4.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Number.prototype.toString()");
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

//  the following two lines cause navigator to crash -- cmb 9/16/97
    array[item++] = new TestCase(SECTION, "Number.prototype.toString()",       "0",        "Number.prototype.toString()" );
    array[item++] = new TestCase(SECTION, "typeof(Number.prototype.toString())", "string",      "typeof(Number.prototype.toString())" );

    array[item++] = new TestCase(SECTION,  "s = Number.prototype.toString; o = new Number(); o.toString = s; o.toString()",  "0",          "s = Number.prototype.toString; o = new Number(); o.toString = s; o.toString()" );
    array[item++] = new TestCase(SECTION,  "s = Number.prototype.toString; o = new Number(1); o.toString = s; o.toString()", "1",          "s = Number.prototype.toString; o = new Number(1); o.toString = s; o.toString()" );
    array[item++] = new TestCase(SECTION,  "s = Number.prototype.toString; o = new Number(-1); o.toString = s; o.toString()", "-1",         "s = Number.prototype.toString; o = new Number(-1); o.toString = s; o.toString()" );

    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(255); MYNUM.toString(10)",          "255",      "var MYNUM = new Number(255); MYNUM.toString(10)" );
    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(Number.NaN); MYNUM.toString(10)",   "NaN",      "var MYNUM = new Number(Number.NaN); MYNUM.toString(10)" );
    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(Infinity); MYNUM.toString(10)",   "Infinity",   "var MYNUM = new Number(Infinity); MYNUM.toString(10)" );
    array[item++] = new TestCase(SECTION, "var MYNUM = new Number(-Infinity); MYNUM.toString(10)",   "-Infinity", "var MYNUM = new Number(-Infinity); MYNUM.toString(10)" );

    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual= eval(testcases[tc].actual );

        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
