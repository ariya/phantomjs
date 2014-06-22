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

    Description:

    The parseInt function produces an integer value dictated by intepretation
    of the contents of the string argument according to the specified radix.

    When the parseInt function is called, the following steps are taken:

   1.   Call ToString(string).
   2.   Compute a substring of Result(1) consisting of the leftmost character
        that is not a StrWhiteSpaceChar and all characters to the right of
        that character. (In other words, remove leading whitespace.)
   3.   Let sign be 1.
   4.   If Result(2) is not empty and the first character of Result(2) is a
        minus sign -, let sign be -1.
   5.   If Result(2) is not empty and the first character of Result(2) is a
        plus sign + or a minus sign -, then Result(5) is the substring of
        Result(2) produced by removing the first character; otherwise, Result(5)
        is Result(2).
   6.   If the radix argument is not supplied, go to step 12.
   7.   Call ToInt32(radix).
   8.   If Result(7) is zero, go to step 12; otherwise, if Result(7) < 2 or
        Result(7) > 36, return NaN.
   9.   Let R be Result(7).
  10.   If R = 16 and the length of Result(5) is at least 2 and the first two
        characters of Result(5) are either "0x" or "0X", let S be the substring
        of Result(5) consisting of all but the first two characters; otherwise,
        let S be Result(5).
  11.   Go to step 22.
  12.   If Result(5) is empty or the first character of Result(5) is not 0,
        go to step 20.
  13.   If the length of Result(5) is at least 2 and the second character of
        Result(5) is x or X, go to step 17.
  14.   Let R be 8.
  15.   Let S be Result(5).
  16.   Go to step 22.
  17.   Let R be 16.
  18.   Let S be the substring of Result(5) consisting of all but the first
        two characters.
  19.   Go to step 22.
  20.   Let R be 10.
  21.   Let S be Result(5).
  22.   If S contains any character that is not a radix-R digit, then let Z be
        the substring of S consisting of all characters to the left of the
        leftmost such character; otherwise, let Z be S.
  23.   If Z is empty, return NaN.
  24.   Compute the mathematical integer value that is represented by Z in
        radix-R notation. (But if R is 10 and Z contains more than 20
        significant digits, every digit after the 20th may be replaced by a 0
        digit, at the option of the implementation; and if R is not 2, 4, 8,
        10, 16, or 32, then Result(24) may be an implementation-dependent
        approximation to the mathematical integer value that is represented
        by Z in radix-R notation.)
  25.   Compute the number value for Result(24).
  26.   Return sign Result(25).

    Note that parseInt may interpret only a leading portion of the string as
    an integer value; it ignores any characters that cannot be interpreted as
    part of the notation of an integer, and no indication is given that any
    such characters were ignored.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.1.2.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "parseInt(string, radix)";
    var BUGNUMBER="111199";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var HEX_STRING = "0x0";
    var HEX_VALUE = 0;

    array[item++] = new TestCase( SECTION,  "parseInt.length",      2,      parseInt.length );
    array[item++] = new TestCase( SECTION,  "parseInt.length = 0; parseInt.length",     2,      eval("parseInt.length = 0; parseInt.length") );
    array[item++] = new TestCase( SECTION,  "var PROPS=''; for ( var p in parseInt ) { PROPS += p; }; PROPS",   "", eval("var PROPS=''; for ( var p in parseInt ) { PROPS += p; }; PROPS") );
    array[item++] = new TestCase( SECTION,  "delete parseInt.length",   false,  delete parseInt.length );
    array[item++] = new TestCase( SECTION,  "delete parseInt.length; parseInt.length",  2,  eval("delete parseInt.length; parseInt.length") );
    array[item++] = new TestCase( SECTION,  "parseInt.length = null; parseInt.length",  2,  eval("parseInt.length = null; parseInt.length") );

    array[item++] = new TestCase( SECTION,  "parseInt()",       NaN,    parseInt() );
    array[item++] = new TestCase( SECTION,  "parseInt('')",     NaN,    parseInt("") );
    array[item++] = new TestCase( SECTION,  "parseInt('','')",  NaN,    parseInt("","") );
    array[item++] = new TestCase( SECTION,
                    "parseInt(\"     0xabcdef     ",
                    11259375,
                    parseInt( "      0xabcdef     " ));

    array[item++] = new TestCase( SECTION,
                    "parseInt(\"     0XABCDEF     ",
                    11259375,
                    parseInt( "      0XABCDEF     " ) );

    array[item++] = new TestCase( SECTION,
                    "parseInt( 0xabcdef )",
                    11259375,
                    parseInt( "0xabcdef") );

    array[item++] = new TestCase( SECTION,
                    "parseInt( 0XABCDEF )",
                    11259375,
                    parseInt( "0XABCDEF") );

    for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "0X0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+",null)",    HEX_VALUE,  parseInt(HEX_STRING,null) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+", void 0)",    HEX_VALUE,  parseInt(HEX_STRING, void 0) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }

    // a few tests with spaces

    for ( var space = " ", HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0;
        POWER < 15;
        POWER++, HEX_STRING = HEX_STRING +"f", space += " ")
    {
        array[item++] = new TestCase( SECTION, "parseInt("+space+HEX_STRING+space+", void 0)",    HEX_VALUE,  parseInt(space+HEX_STRING+space, void 0) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }

    // a few tests with negative numbers
    for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
        HEX_VALUE -= Math.pow(16,POWER)*15;
    }

    // we should stop parsing when we get to a value that is not a numeric literal for the type we expect

    for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+"g,16)",    HEX_VALUE,  parseInt(HEX_STRING+"g",16) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+"g,16)",    HEX_VALUE,  parseInt(HEX_STRING+"G",16) );
        HEX_VALUE += Math.pow(16,POWER)*15;
    }

    for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
        HEX_VALUE -= Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "-0X0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+")",    HEX_VALUE,  parseInt(HEX_STRING) );
        HEX_VALUE -= Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
        HEX_VALUE -= Math.pow(16,POWER)*15;
    }
    for ( HEX_STRING = "-0x0", HEX_VALUE = 0, POWER = 0; POWER < 15; POWER++, HEX_STRING = HEX_STRING +"f" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+HEX_STRING+",16)",    HEX_VALUE,  parseInt(HEX_STRING,16) );
        HEX_VALUE -= Math.pow(16,POWER)*15;
    }

    //  let us do some 'octal' tests - ES5 does not permit octal, these must be treated as decimal.

    var OCT_STRING = "0";
    var OCT_VALUE = 0;

    for ( OCT_STRING = "0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+OCT_STRING+")",    OCT_VALUE,  parseInt(OCT_STRING) );
        OCT_VALUE += Math.pow(10,POWER)*7;
    }

    for ( OCT_STRING = "-0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+OCT_STRING+")",    OCT_VALUE,  parseInt(OCT_STRING) );
        OCT_VALUE -= Math.pow(10,POWER)*7;
    }

    // should get the same results as above if we provid the radix of 8 (or 010)

    for ( OCT_STRING = "0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+OCT_STRING+",8)",    OCT_VALUE,  parseInt(OCT_STRING,8) );
        OCT_VALUE += Math.pow(8,POWER)*7;
    }
    for ( OCT_STRING = "-0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+OCT_STRING+",010)",    OCT_VALUE,  parseInt(OCT_STRING,010) );
        OCT_VALUE -= Math.pow(8,POWER)*7;
    }

    // we shall stop parsing digits when we get one that isn't a numeric literal of the type we think
    // it should be.
    for ( OCT_STRING = "0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+OCT_STRING+"8,8)",    OCT_VALUE,  parseInt(OCT_STRING+"8",8) );
        OCT_VALUE += Math.pow(8,POWER)*7;
    }
    for ( OCT_STRING = "-0", OCT_VALUE = 0, POWER = 0; POWER < 15; POWER++, OCT_STRING = OCT_STRING +"7" ) {
        array[item++] = new TestCase( SECTION, "parseInt("+OCT_STRING+"8,010)",    OCT_VALUE,  parseInt(OCT_STRING+"8",010) );
        OCT_VALUE -= Math.pow(8,POWER)*7;
    }

    array[item++] = new TestCase( SECTION, "parseInt( '0x' )",              NaN,        parseInt("0x") );
    array[item++] = new TestCase( SECTION, "parseInt( '0X' )",              NaN,        parseInt("0X") );

    array[item++] = new TestCase( SECTION, "parseInt( '11111111112222222222' )",    11111111112222222222,   parseInt("11111111112222222222") );
    array[item++] = new TestCase( SECTION, "parseInt( '111111111122222222223' )",    111111111122222222220,   parseInt("111111111122222222223") );
    array[item++] = new TestCase( SECTION, "parseInt( '11111111112222222222',10 )",    11111111112222222222,   parseInt("11111111112222222222",10) );
    array[item++] = new TestCase( SECTION, "parseInt( '111111111122222222223',10 )",    111111111122222222220,   parseInt("111111111122222222223",10) );

    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', -1 )",  Number.NaN,    parseInt("01234567890",-1) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 0 )",  Number.NaN,     parseInt("01234567890",1) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 1 )",  Number.NaN,     parseInt("01234567890",1) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 2 )",  1,              parseInt("01234567890",2) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 3 )",  5,              parseInt("01234567890",3) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 4 )",  27,             parseInt("01234567890",4) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 5 )",  194,            parseInt("01234567890",5) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 6 )",  1865,           parseInt("01234567890",6) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 7 )",  22875,          parseInt("01234567890",7) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 8 )",  342391,         parseInt("01234567890",8) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 9 )",  6053444,        parseInt("01234567890",9) );
    array[item++] = new TestCase( SECTION, "parseInt( '01234567890', 10 )", 1234567890,     parseInt("01234567890",10) );

    // need more test cases with hex radix

    array[item++] = new TestCase( SECTION, "parseInt( '1234567890', '0xa')", 1234567890, parseInt("1234567890","0xa") );

    array[item++] = new TestCase( SECTION, "parseInt( '012345', 11 )",      17715,          parseInt("012345",11) );

    array[item++] = new TestCase( SECTION, "parseInt( '012345', 35 )",      1590195,        parseInt("012345",35) );
    array[item++] = new TestCase( SECTION, "parseInt( '012345', 36 )",      1776965,        parseInt("012345",36) );
    array[item++] = new TestCase( SECTION, "parseInt( '012345', 37 )",      Number.NaN,     parseInt("012345",37) );

    return ( array );
}
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
