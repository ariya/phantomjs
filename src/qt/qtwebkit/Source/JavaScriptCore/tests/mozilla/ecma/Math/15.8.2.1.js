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
    File Name:          15.8.2.1.js
    ECMA Section:       15.8.2.1 abs( x )
    Description:        return the absolute value of the argument,
                        which should be the magnitude of the argument
                        with a positive sign.
                        -   if x is NaN, return NaN
                        -   if x is -0, result is +0
                        -   if x is -Infinity, result is +Infinity
    Author:             christine@netscape.com
    Date:               7 july 1997
*/
    var SECTION = "15.8.2.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.abs()";
    var BUGNUMBER = "77391";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "Math.abs.length",             1,              Math.abs.length );

    array[item++] = new TestCase( SECTION,   "Math.abs()",                  Number.NaN,     Math.abs() );
    array[item++] = new TestCase( SECTION,   "Math.abs( void 0 )",          Number.NaN,     Math.abs(void 0) );
    array[item++] = new TestCase( SECTION,   "Math.abs( null )",            0,              Math.abs(null) );
    array[item++] = new TestCase( SECTION,   "Math.abs( true )",            1,              Math.abs(true) );
    array[item++] = new TestCase( SECTION,   "Math.abs( false )",           0,              Math.abs(false) );
    array[item++] = new TestCase( SECTION,   "Math.abs( string primitive)", Number.NaN,     Math.abs("a string primitive")                  );
    array[item++] = new TestCase( SECTION,   "Math.abs( string object )",   Number.NaN,     Math.abs(new String( 'a String object' ))       );
    array[item++] = new TestCase( SECTION,   "Math.abs( Number.NaN )",      Number.NaN,     Math.abs(Number.NaN) );

    array[item++] = new TestCase( SECTION,   "Math.abs(0)",                 0,              Math.abs( 0 )                                   );
    array[item++] = new TestCase( SECTION,   "Math.abs( -0 )",              0,              Math.abs(-0) );
    array[item++] = new TestCase( SECTION,   "Infinity/Math.abs(-0)",      Infinity,        Infinity/Math.abs(-0) );

    array[item++] = new TestCase( SECTION,   "Math.abs( -Infinity )",       Number.POSITIVE_INFINITY,   Math.abs( Number.NEGATIVE_INFINITY ) );
    array[item++] = new TestCase( SECTION,   "Math.abs( Infinity )",        Number.POSITIVE_INFINITY,   Math.abs( Number.POSITIVE_INFINITY ) );
    array[item++] = new TestCase( SECTION,   "Math.abs( - MAX_VALUE )",     Number.MAX_VALUE,           Math.abs( - Number.MAX_VALUE )       );
    array[item++] = new TestCase( SECTION,   "Math.abs( - MIN_VALUE )",     Number.MIN_VALUE,           Math.abs( -Number.MIN_VALUE )        );
    array[item++] = new TestCase( SECTION,   "Math.abs( MAX_VALUE )",       Number.MAX_VALUE,           Math.abs( Number.MAX_VALUE )       );
    array[item++] = new TestCase( SECTION,   "Math.abs( MIN_VALUE )",       Number.MIN_VALUE,           Math.abs( Number.MIN_VALUE )        );

    array[item++] = new TestCase( SECTION,   "Math.abs( -1 )",               1,                          Math.abs( -1 )                       );
    array[item++] = new TestCase( SECTION,   "Math.abs( new Number( -1 ) )", 1,                          Math.abs( new Number(-1) )           );
    array[item++] = new TestCase( SECTION,   "Math.abs( 1 )",                1,                          Math.abs( 1 ) );
    array[item++] = new TestCase( SECTION,   "Math.abs( Math.PI )",          Math.PI,                    Math.abs( Math.PI ) );
    array[item++] = new TestCase( SECTION,   "Math.abs( -Math.PI )",         Math.PI,                    Math.abs( -Math.PI ) );
    array[item++] = new TestCase( SECTION,   "Math.abs(-1/100000000)",       1/100000000,                Math.abs(-1/100000000) );
    array[item++] = new TestCase( SECTION,   "Math.abs(-Math.pow(2,32))",    Math.pow(2,32),             Math.abs(-Math.pow(2,32)) );
    array[item++] = new TestCase( SECTION,   "Math.abs(Math.pow(2,32))",     Math.pow(2,32),             Math.abs(Math.pow(2,32)) );
    array[item++] = new TestCase( SECTION,   "Math.abs( -0xfff )",           4095,                       Math.abs( -0xfff ) );
    array[item++] = new TestCase( SECTION,   "Math.abs( -0777 )",            511,                        Math.abs(-0777 ) );

    array[item++] = new TestCase( SECTION,   "Math.abs('-1e-1')",           0.1,            Math.abs('-1e-1') );
    array[item++] = new TestCase( SECTION,   "Math.abs('0xff')",            255,            Math.abs('0xff') );
    array[item++] = new TestCase( SECTION,   "Math.abs('077')",             77,             Math.abs('077') );
    array[item++] = new TestCase( SECTION,   "Math.abs( 'Infinity' )",      Infinity,       Math.abs('Infinity') );
    array[item++] = new TestCase( SECTION,   "Math.abs( '-Infinity' )",     Infinity,       Math.abs('-Infinity') );

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
