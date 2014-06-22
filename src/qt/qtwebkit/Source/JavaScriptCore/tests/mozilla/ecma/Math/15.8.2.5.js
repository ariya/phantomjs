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
    File Name:          15.8.2.5.js
    ECMA Section:       15.8.2.5 atan2( y, x )
    Description:

    Author:             christine@netscape.com
    Date:               7 july 1997

*/
    var SECTION = "15.8.2.5";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.atan2(x,y)";
    var BUGNUMBER="76111";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "Math.atan2.length",       2,              Math.atan2.length );

    array[item++] = new TestCase( SECTION,   "Math.atan2(NaN, 0)",      Number.NaN,     Math.atan2(Number.NaN,0) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(null, null)",  0,              Math.atan2(null, null) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(void 0, void 0)",       Number.NaN,     Math.atan2(void 0, void 0) );
    array[item++] = new TestCase( SECTION,   "Math.atan2()",       Number.NaN,                  Math.atan2() );

    array[item++] = new TestCase( SECTION,   "Math.atan2(0, NaN)",       Number.NaN,     Math.atan2(0,Number.NaN) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(1, 0)",         Math.PI/2,      Math.atan2(1,0)          );
    array[item++] = new TestCase( SECTION,   "Math.atan2(1,-0)",         Math.PI/2,      Math.atan2(1,-0)         );
    array[item++] = new TestCase( SECTION,   "Math.atan2(0,0.001)",      0,              Math.atan2(0,0.001)      );
    array[item++] = new TestCase( SECTION,   "Math.atan2(0,0)",          0,              Math.atan2(0,0)          );
    array[item++] = new TestCase( SECTION,   "Math.atan2(0, -0)",        Math.PI,        Math.atan2(0,-0)         );
    array[item++] = new TestCase( SECTION,   "Math.atan2(0, -1)",        Math.PI,        Math.atan2(0, -1)        );

    array[item++] = new TestCase( SECTION,   "Math.atan2(-0, 1)",        -0,             Math.atan2(-0, 1)        );
    array[item++] = new TestCase( SECTION,   "Infinity/Math.atan2(-0, 1)", -Infinity,   Infinity/Math.atan2(-0,1) );

    array[item++] = new TestCase( SECTION,   "Math.atan2(-0,	0)",    	-0,             Math.atan2(-0,0)         );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-0,	-0)",   	-Math.PI,       Math.atan2(-0, -0)       );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-0,	-1)",   	-Math.PI,       Math.atan2(-0, -1)       );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-1,	0)",    	-Math.PI/2,     Math.atan2(-1, 0)        );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-1,	-0)",   	-Math.PI/2,     Math.atan2(-1, -0)       );
    array[item++] = new TestCase( SECTION,   "Math.atan2(1, Infinity)",  0,              Math.atan2(1, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(1,-Infinity)",  Math.PI,    	Math.atan2(1, Number.NEGATIVE_INFINITY) );

    array[item++] = new TestCase( SECTION,   "Math.atan2(-1, Infinity)", -0,         	Math.atan2(-1,Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Infinity/Math.atan2(-1, Infinity)",   -Infinity,  Infinity/Math.atan2(-1,Infinity) );

    array[item++] = new TestCase( SECTION,   "Math.atan2(-1,-Infinity)", -Math.PI,       Math.atan2(-1,Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(Infinity, 0)",  Math.PI/2,      Math.atan2(Number.POSITIVE_INFINITY, 0) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(Infinity, 1)",  Math.PI/2,      Math.atan2(Number.POSITIVE_INFINITY, 1) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(Infinity,-1)",  Math.PI/2,      Math.atan2(Number.POSITIVE_INFINITY,-1) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(Infinity,-0)",  Math.PI/2,      Math.atan2(Number.POSITIVE_INFINITY,-0) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-Infinity, 0)", -Math.PI/2,     Math.atan2(Number.NEGATIVE_INFINITY, 0) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-Infinity,-0)", -Math.PI/2,     Math.atan2(Number.NEGATIVE_INFINITY,-0) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-Infinity, 1)", -Math.PI/2,     Math.atan2(Number.NEGATIVE_INFINITY, 1) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-Infinity, -1)", -Math.PI/2,    Math.atan2(Number.NEGATIVE_INFINITY,-1) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(Infinity, Infinity)",   Math.PI/4,      Math.atan2(Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(Infinity, -Infinity)",  3*Math.PI/4,    Math.atan2(Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-Infinity, Infinity)",  -Math.PI/4,     Math.atan2(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-Infinity, -Infinity)", -3*Math.PI/4,   Math.atan2(Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.atan2(-1, 1)",        -Math.PI/4,     Math.atan2( -1, 1) );

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
