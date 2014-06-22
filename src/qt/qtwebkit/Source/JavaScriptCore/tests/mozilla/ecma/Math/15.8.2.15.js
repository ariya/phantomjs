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
    File Name:          15.8.2.15.js
    ECMA Section:       15.8.2.15  Math.round(x)
    Description:        return the greatest number value that is closest to the
                        argument and is an integer.  if two integers are equally
                        close to the argument. then the result is the number value
                        that is closer to Infinity.  if the argument is an integer,
                        return the argument.
                        special cases:
                            - if x is NaN       return NaN
                            - if x = +0         return +0
                            - if x = -0          return -0
                            - if x = Infinity   return Infinity
                            - if x = -Infinity  return -Infinity
                            - if 0 < x < 0.5    return 0
                            - if -0.5 <= x < 0  return -0
                        example:
                            Math.round( 3.5 ) == 4
                            Math.round( -3.5 ) == 3
                        also:
                            - Math.round(x) == Math.floor( x + 0.5 )
                              except if x = -0.  in that case, Math.round(x) = -0

                              and Math.floor( x+0.5 ) = +0


    Author:             christine@netscape.com
    Date:               7 july 1997
*/

    var SECTION = "15.8.2.15";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.round(x)";
    var BUGNUMBER="331411";

    var EXCLUDE = "true";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Math.round.length",   1,               Math.round.length );

    array[item++] = new TestCase( SECTION,  "Math.round()",         Number.NaN,     Math.round() );
    array[item++] = new TestCase( SECTION,  "Math.round(null)",     0,              Math.round(0) );
    array[item++] = new TestCase( SECTION,  "Math.round(void 0)",   Number.NaN,     Math.round(void 0) );
    array[item++] = new TestCase( SECTION,  "Math.round(true)",     1,              Math.round(true) );
    array[item++] = new TestCase( SECTION,  "Math.round(false)",    0,              Math.round(false) );
    array[item++] = new TestCase( SECTION,  "Math.round('.99999')",  1,              Math.round('.99999') );
    array[item++] = new TestCase( SECTION,  "Math.round('12345e-2')",  123,          Math.round('12345e-2') );

    array[item++] = new TestCase( SECTION,  "Math.round(NaN)",      Number.NaN,     Math.round(Number.NaN) );
    array[item++] = new TestCase( SECTION,  "Math.round(0)",        0,              Math.round(0) );
    array[item++] = new TestCase( SECTION,  "Math.round(-0)",        -0,            Math.round(-0));
    array[item++] = new TestCase( SECTION,  "Infinity/Math.round(-0)",  -Infinity,  Infinity/Math.round(-0) );

    array[item++] = new TestCase( SECTION,  "Math.round(Infinity)", Number.POSITIVE_INFINITY,   Math.round(Number.POSITIVE_INFINITY));
    array[item++] = new TestCase( SECTION,  "Math.round(-Infinity)",Number.NEGATIVE_INFINITY,       Math.round(Number.NEGATIVE_INFINITY));
    array[item++] = new TestCase( SECTION,  "Math.round(0.49)",     0,              Math.round(0.49));
    array[item++] = new TestCase( SECTION,  "Math.round(0.5)",      1,              Math.round(0.5));
    array[item++] = new TestCase( SECTION,  "Math.round(0.51)",     1,              Math.round(0.51));

    array[item++] = new TestCase( SECTION,  "Math.round(-0.49)",    -0,             Math.round(-0.49));
    array[item++] = new TestCase( SECTION,  "Math.round(-0.5)",     -0,             Math.round(-0.5));
    array[item++] = new TestCase( SECTION,  "Infinity/Math.round(-0.49)",    -Infinity,             Infinity/Math.round(-0.49));
    array[item++] = new TestCase( SECTION,  "Infinity/Math.round(-0.5)",     -Infinity,             Infinity/Math.round(-0.5));

    array[item++] = new TestCase( SECTION,  "Math.round(-0.51)",    -1,             Math.round(-0.51));
    array[item++] = new TestCase( SECTION,  "Math.round(3.5)",      4,              Math.round(3.5));
    array[item++] = new TestCase( SECTION,  "Math.round(-3.5)",     -3,             Math.round(-3));

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