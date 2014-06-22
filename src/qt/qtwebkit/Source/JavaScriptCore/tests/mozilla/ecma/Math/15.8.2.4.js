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
    File Name:          15.8.2.4.js
    ECMA Section:       15.8.2.4 atan( x )
    Description:        return an approximation to the arc tangent of the
                        argument.  the result is expressed in radians and
                        range is from -PI/2 to +PI/2.  special cases:
                        - if x is NaN,  the result is NaN
                        - if x == +0,   the result is +0
                        - if x == -0,   the result is -0
                        - if x == +Infinity,    the result is approximately +PI/2
                        - if x == -Infinity,    the result is approximately -PI/2
    Author:             christine@netscape.com
    Date:               7 july 1997

*/

    var SECTION = "15.8.2.4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.atan()";
    var BUGNUMBER="77391";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "Math.atan.length",        1,              Math.atan.length );

    array[item++] = new TestCase( SECTION,   "Math.atan()",             Number.NaN,     Math.atan() );
    array[item++] = new TestCase( SECTION,   "Math.atan(void 0)",       Number.NaN,     Math.atan(void 0) );
    array[item++] = new TestCase( SECTION,   "Math.atan(null)",         0,              Math.atan(null) );
    array[item++] = new TestCase( SECTION,   "Math.atan(NaN)",          Number.NaN,     Math.atan(Number.NaN) );

    array[item++] = new TestCase( SECTION,   "Math.atan('a string')",   Number.NaN,     Math.atan("a string") );
    array[item++] = new TestCase( SECTION,   "Math.atan('0')",          0,              Math.atan('0') );
    array[item++] = new TestCase( SECTION,   "Math.atan('1')",          Math.PI/4,      Math.atan('1') );
    array[item++] = new TestCase( SECTION,   "Math.atan('-1')",         -Math.PI/4,     Math.atan('-1') );
    array[item++] = new TestCase( SECTION,   "Math.atan('Infinity)",    Math.PI/2,      Math.atan('Infinity') );
    array[item++] = new TestCase( SECTION,   "Math.atan('-Infinity)",   -Math.PI/2,     Math.atan('-Infinity') );

    array[item++] = new TestCase( SECTION,   "Math.atan(0)",            0,              Math.atan(0)          );
    array[item++] = new TestCase( SECTION,   "Math.atan(-0)",	        -0,             Math.atan(-0)         );
    array[item++] = new TestCase( SECTION,   "Infinity/Math.atan(-0)",  -Infinity,      Infinity/Math.atan(-0) );
    array[item++] = new TestCase( SECTION,   "Math.atan(Infinity)",     Math.PI/2,      Math.atan(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan(-Infinity)",    -Math.PI/2,     Math.atan(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan(1)",     	    Math.PI/4,      Math.atan(1)          );
    array[item++] = new TestCase( SECTION,   "Math.atan(-1)",           -Math.PI/4,     Math.atan(-1)         );
    return array;
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