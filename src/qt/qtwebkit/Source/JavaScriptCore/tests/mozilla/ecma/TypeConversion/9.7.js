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
    File Name:          9.7.js
    ECMA Section:       9.7  Type Conversion:  ToInt16
    Description:        rules for converting an argument to an unsigned
                        16 bit integer in the range 0 to 2^16-1.

                        this test uses String.prototype.fromCharCode() and
                        String.prototype.charCodeAt() to test ToInt16.

                        special cases:
                            -0          returns 0
                            Infinity    returns 0
                            -Infinity   returns 0
                            0           returns 0

    Author:             christine@netscape.com
    Date:               17 july 1997
*/
    var SECTION = "9.7";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Type Conversion:  ToInt16");
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
function ToInt16( num ) {
    num = Number( num );
    if ( isNaN( num ) || num == 0 || num == Number.POSITIVE_INFINITY || num == Number.NEGATIVE_INFINITY ) {
        return 0;
    }

    var sign = ( num < 0 ) ? -1 : 1;

    num = sign * Math.floor( Math.abs( num ) );

    num = num % Math.pow(2,16);

    num = ( num > -65536 && num < 0) ? 65536 + num : num;

    return num;
}

function getTestCases() {
    var array = new Array();
    var item = 0;
/*
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(0).charCodeAt(0)",          0,      String.fromCharCode(0).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-0).charCodeAt(0)",         0,      String.fromCharCode(-0).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(1).charCodeAt(0)",          1,      String.fromCharCode(1).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(64).charCodeAt(0)",         64,     String.fromCharCode(64).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(126).charCodeAt(0)",        126,    String.fromCharCode(126).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(127).charCodeAt(0)",        127,    String.fromCharCode(127).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(128).charCodeAt(0)",        128,    String.fromCharCode(128).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(130).charCodeAt(0)",        130,    String.fromCharCode(130).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(255).charCodeAt(0)",        255,    String.fromCharCode(255).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(256).charCodeAt(0)",        256,    String.fromCharCode(256).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(Math.pow(2,16)-1).charCodeAt(0)",   65535,  String.fromCharCode(Math.pow(2,16)-1).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(Math.pow(2,16)).charCodeAt(0)",     0,      String.fromCharCode(Math.pow(2,16)).charCodeAt(0) );
*/


    array[item++] = new TestCase( "9.7",   "String.fromCharCode(0).charCodeAt(0)",          ToInt16(0),      String.fromCharCode(0).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-0).charCodeAt(0)",         ToInt16(0),      String.fromCharCode(-0).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(1).charCodeAt(0)",          ToInt16(1),      String.fromCharCode(1).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(64).charCodeAt(0)",         ToInt16(64),     String.fromCharCode(64).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(126).charCodeAt(0)",        ToInt16(126),    String.fromCharCode(126).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(127).charCodeAt(0)",        ToInt16(127),    String.fromCharCode(127).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(128).charCodeAt(0)",        ToInt16(128),    String.fromCharCode(128).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(130).charCodeAt(0)",        ToInt16(130),    String.fromCharCode(130).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(255).charCodeAt(0)",        ToInt16(255),    String.fromCharCode(255).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(256).charCodeAt(0)",        ToInt16(256),    String.fromCharCode(256).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode(Math.pow(2,16)-1).charCodeAt(0)",   65535,  String.fromCharCode(Math.pow(2,16)-1).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(Math.pow(2,16)).charCodeAt(0)",     0,      String.fromCharCode(Math.pow(2,16)).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode(65535).charCodeAt(0)",     ToInt16(65535),      String.fromCharCode(65535).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(65536).charCodeAt(0)",     ToInt16(65536),      String.fromCharCode(65536).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(65537).charCodeAt(0)",     ToInt16(65537),      String.fromCharCode(65537).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode(131071).charCodeAt(0)",     ToInt16(131071),    String.fromCharCode(131071).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(131072).charCodeAt(0)",     ToInt16(131072),    String.fromCharCode(131072).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(131073).charCodeAt(0)",     ToInt16(131073),    String.fromCharCode(131073).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode('65535').charCodeAt(0)",     65535,             String.fromCharCode("65535").charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode('65536').charCodeAt(0)",     0,                 String.fromCharCode("65536").charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-1).charCodeAt(0)",         ToInt16(-1),        String.fromCharCode(-1).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-64).charCodeAt(0)",        ToInt16(-64),       String.fromCharCode(-64).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-126).charCodeAt(0)",       ToInt16(-126),      String.fromCharCode(-126).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-127).charCodeAt(0)",       ToInt16(-127),      String.fromCharCode(-127).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-128).charCodeAt(0)",       ToInt16(-128),      String.fromCharCode(-128).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-130).charCodeAt(0)",       ToInt16(-130),      String.fromCharCode(-130).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-255).charCodeAt(0)",       ToInt16(-255),      String.fromCharCode(-255).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-256).charCodeAt(0)",       ToInt16(-256),      String.fromCharCode(-256).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-Math.pow(2,16)-1).charCodeAt(0)",   65535,     String.fromCharCode(-Math.pow(2,16)-1).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-Math.pow(2,16)).charCodeAt(0)",     0,         String.fromCharCode(-Math.pow(2,16)).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-65535).charCodeAt(0)",     ToInt16(-65535),    String.fromCharCode(-65535).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-65536).charCodeAt(0)",     ToInt16(-65536),    String.fromCharCode(-65536).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-65537).charCodeAt(0)",     ToInt16(-65537),    String.fromCharCode(-65537).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-131071).charCodeAt(0)",    ToInt16(-131071),   String.fromCharCode(-131071).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-131072).charCodeAt(0)",    ToInt16(-131072),   String.fromCharCode(-131072).charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-131073).charCodeAt(0)",    ToInt16(-131073),   String.fromCharCode(-131073).charCodeAt(0) );

    array[item++] = new TestCase( "9.7",   "String.fromCharCode('-65535').charCodeAt(0)",   ToInt16(-65535),    String.fromCharCode("-65535").charCodeAt(0) );
    array[item++] = new TestCase( "9.7",   "String.fromCharCode('-65536').charCodeAt(0)",   ToInt16(-65536),    String.fromCharCode("-65536").charCodeAt(0) );


//    array[item++] = new TestCase( "9.7",   "String.fromCharCode(2147483648).charCodeAt(0)", ToInt16(2147483648),      String.fromCharCode(2147483648).charCodeAt(0) );



//    the following test cases cause a runtime error.  see:  http://scopus.mcom.com/bugsplat/show_bug.cgi?id=78878

//    array[item++] = new TestCase( "9.7",   "String.fromCharCode(Infinity).charCodeAt(0)",           0,      String.fromCharCode("Infinity").charCodeAt(0) );
//    array[item++] = new TestCase( "9.7",   "String.fromCharCode(-Infinity).charCodeAt(0)",          0,      String.fromCharCode("-Infinity").charCodeAt(0) );
//    array[item++] = new TestCase( "9.7",   "String.fromCharCode(NaN).charCodeAt(0)",                0,      String.fromCharCode(Number.NaN).charCodeAt(0) );
//    array[item++] = new TestCase( "9.7",   "String.fromCharCode(Number.POSITIVE_INFINITY).charCodeAt(0)",   0,  String.fromCharCode(Number.POSITIVE_INFINITY).charCodeAt(0) );
//    array[item++] = new TestCase( "9.7",   "String.fromCharCode(Number.NEGATIVE_INFINITY).charCodeAt(0)",   0,  String.fromCharCode(Number.NEGATIVE_INFINITY).charCodeAt(0) );
    return ( array );
}
