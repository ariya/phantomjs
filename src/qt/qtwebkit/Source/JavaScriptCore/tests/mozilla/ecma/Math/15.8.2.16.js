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
    File Name:          15.8.2.16.js
    ECMA Section:       15.8.2.16 sin( x )
    Description:        return an approximation to the sine of the
                        argument.  argument is expressed in radians
    Author:             christine@netscape.com
    Date:               7 july 1997

*/
    var SECTION = "15.8.2.16";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.sin(x)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Math.sin.length",      1,              Math.sin.length );

    array[item++] = new TestCase( SECTION,  "Math.sin()",           Number.NaN,     Math.sin() );
    array[item++] = new TestCase( SECTION,  "Math.sin(null)",       0,              Math.sin(null) );
    array[item++] = new TestCase( SECTION,  "Math.sin(void 0)",     Number.NaN,     Math.sin(void 0) );
    array[item++] = new TestCase( SECTION,  "Math.sin(false)",      0,              Math.sin(false) );
    array[item++] = new TestCase( SECTION,  "Math.sin('2.356194490192')",    0.7071067811865,    Math.sin('2.356194490192') );

    array[item++] = new TestCase( SECTION,  "Math.sin(NaN)",        Number.NaN,     Math.sin(Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.sin(0)",          0,              Math.sin(0) );
    array[item++] = new TestCase( SECTION,  "Math.sin(-0)",         -0,             Math.sin(-0));
    array[item++] = new TestCase( SECTION,  "Math.sin(Infinity)",   Number.NaN,     Math.sin(Number.POSITIVE_INFINITY));
    array[item++] = new TestCase( SECTION,  "Math.sin(-Infinity)",  Number.NaN,	    Math.sin(Number.NEGATIVE_INFINITY));
    array[item++] = new TestCase( SECTION,  "Math.sin(0.7853981633974)",	0.7071067811865,    Math.sin(0.7853981633974));
    array[item++] = new TestCase( SECTION,  "Math.sin(1.570796326795)",	    1,                  Math.sin(1.570796326795));
    array[item++] = new TestCase( SECTION,  "Math.sin(2.356194490192)",	    0.7071067811865,    Math.sin(2.356194490192));
    array[item++] = new TestCase( SECTION,  "Math.sin(3.14159265359)",	    0,                  Math.sin(3.14159265359));

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