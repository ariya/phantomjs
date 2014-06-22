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
    File Name:          9.5-2.js
    ECMA Section:       9.5  Type Conversion:  ToInt32
    Description:        rules for converting an argument to a signed 32 bit integer

                        this test uses << 0 to convert the argument to a 32bit
                        integer.

                        The operator ToInt32 converts its argument to one of 2^32
                        integer values in the range -2^31 through 2^31 inclusive.
                        This operator functions as follows:

                        1 call ToNumber on argument
                        2 if result is NaN, 0, -0, return 0
                        3 compute (sign (result(1)) * floor(abs(result 1)))
                        4 compute result(3) modulo 2^32:
                        5 if result(4) is greater than or equal to 2^31, return
                          result(5)-2^32.  otherwise, return result(5)

                        special cases:
                            -0          returns 0
                            Infinity    returns 0
                            -Infinity   returns 0
                            ToInt32(ToUint32(x)) == ToInt32(x) for all values of x
                            Numbers greater than 2^31 (see step 5 above)
                            (note http://bugzilla.mozilla.org/show_bug.cgi?id=120083)

    Author:             christine@netscape.com
    Date:               17 july 1997
*/
    var SECTION = "9.5-2";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " ToInt32");
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
function ToInt32( n ) {
    n = Number( n );
    var sign = ( n < 0 ) ? -1 : 1;

    if ( Math.abs( n ) == 0 || Math.abs( n ) == Number.POSITIVE_INFINITY) {
        return 0;
    }

    n = (sign * Math.floor( Math.abs(n) )) % Math.pow(2,32);
    if ( sign == -1 ) {
        n = ( n < -Math.pow(2,31) ) ? n + Math.pow(2,32) : n;
    } else{
        n = ( n >= Math.pow(2,31) ) ? n - Math.pow(2,32) : n;
    }

    return ( n );
}
function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "0 << 0",                        0,              0 << 0 );
    array[item++] = new TestCase( SECTION,   "-0 << 0",                       0,              -0 << 0 );
    array[item++] = new TestCase( SECTION,   "Infinity << 0",                 0,              "Infinity" << 0 );
    array[item++] = new TestCase( SECTION,   "-Infinity << 0",                0,              "-Infinity" << 0 );
    array[item++] = new TestCase( SECTION,   "Number.POSITIVE_INFINITY << 0", 0,              Number.POSITIVE_INFINITY << 0 );
    array[item++] = new TestCase( SECTION,   "Number.NEGATIVE_INFINITY << 0", 0,              Number.NEGATIVE_INFINITY << 0 );
    array[item++] = new TestCase( SECTION,   "Number.NaN << 0",               0,              Number.NaN << 0 );

    array[item++] = new TestCase( SECTION,   "Number.MIN_VALUE << 0",         0,              Number.MIN_VALUE << 0 );
    array[item++] = new TestCase( SECTION,   "-Number.MIN_VALUE << 0",        0,              -Number.MIN_VALUE << 0 );
    array[item++] = new TestCase( SECTION,   "0.1 << 0",                      0,              0.1 << 0 );
    array[item++] = new TestCase( SECTION,   "-0.1 << 0",                     0,              -0.1 << 0 );
    array[item++] = new TestCase( SECTION,   "1 << 0",                        1,              1 << 0 );
    array[item++] = new TestCase( SECTION,   "1.1 << 0",                      1,              1.1 << 0 );
    array[item++] = new TestCase( SECTION,   "-1 << 0",                     ToInt32(-1),             -1 << 0 );


    array[item++] = new TestCase( SECTION,   "2147483647 << 0",     ToInt32(2147483647),    2147483647 << 0 );
    array[item++] = new TestCase( SECTION,   "2147483648 << 0",     ToInt32(2147483648),    2147483648 << 0 );
    array[item++] = new TestCase( SECTION,   "2147483649 << 0",     ToInt32(2147483649),    2147483649 << 0 );

    array[item++] = new TestCase( SECTION,   "(Math.pow(2,31)-1) << 0", ToInt32(2147483647),    (Math.pow(2,31)-1) << 0 );
    array[item++] = new TestCase( SECTION,   "Math.pow(2,31) << 0",     ToInt32(2147483648),    Math.pow(2,31) << 0 );
    array[item++] = new TestCase( SECTION,   "(Math.pow(2,31)+1) << 0", ToInt32(2147483649),    (Math.pow(2,31)+1) << 0 );

    array[item++] = new TestCase( SECTION,   "(Math.pow(2,32)-1) << 0",   ToInt32(4294967295),    (Math.pow(2,32)-1) << 0 );
    array[item++] = new TestCase( SECTION,   "(Math.pow(2,32)) << 0",     ToInt32(4294967296),    (Math.pow(2,32)) << 0 );
    array[item++] = new TestCase( SECTION,   "(Math.pow(2,32)+1) << 0",   ToInt32(4294967297),    (Math.pow(2,32)+1) << 0 );

    array[item++] = new TestCase( SECTION,   "4294967295 << 0",     ToInt32(4294967295),    4294967295 << 0 );
    array[item++] = new TestCase( SECTION,   "4294967296 << 0",     ToInt32(4294967296),    4294967296 << 0 );
    array[item++] = new TestCase( SECTION,   "4294967297 << 0",     ToInt32(4294967297),    4294967297 << 0 );

    array[item++] = new TestCase( SECTION,   "'2147483647' << 0",   ToInt32(2147483647),    '2147483647' << 0 );
    array[item++] = new TestCase( SECTION,   "'2147483648' << 0",   ToInt32(2147483648),    '2147483648' << 0 );
    array[item++] = new TestCase( SECTION,   "'2147483649' << 0",   ToInt32(2147483649),    '2147483649' << 0 );

    array[item++] = new TestCase( SECTION,   "'4294967295' << 0",   ToInt32(4294967295),    '4294967295' << 0 );
    array[item++] = new TestCase( SECTION,   "'4294967296' << 0",   ToInt32(4294967296),    '4294967296' << 0 );
    array[item++] = new TestCase( SECTION,   "'4294967297' << 0",   ToInt32(4294967297),    '4294967297' << 0 );

    array[item++] = new TestCase( SECTION,   "-2147483647 << 0",    ToInt32(-2147483647),   -2147483647	<< 0 );
    array[item++] = new TestCase( SECTION,   "-2147483648 << 0",    ToInt32(-2147483648),   -2147483648 << 0 );
    array[item++] = new TestCase( SECTION,   "-2147483649 << 0",    ToInt32(-2147483649),   -2147483649 << 0 );

    array[item++] = new TestCase( SECTION,   "-4294967295 << 0",    ToInt32(-4294967295),   -4294967295 << 0 );
    array[item++] = new TestCase( SECTION,   "-4294967296 << 0",    ToInt32(-4294967296),   -4294967296 << 0 );
    array[item++] = new TestCase( SECTION,   "-4294967297 << 0",    ToInt32(-4294967297),   -4294967297 << 0 );

    /*
     * Numbers between 2^31 and 2^32 will have a negative ToInt32 per ECMA (see step 5 of introduction)
     * (These are by stevechapel@earthlink.net; cf. http://bugzilla.mozilla.org/show_bug.cgi?id=120083)
     */
    array[item++] = new TestCase( SECTION,   "2147483648.25 << 0",  ToInt32(2147483648.25),   2147483648.25 << 0 );
    array[item++] = new TestCase( SECTION,   "2147483648.5 << 0",   ToInt32(2147483648.5),    2147483648.5 << 0 );
    array[item++] = new TestCase( SECTION,   "2147483648.75 << 0",  ToInt32(2147483648.75),   2147483648.75 << 0 );
    array[item++] = new TestCase( SECTION,   "4294967295.25 << 0",  ToInt32(4294967295.25),   4294967295.25 << 0 );
    array[item++] = new TestCase( SECTION,   "4294967295.5 << 0",   ToInt32(4294967295.5),    4294967295.5 << 0 );
    array[item++] = new TestCase( SECTION,   "4294967295.75 << 0",  ToInt32(4294967295.75),   4294967295.75 << 0 );
    array[item++] = new TestCase( SECTION,   "3000000000.25 << 0",  ToInt32(3000000000.25),   3000000000.25 << 0 );
    array[item++] = new TestCase( SECTION,   "3000000000.5 << 0",   ToInt32(3000000000.5),    3000000000.5 << 0 );
    array[item++] = new TestCase( SECTION,   "3000000000.75 << 0",  ToInt32(3000000000.75),   3000000000.75 << 0 );

    /*
     * Numbers between - 2^31 and - 2^32 
     */
    array[item++] = new TestCase( SECTION,   "-2147483648.25 << 0",  ToInt32(-2147483648.25),   -2147483648.25 << 0 );
    array[item++] = new TestCase( SECTION,   "-2147483648.5 << 0",   ToInt32(-2147483648.5),    -2147483648.5 << 0 );
    array[item++] = new TestCase( SECTION,   "-2147483648.75 << 0",  ToInt32(-2147483648.75),   -2147483648.75 << 0 );
    array[item++] = new TestCase( SECTION,   "-4294967295.25 << 0",  ToInt32(-4294967295.25),   -4294967295.25 << 0 );
    array[item++] = new TestCase( SECTION,   "-4294967295.5 << 0",   ToInt32(-4294967295.5),    -4294967295.5 << 0 );
    array[item++] = new TestCase( SECTION,   "-4294967295.75 << 0",  ToInt32(-4294967295.75),   -4294967295.75 << 0 );
    array[item++] = new TestCase( SECTION,   "-3000000000.25 << 0",  ToInt32(-3000000000.25),   -3000000000.25 << 0 );
    array[item++] = new TestCase( SECTION,   "-3000000000.5 << 0",   ToInt32(-3000000000.5),    -3000000000.5 << 0 );
    array[item++] = new TestCase( SECTION,   "-3000000000.75 << 0",  ToInt32(-3000000000.75),   -3000000000.75 << 0 );

    return ( array );
}
