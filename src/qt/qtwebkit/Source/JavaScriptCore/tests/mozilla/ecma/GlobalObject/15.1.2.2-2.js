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
    File Name:          15.1.2.2-1.js
    ECMA Section:       15.1.2.2 Function properties of the global object
                        parseInt( string, radix )

    Description:        parseInt test cases written by waldemar, and documented in
                        http://scopus.mcom.com/bugsplat/show_bug.cgi?id=123874.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
var SECTION = "15.1.2.2-2";
var VERSION = "ECMA_1";
    startTest();
var TITLE   = "parseInt(string, radix)";
var BUGNUMBER="123874";

writeHeaderToLog( SECTION + " "+ TITLE);

var testcases = new Array();

testcases[tc++] = new TestCase( SECTION,
    'parseInt("000000100000000100100011010001010110011110001001101010111100",2)',
    9027215253084860,
    parseInt("000000100000000100100011010001010110011110001001101010111100",2) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("000000100000000100100011010001010110011110001001101010111101",2)',
    9027215253084860,
    parseInt("000000100000000100100011010001010110011110001001101010111101",2));

testcases[tc++] = new TestCase( SECTION,
    'parseInt("000000100000000100100011010001010110011110001001101010111111",2)',
    9027215253084864,
    parseInt("000000100000000100100011010001010110011110001001101010111111",2) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0000001000000001001000110100010101100111100010011010101111010",2)',
    18054430506169720,
    parseInt("0000001000000001001000110100010101100111100010011010101111010",2) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0000001000000001001000110100010101100111100010011010101111011",2)',
    18054430506169724,
    parseInt("0000001000000001001000110100010101100111100010011010101111011",2));

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0000001000000001001000110100010101100111100010011010101111100",2)',
    18054430506169724,
    parseInt("0000001000000001001000110100010101100111100010011010101111100",2) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0000001000000001001000110100010101100111100010011010101111110",2)',
    18054430506169728,
    parseInt("0000001000000001001000110100010101100111100010011010101111110",2) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("yz",35)',
    34,
    parseInt("yz",35) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("yz",36)',
    1259,
    parseInt("yz",36) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("yz",37)',
    NaN,
    parseInt("yz",37) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("+77")',
    77,
    parseInt("+77") );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("-77",9)',
    -70,
    parseInt("-77",9) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("\u20001234\u2000")',
    1234,
    parseInt("\u20001234\u2000") );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("123456789012345678")',
    123456789012345680,
    parseInt("123456789012345678") );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("9",8)',
    NaN,
    parseInt("9",8) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("1e2")',
    1,
    parseInt("1e2") );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("1.9999999999999999999")',
    1,
    parseInt("1.9999999999999999999") );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0x10")',
    16,
    parseInt("0x10") );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0x10",10)',
    0,
    parseInt("0x10",10));

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0022")',
    22,
    parseInt("0022"));

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0022",10)',
    22,
    parseInt("0022",10) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0x1000000000000080")',
    1152921504606847000,
    parseInt("0x1000000000000080") );

testcases[tc++] = new TestCase( SECTION,
    'parseInt("0x1000000000000081")',
    1152921504606847200,
    parseInt("0x1000000000000081") );

s =
"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"

s += "0000000000000000000000000000000000000";

testcases[tc++] = new TestCase( SECTION,
    "s = " + s +"; -s",
    -1.7976931348623157e+308,
    -s );

s =
"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
s += "0000000000000000000000000000000000001";

testcases[tc++] = new TestCase( SECTION,
    "s = " + s +"; -s",
    -1.7976931348623157e+308,
    -s );


s = "0xFFFFFFFFFFFFFC0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

s += "0000000000000000000000000000000000000"


testcases[tc++] = new TestCase( SECTION,
    "s = " + s + "; -s",
    -Infinity,
    -s );

s = "0xFFFFFFFFFFFFFB0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
s += "0000000000000000000000000000000000001";

testcases[tc++] = new TestCase( SECTION,
    "s = " + s + "; -s",
    -1.7976931348623157e+308,
    -s );

s += "0"

testcases[tc++] = new TestCase( SECTION,
    "s = " + s + "; -s",
    -Infinity,
    -s );

testcases[tc++] = new TestCase( SECTION,
    'parseInt(s)',
    Infinity,
    parseInt(s) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt(s,32)',
    0,
    parseInt(s,32) );

testcases[tc++] = new TestCase( SECTION,
    'parseInt(s,36)',
    Infinity,
    parseInt(s,36));

test();

function test( array ) {
    for ( tc=0 ; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
