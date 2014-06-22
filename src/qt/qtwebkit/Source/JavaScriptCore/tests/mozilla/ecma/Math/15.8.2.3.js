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
    File Name:          15.8.2.3.js
    ECMA Section:       15.8.2.3 asin( x )
    Description:        return an approximation to the arc sine of the
                        argument.  the result is expressed in radians and
                        range is from -PI/2 to +PI/2.  special cases:
                        - if x is NaN,  the result is NaN
                        - if x > 1,     the result is NaN
                        - if x < -1,    the result is NaN
                        - if x == +0,   the result is +0
                        - if x == -0,   the result is -0
    Author:             christine@netscape.com
    Date:               7 july 1997

*/
    var SECTION = "15.8.2.3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.asin()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "Math.asin()",           Number.NaN,     Math.asin() );
    array[item++] = new TestCase( SECTION, "Math.asin(void 0)",     Number.NaN,     Math.asin(void 0) );
    array[item++] = new TestCase( SECTION, "Math.asin(null)",       0,              Math.asin(null) );
    array[item++] = new TestCase( SECTION, "Math.asin(NaN)",        Number.NaN,     Math.asin(Number.NaN)   );

    array[item++] = new TestCase( SECTION, "Math.asin('string')",   Number.NaN,     Math.asin("string")     );
    array[item++] = new TestCase( SECTION, "Math.asin('0')",        0,              Math.asin("0") );
    array[item++] = new TestCase( SECTION, "Math.asin('1')",        Math.PI/2,      Math.asin("1") );
    array[item++] = new TestCase( SECTION, "Math.asin('-1')",       -Math.PI/2,     Math.asin("-1") );
    array[item++] = new TestCase( SECTION, "Math.asin(Math.SQRT1_2+'')",    Math.PI/4,  Math.asin(Math.SQRT1_2+'') );
    array[item++] = new TestCase( SECTION, "Math.asin(-Math.SQRT1_2+'')",   -Math.PI/4, Math.asin(-Math.SQRT1_2+'') );

    array[item++] = new TestCase( SECTION, "Math.asin(1.000001)",    Number.NaN,    Math.asin(1.000001)     );
    array[item++] = new TestCase( SECTION, "Math.asin(-1.000001)",   Number.NaN,    Math.asin(-1.000001)    );
    array[item++] = new TestCase( SECTION, "Math.asin(0)",           0,             Math.asin(0)            );
    array[item++] = new TestCase( SECTION, "Math.asin(-0)",          -0,            Math.asin(-0)           );

    array[item++] = new TestCase( SECTION, "Infinity/Math.asin(-0)",    -Infinity,  Infinity/Math.asin(-0) );

    array[item++] = new TestCase( SECTION, "Math.asin(1)",              Math.PI/2,  Math.asin(1)            );
    array[item++] = new TestCase( SECTION, "Math.asin(-1)",             -Math.PI/2, Math.asin(-1)            );
    array[item++] = new TestCase( SECTION, "Math.asin(Math.SQRT1_2))",  Math.PI/4,  Math.asin(Math.SQRT1_2) );
    array[item++] = new TestCase( SECTION, "Math.asin(-Math.SQRT1_2))", -Math.PI/4, Math.asin(-Math.SQRT1_2));

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