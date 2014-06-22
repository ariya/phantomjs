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
    File Name:          7.7.3-1.js
    ECMA Section:       7.7.3 Numeric Literals

    Description:        A numeric literal stands for a value of the Number type
                        This value is determined in two steps:  first a
                        mathematical value (MV) is derived from the literal;
                        second, this mathematical value is rounded, ideally
                        using IEEE 754 round-to-nearest mode, to a reprentable
                        value of of the number type.

                        These test cases came from Waldemar.

    Author:             christine@netscape.com
    Date:               12 June 1998
*/

var SECTION = "7.7.3-1";
var VERSION = "ECMA_1";
    startTest();
var TITLE   = "Numeric Literals";
var BUGNUMBER="122877";

writeHeaderToLog( SECTION + " "+ TITLE);

var testcases = new Array();


testcases[tc++] = new TestCase( SECTION,
    "0x12345678",
    305419896,
    0x12345678 );

testcases[tc++] = new TestCase( SECTION,
    "0x80000000",
    2147483648,
    0x80000000 );

testcases[tc++] = new TestCase( SECTION,
    "0xffffffff",
    4294967295,
    0xffffffff );

testcases[tc++] = new TestCase( SECTION,
    "0x100000000",
    4294967296,
    0x100000000 );

testcases[tc++] = new TestCase( SECTION,
    "077777777777777777",
    2251799813685247,
    077777777777777777 );

testcases[tc++] = new TestCase( SECTION,
    "077777777777777776",
    2251799813685246,
    077777777777777776 );

testcases[tc++] = new TestCase( SECTION,
    "0x1fffffffffffff",
    9007199254740991,
    0x1fffffffffffff );

testcases[tc++] = new TestCase( SECTION,
    "0x20000000000000",
    9007199254740992,
    0x20000000000000 );

testcases[tc++] = new TestCase( SECTION,
    "0x20123456789abc",
    9027215253084860,
    0x20123456789abc );

testcases[tc++] = new TestCase( SECTION,
    "0x20123456789abd",
    9027215253084860,
    0x20123456789abd );

testcases[tc++] = new TestCase( SECTION,
    "0x20123456789abe",
    9027215253084862,
    0x20123456789abe );

testcases[tc++] = new TestCase( SECTION,
    "0x20123456789abf",
    9027215253084864,
    0x20123456789abf );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000080",
    1152921504606847000,
    0x1000000000000080 );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000081",
    1152921504606847200,
    0x1000000000000081 );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000100",
    1152921504606847200,
    0x1000000000000100 );

testcases[tc++] = new TestCase( SECTION,
    "0x100000000000017f",
    1152921504606847200,
    0x100000000000017f );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000180",
    1152921504606847500,
    0x1000000000000180 );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000181",
    1152921504606847500,
    0x1000000000000181 );

testcases[tc++] = new TestCase( SECTION,
    "0x10000000000001f0",
    1152921504606847500,
    0x10000000000001f0 );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000200",
    1152921504606847500,
    0x1000000000000200 );

testcases[tc++] = new TestCase( SECTION,
    "0x100000000000027f",
    1152921504606847500,
    0x100000000000027f );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000280",
    1152921504606847500,
    0x1000000000000280 );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000281",
    1152921504606847700,
    0x1000000000000281 );

testcases[tc++] = new TestCase( SECTION,
    "0x10000000000002ff",
    1152921504606847700,
    0x10000000000002ff );

testcases[tc++] = new TestCase( SECTION,
    "0x1000000000000300",
    1152921504606847700,
    0x1000000000000300 );

testcases[tc++] = new TestCase( SECTION,
    "0x10000000000000000",
    18446744073709552000,
    0x10000000000000000 );

test();

function test() {
        for ( tc=0; tc < testcases.length; tc++ ) {
            testcases[tc].actual = testcases[tc].actual;

            testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";

        }

        stopTest();
        return ( testcases );
}
