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
    File Name:          15.8.2.12.js
    ECMA Section:       15.8.2.12 Math.min(x, y)
    Description:        return the smaller of the two arguments.
                        special cases:
                            - if x is NaN or y is NaN   return NaN
                            - if x < y                  return x
                            - if y > x                  return y
                            - if x is +0 and y is +0    return +0
                            - if x is +0 and y is -0    return -0
                            - if x is -0 and y is +0    return -0
                            - if x is -0 and y is -0    return -0
    Author:             christine@netscape.com
    Date:               7 july 1997
*/


    var SECTION = "15.8.2.12";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.min(x, y)";
    var BUGNUMBER="76439";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Math.min.length",              2,              Math.min.length );

    array[item++] = new TestCase( SECTION,  "Math.min()",                   Infinity,       Math.min() );
    array[item++] = new TestCase( SECTION,  "Math.min(void 0, 1)",          Number.NaN,     Math.min( void 0, 1 ) );
    array[item++] = new TestCase( SECTION,  "Math.min(void 0, void 0)",     Number.NaN,     Math.min( void 0, void 0 ) );
    array[item++] = new TestCase( SECTION,  "Math.min(null, 1)",            0,              Math.min( null, 1 ) );
    array[item++] = new TestCase( SECTION,  "Math.min(-1, null)",           -1,              Math.min( -1, null ) );
    array[item++] = new TestCase( SECTION,  "Math.min(true, false)",        0,              Math.min(true,false) );

    array[item++] = new TestCase( SECTION,  "Math.min('-99','99')",         -99,             Math.min( "-99","99") );

    array[item++] = new TestCase( SECTION,  "Math.min(NaN,0)",      Number.NaN, Math.min(Number.NaN,0) );
    array[item++] = new TestCase( SECTION,  "Math.min(NaN,1)",      Number.NaN, Math.min(Number.NaN,1) );
    array[item++] = new TestCase( SECTION,  "Math.min(NaN,-1)",     Number.NaN, Math.min(Number.NaN,-1) );
    array[item++] = new TestCase( SECTION,  "Math.min(0,NaN)",      Number.NaN, Math.min(0,Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.min(1,NaN)",      Number.NaN, Math.min(1,Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.min(-1,NaN)",     Number.NaN, Math.min(-1,Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.min(NaN,NaN)",    Number.NaN, Math.min(Number.NaN,Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.min(1,1.0000000001)", 1,      Math.min(1,1.0000000001) );
    array[item++] = new TestCase( SECTION,  "Math.min(1.0000000001,1)", 1,      Math.min(1.0000000001,1) );
    array[item++] = new TestCase( SECTION,  "Math.min(0,0)",        0,          Math.min(0,0) );
    array[item++] = new TestCase( SECTION,  "Math.min(0,-0)",       -0,         Math.min(0,-0) );
    array[item++] = new TestCase( SECTION,  "Math.min(-0,-0)",      -0,         Math.min(-0,-0) );

    array[item++] = new TestCase( SECTION,  "Infinity/Math.min(0,-0)",       -Infinity,         Infinity/Math.min(0,-0) );
    array[item++] = new TestCase( SECTION,  "Infinity/Math.min(-0,-0)",      -Infinity,         Infinity/Math.min(-0,-0) );


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
