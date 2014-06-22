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
    File Name:          15.7.1.js
    ECMA Section:       15.7.1 The Number Constructor Called as a Function
                        15.7.1.1
                        15.7.1.2

    Description:        When Number is called as a function rather than as a
                        constructor, it performs a type conversion.
                        15.7.1.1    Return a number value (not a Number object)
                                    computed by ToNumber( value )
                        15.7.1.2    Number() returns 0.

                        need to add more test cases.  see the testcases for
                        TypeConversion ToNumber.

    Author:             christine@netscape.com
    Date:               29 september 1997
*/

    var SECTION = "15.7.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Number Constructor Called as a Function";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase(SECTION, "Number()",                  0,          Number() );
    array[item++] = new TestCase(SECTION, "Number(void 0)",            Number.NaN,  Number(void 0) );
    array[item++] = new TestCase(SECTION, "Number(null)",              0,          Number(null) );
    array[item++] = new TestCase(SECTION, "Number()",                  0,          Number() );
    array[item++] = new TestCase(SECTION, "Number(new Number())",      0,          Number( new Number() ) );
    array[item++] = new TestCase(SECTION, "Number(0)",                 0,          Number(0) );
    array[item++] = new TestCase(SECTION, "Number(1)",                 1,          Number(1) );
    array[item++] = new TestCase(SECTION, "Number(-1)",                -1,         Number(-1) );
    array[item++] = new TestCase(SECTION, "Number(NaN)",               Number.NaN, Number( Number.NaN ) );
    array[item++] = new TestCase(SECTION, "Number('string')",          Number.NaN, Number( "string") );
    array[item++] = new TestCase(SECTION, "Number(new String())",      0,          Number( new String() ) );
    array[item++] = new TestCase(SECTION, "Number('')",                0,          Number( "" ) );
    array[item++] = new TestCase(SECTION, "Number(Infinity)",          Number.POSITIVE_INFINITY,   Number("Infinity") );

    array[item++] = new TestCase(SECTION, "Number(new MyObject(100))",  100,        Number(new MyObject(100)) );

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
function MyObject( value ) {
    this.value = value;
    this.valueOf = new Function( "return this.value" );
}