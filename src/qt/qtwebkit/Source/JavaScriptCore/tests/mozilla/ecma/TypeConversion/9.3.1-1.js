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
    File Name:          9.3.1-1.js
    ECMA Section:       9.3  Type Conversion:  ToNumber
    Description:        rules for converting an argument to a number.
                        see 9.3.1 for cases for converting strings to numbers.
                        special cases:
                        undefined           NaN
                        Null                NaN
                        Boolean             1 if true; +0 if false
                        Number              the argument ( no conversion )
                        String              see test 9.3.1
                        Object              see test 9.3-1


                        This tests ToNumber applied to the string type

    Author:             christine@netscape.com
    Date:               10 july 1997

*/
    var SECTION = "9.3.1-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "ToNumber applied to the String type";
    var BUGNUMBER="77391";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
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
function getTestCases() {
    var array = new Array();
    var item = 0;

    //  StringNumericLiteral:::StrWhiteSpace:::StrWhiteSpaceChar StrWhiteSpace:::
    //
    //  Name    Unicode Value   Escape Sequence
    //  <TAB>   0X0009          \t
    //  <SP>    0X0020
    //  <FF>    0X000C          \f
    //  <VT>    0X000B
    //  <CR>    0X000D          \r
    //  <LF>    0X000A          \n
    array[item++] = new TestCase( SECTION,  "Number('')",           0,      Number("") );
    array[item++] = new TestCase( SECTION,  "Number(' ')",          0,      Number(" ") );
    array[item++] = new TestCase( SECTION,  "Number(\\t)",          0,      Number("\t") );
    array[item++] = new TestCase( SECTION,  "Number(\\n)",          0,      Number("\n") );
    array[item++] = new TestCase( SECTION,  "Number(\\r)",          0,      Number("\r") );
    array[item++] = new TestCase( SECTION,  "Number(\\f)",          0,      Number("\f") );

    array[item++] = new TestCase( SECTION,  "Number(String.fromCharCode(0x0009)",   0,  Number(String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "Number(String.fromCharCode(0x0020)",   0,  Number(String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "Number(String.fromCharCode(0x000C)",   0,  Number(String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "Number(String.fromCharCode(0x000B)",   0,  Number(String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "Number(String.fromCharCode(0x000D)",   0,  Number(String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "Number(String.fromCharCode(0x000A)",   0,  Number(String.fromCharCode(0x000A)) );

    //  a StringNumericLiteral may be preceeded or followed by whitespace and/or
    //  line terminators

    array[item++] = new TestCase( SECTION,  "Number( '   ' +  999 )",        999,    Number( '   '+999) );
    array[item++] = new TestCase( SECTION,  "Number( '\\n'  + 999 )",       999,    Number( '\n' +999) );
    array[item++] = new TestCase( SECTION,  "Number( '\\r'  + 999 )",       999,    Number( '\r' +999) );
    array[item++] = new TestCase( SECTION,  "Number( '\\t'  + 999 )",       999,    Number( '\t' +999) );
    array[item++] = new TestCase( SECTION,  "Number( '\\f'  + 999 )",       999,    Number( '\f' +999) );

    array[item++] = new TestCase( SECTION,  "Number( 999 + '   ' )",        999,    Number( 999+'   ') );
    array[item++] = new TestCase( SECTION,  "Number( 999 + '\\n' )",        999,    Number( 999+'\n' ) );
    array[item++] = new TestCase( SECTION,  "Number( 999 + '\\r' )",        999,    Number( 999+'\r' ) );
    array[item++] = new TestCase( SECTION,  "Number( 999 + '\\t' )",        999,    Number( 999+'\t' ) );
    array[item++] = new TestCase( SECTION,  "Number( 999 + '\\f' )",        999,    Number( 999+'\f' ) );

    array[item++] = new TestCase( SECTION,  "Number( '\\n'  + 999 + '\\n' )",         999,    Number( '\n' +999+'\n' ) );
    array[item++] = new TestCase( SECTION,  "Number( '\\r'  + 999 + '\\r' )",         999,    Number( '\r' +999+'\r' ) );
    array[item++] = new TestCase( SECTION,  "Number( '\\t'  + 999 + '\\t' )",         999,    Number( '\t' +999+'\t' ) );
    array[item++] = new TestCase( SECTION,  "Number( '\\f'  + 999 + '\\f' )",         999,    Number( '\f' +999+'\f' ) );

    array[item++] = new TestCase( SECTION,  "Number( '   ' +  '999' )",     999,    Number( '   '+'999') );
    array[item++] = new TestCase( SECTION,  "Number( '\\n'  + '999' )",       999,    Number( '\n' +'999') );
    array[item++] = new TestCase( SECTION,  "Number( '\\r'  + '999' )",       999,    Number( '\r' +'999') );
    array[item++] = new TestCase( SECTION,  "Number( '\\t'  + '999' )",       999,    Number( '\t' +'999') );
    array[item++] = new TestCase( SECTION,  "Number( '\\f'  + '999' )",       999,    Number( '\f' +'999') );

    array[item++] = new TestCase( SECTION,  "Number( '999' + '   ' )",        999,    Number( '999'+'   ') );
    array[item++] = new TestCase( SECTION,  "Number( '999' + '\\n' )",        999,    Number( '999'+'\n' ) );
    array[item++] = new TestCase( SECTION,  "Number( '999' + '\\r' )",        999,    Number( '999'+'\r' ) );
    array[item++] = new TestCase( SECTION,  "Number( '999' + '\\t' )",        999,    Number( '999'+'\t' ) );
    array[item++] = new TestCase( SECTION,  "Number( '999' + '\\f' )",        999,    Number( '999'+'\f' ) );

    array[item++] = new TestCase( SECTION,  "Number( '\\n'  + '999' + '\\n' )",         999,    Number( '\n' +'999'+'\n' ) );
    array[item++] = new TestCase( SECTION,  "Number( '\\r'  + '999' + '\\r' )",         999,    Number( '\r' +'999'+'\r' ) );
    array[item++] = new TestCase( SECTION,  "Number( '\\t'  + '999' + '\\t' )",         999,    Number( '\t' +'999'+'\t' ) );
    array[item++] = new TestCase( SECTION,  "Number( '\\f'  + '999' + '\\f' )",         999,    Number( '\f' +'999'+'\f' ) );

    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0009) +  '99' )",    99,     Number( String.fromCharCode(0x0009) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0020) +  '99' )",    99,     Number( String.fromCharCode(0x0020) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000C) +  '99' )",    99,     Number( String.fromCharCode(0x000C) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000B) +  '99' )",    99,     Number( String.fromCharCode(0x000B) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000D) +  '99' )",    99,     Number( String.fromCharCode(0x000D) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000A) +  '99' )",    99,     Number( String.fromCharCode(0x000A) +  '99' ) );

    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x0009)",    99,     Number( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0020) +  '99' + String.fromCharCode(0x0020)",    99,     Number( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000C) +  '99' + String.fromCharCode(0x000C)",    99,     Number( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000D) +  '99' + String.fromCharCode(0x000D)",    99,     Number( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000B) +  '99' + String.fromCharCode(0x000B)",    99,     Number( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000A) +  '99' + String.fromCharCode(0x000A)",    99,     Number( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000A)) );

    array[item++] = new TestCase( SECTION,  "Number( '99' + String.fromCharCode(0x0009)",    99,     Number( '99' + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "Number( '99' + String.fromCharCode(0x0020)",    99,     Number( '99' + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "Number( '99' + String.fromCharCode(0x000C)",    99,     Number( '99' + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "Number( '99' + String.fromCharCode(0x000D)",    99,     Number( '99' + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "Number( '99' + String.fromCharCode(0x000B)",    99,     Number( '99' + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "Number( '99' + String.fromCharCode(0x000A)",    99,     Number( '99' + String.fromCharCode(0x000A)) );

    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0009) +  99 )",    99,     Number( String.fromCharCode(0x0009) +  99 ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0020) +  99 )",    99,     Number( String.fromCharCode(0x0020) +  99 ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000C) +  99 )",    99,     Number( String.fromCharCode(0x000C) +  99 ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000B) +  99 )",    99,     Number( String.fromCharCode(0x000B) +  99 ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000D) +  99 )",    99,     Number( String.fromCharCode(0x000D) +  99 ) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000A) +  99 )",    99,     Number( String.fromCharCode(0x000A) +  99 ) );

    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x0009)",    99,     Number( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x0020) +  99 + String.fromCharCode(0x0020)",    99,     Number( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000C) +  99 + String.fromCharCode(0x000C)",    99,     Number( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000D) +  99 + String.fromCharCode(0x000D)",    99,     Number( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000B) +  99 + String.fromCharCode(0x000B)",    99,     Number( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "Number( String.fromCharCode(0x000A) +  99 + String.fromCharCode(0x000A)",    99,     Number( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000A)) );

    array[item++] = new TestCase( SECTION,  "Number( 99 + String.fromCharCode(0x0009)",    99,     Number( 99 + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "Number( 99 + String.fromCharCode(0x0020)",    99,     Number( 99 + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "Number( 99 + String.fromCharCode(0x000C)",    99,     Number( 99 + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "Number( 99 + String.fromCharCode(0x000D)",    99,     Number( 99 + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "Number( 99 + String.fromCharCode(0x000B)",    99,     Number( 99 + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "Number( 99 + String.fromCharCode(0x000A)",    99,     Number( 99 + String.fromCharCode(0x000A)) );


    // StrNumericLiteral:::StrDecimalLiteral:::Infinity

    array[item++] = new TestCase( SECTION,  "Number('Infinity')",   Math.pow(10,10000),   Number("Infinity") );
    array[item++] = new TestCase( SECTION,  "Number('-Infinity')", -Math.pow(10,10000),   Number("-Infinity") );
    array[item++] = new TestCase( SECTION,  "Number('+Infinity')",  Math.pow(10,10000),   Number("+Infinity") );

    // StrNumericLiteral:::   StrDecimalLiteral ::: DecimalDigits . DecimalDigits opt ExponentPart opt

    array[item++] = new TestCase( SECTION,  "Number('0')",          0,          Number("0") );
    array[item++] = new TestCase( SECTION,  "Number('-0')",         -0,         Number("-0") );
    array[item++] = new TestCase( SECTION,  "Number('+0')",          0,         Number("+0") );

    array[item++] = new TestCase( SECTION,  "Number('1')",          1,          Number("1") );
    array[item++] = new TestCase( SECTION,  "Number('-1')",         -1,         Number("-1") );
    array[item++] = new TestCase( SECTION,  "Number('+1')",          1,         Number("+1") );

    array[item++] = new TestCase( SECTION,  "Number('2')",          2,          Number("2") );
    array[item++] = new TestCase( SECTION,  "Number('-2')",         -2,         Number("-2") );
    array[item++] = new TestCase( SECTION,  "Number('+2')",          2,         Number("+2") );

    array[item++] = new TestCase( SECTION,  "Number('3')",          3,          Number("3") );
    array[item++] = new TestCase( SECTION,  "Number('-3')",         -3,         Number("-3") );
    array[item++] = new TestCase( SECTION,  "Number('+3')",          3,         Number("+3") );

    array[item++] = new TestCase( SECTION,  "Number('4')",          4,          Number("4") );
    array[item++] = new TestCase( SECTION,  "Number('-4')",         -4,         Number("-4") );
    array[item++] = new TestCase( SECTION,  "Number('+4')",          4,         Number("+4") );

    array[item++] = new TestCase( SECTION,  "Number('5')",          5,          Number("5") );
    array[item++] = new TestCase( SECTION,  "Number('-5')",         -5,         Number("-5") );
    array[item++] = new TestCase( SECTION,  "Number('+5')",          5,         Number("+5") );

    array[item++] = new TestCase( SECTION,  "Number('6')",          6,          Number("6") );
    array[item++] = new TestCase( SECTION,  "Number('-6')",         -6,         Number("-6") );
    array[item++] = new TestCase( SECTION,  "Number('+6')",          6,         Number("+6") );

    array[item++] = new TestCase( SECTION,  "Number('7')",          7,          Number("7") );
    array[item++] = new TestCase( SECTION,  "Number('-7')",         -7,         Number("-7") );
    array[item++] = new TestCase( SECTION,  "Number('+7')",          7,         Number("+7") );

    array[item++] = new TestCase( SECTION,  "Number('8')",          8,          Number("8") );
    array[item++] = new TestCase( SECTION,  "Number('-8')",         -8,         Number("-8") );
    array[item++] = new TestCase( SECTION,  "Number('+8')",          8,         Number("+8") );

    array[item++] = new TestCase( SECTION,  "Number('9')",          9,          Number("9") );
    array[item++] = new TestCase( SECTION,  "Number('-9')",         -9,         Number("-9") );
    array[item++] = new TestCase( SECTION,  "Number('+9')",          9,         Number("+9") );

    array[item++] = new TestCase( SECTION,  "Number('3.14159')",    3.14159,    Number("3.14159") );
    array[item++] = new TestCase( SECTION,  "Number('-3.14159')",   -3.14159,   Number("-3.14159") );
    array[item++] = new TestCase( SECTION,  "Number('+3.14159')",   3.14159,    Number("+3.14159") );

    array[item++] = new TestCase( SECTION,  "Number('3.')",         3,          Number("3.") );
    array[item++] = new TestCase( SECTION,  "Number('-3.')",        -3,         Number("-3.") );
    array[item++] = new TestCase( SECTION,  "Number('+3.')",        3,          Number("+3.") );

    array[item++] = new TestCase( SECTION,  "Number('3.e1')",       30,         Number("3.e1") );
    array[item++] = new TestCase( SECTION,  "Number('-3.e1')",      -30,        Number("-3.e1") );
    array[item++] = new TestCase( SECTION,  "Number('+3.e1')",      30,         Number("+3.e1") );

    array[item++] = new TestCase( SECTION,  "Number('3.e+1')",       30,         Number("3.e+1") );
    array[item++] = new TestCase( SECTION,  "Number('-3.e+1')",      -30,        Number("-3.e+1") );
    array[item++] = new TestCase( SECTION,  "Number('+3.e+1')",      30,         Number("+3.e+1") );

    array[item++] = new TestCase( SECTION,  "Number('3.e-1')",       .30,         Number("3.e-1") );
    array[item++] = new TestCase( SECTION,  "Number('-3.e-1')",      -.30,        Number("-3.e-1") );
    array[item++] = new TestCase( SECTION,  "Number('+3.e-1')",      .30,         Number("+3.e-1") );

    // StrDecimalLiteral:::  .DecimalDigits ExponentPart opt

    array[item++] = new TestCase( SECTION,  "Number('.00001')",     0.00001,    Number(".00001") );
    array[item++] = new TestCase( SECTION,  "Number('+.00001')",    0.00001,    Number("+.00001") );
    array[item++] = new TestCase( SECTION,  "Number('-0.0001')",    -0.00001,   Number("-.00001") );

    array[item++] = new TestCase( SECTION,  "Number('.01e2')",      1,          Number(".01e2") );
    array[item++] = new TestCase( SECTION,  "Number('+.01e2')",     1,          Number("+.01e2") );
    array[item++] = new TestCase( SECTION,  "Number('-.01e2')",     -1,         Number("-.01e2") );

    array[item++] = new TestCase( SECTION,  "Number('.01e+2')",      1,         Number(".01e+2") );
    array[item++] = new TestCase( SECTION,  "Number('+.01e+2')",     1,         Number("+.01e+2") );
    array[item++] = new TestCase( SECTION,  "Number('-.01e+2')",     -1,        Number("-.01e+2") );

    array[item++] = new TestCase( SECTION,  "Number('.01e-2')",      0.0001,    Number(".01e-2") );
    array[item++] = new TestCase( SECTION,  "Number('+.01e-2')",     0.0001,    Number("+.01e-2") );
    array[item++] = new TestCase( SECTION,  "Number('-.01e-2')",     -0.0001,   Number("-.01e-2") );

    //  StrDecimalLiteral:::    DecimalDigits ExponentPart opt

    array[item++] = new TestCase( SECTION,  "Number('1234e5')",     123400000,  Number("1234e5") );
    array[item++] = new TestCase( SECTION,  "Number('+1234e5')",    123400000,  Number("+1234e5") );
    array[item++] = new TestCase( SECTION,  "Number('-1234e5')",    -123400000, Number("-1234e5") );

    array[item++] = new TestCase( SECTION,  "Number('1234e+5')",    123400000,  Number("1234e+5") );
    array[item++] = new TestCase( SECTION,  "Number('+1234e+5')",   123400000,  Number("+1234e+5") );
    array[item++] = new TestCase( SECTION,  "Number('-1234e+5')",   -123400000, Number("-1234e+5") );

    array[item++] = new TestCase( SECTION,  "Number('1234e-5')",     0.01234,  Number("1234e-5") );
    array[item++] = new TestCase( SECTION,  "Number('+1234e-5')",    0.01234,  Number("+1234e-5") );
    array[item++] = new TestCase( SECTION,  "Number('-1234e-5')",    -0.01234, Number("-1234e-5") );

    // StrNumericLiteral::: HexIntegerLiteral

    array[item++] = new TestCase( SECTION,  "Number('0x0')",        0,          Number("0x0"));
    array[item++] = new TestCase( SECTION,  "Number('0x1')",        1,          Number("0x1"));
    array[item++] = new TestCase( SECTION,  "Number('0x2')",        2,          Number("0x2"));
    array[item++] = new TestCase( SECTION,  "Number('0x3')",        3,          Number("0x3"));
    array[item++] = new TestCase( SECTION,  "Number('0x4')",        4,          Number("0x4"));
    array[item++] = new TestCase( SECTION,  "Number('0x5')",        5,          Number("0x5"));
    array[item++] = new TestCase( SECTION,  "Number('0x6')",        6,          Number("0x6"));
    array[item++] = new TestCase( SECTION,  "Number('0x7')",        7,          Number("0x7"));
    array[item++] = new TestCase( SECTION,  "Number('0x8')",        8,          Number("0x8"));
    array[item++] = new TestCase( SECTION,  "Number('0x9')",        9,          Number("0x9"));
    array[item++] = new TestCase( SECTION,  "Number('0xa')",        10,         Number("0xa"));
    array[item++] = new TestCase( SECTION,  "Number('0xb')",        11,         Number("0xb"));
    array[item++] = new TestCase( SECTION,  "Number('0xc')",        12,         Number("0xc"));
    array[item++] = new TestCase( SECTION,  "Number('0xd')",        13,         Number("0xd"));
    array[item++] = new TestCase( SECTION,  "Number('0xe')",        14,         Number("0xe"));
    array[item++] = new TestCase( SECTION,  "Number('0xf')",        15,         Number("0xf"));
    array[item++] = new TestCase( SECTION,  "Number('0xA')",        10,         Number("0xA"));
    array[item++] = new TestCase( SECTION,  "Number('0xB')",        11,         Number("0xB"));
    array[item++] = new TestCase( SECTION,  "Number('0xC')",        12,         Number("0xC"));
    array[item++] = new TestCase( SECTION,  "Number('0xD')",        13,         Number("0xD"));
    array[item++] = new TestCase( SECTION,  "Number('0xE')",        14,         Number("0xE"));
    array[item++] = new TestCase( SECTION,  "Number('0xF')",        15,         Number("0xF"));

    array[item++] = new TestCase( SECTION,  "Number('0X0')",        0,          Number("0X0"));
    array[item++] = new TestCase( SECTION,  "Number('0X1')",        1,          Number("0X1"));
    array[item++] = new TestCase( SECTION,  "Number('0X2')",        2,          Number("0X2"));
    array[item++] = new TestCase( SECTION,  "Number('0X3')",        3,          Number("0X3"));
    array[item++] = new TestCase( SECTION,  "Number('0X4')",        4,          Number("0X4"));
    array[item++] = new TestCase( SECTION,  "Number('0X5')",        5,          Number("0X5"));
    array[item++] = new TestCase( SECTION,  "Number('0X6')",        6,          Number("0X6"));
    array[item++] = new TestCase( SECTION,  "Number('0X7')",        7,          Number("0X7"));
    array[item++] = new TestCase( SECTION,  "Number('0X8')",        8,          Number("0X8"));
    array[item++] = new TestCase( SECTION,  "Number('0X9')",        9,          Number("0X9"));
    array[item++] = new TestCase( SECTION,  "Number('0Xa')",        10,         Number("0Xa"));
    array[item++] = new TestCase( SECTION,  "Number('0Xb')",        11,         Number("0Xb"));
    array[item++] = new TestCase( SECTION,  "Number('0Xc')",        12,         Number("0Xc"));
    array[item++] = new TestCase( SECTION,  "Number('0Xd')",        13,         Number("0Xd"));
    array[item++] = new TestCase( SECTION,  "Number('0Xe')",        14,         Number("0Xe"));
    array[item++] = new TestCase( SECTION,  "Number('0Xf')",        15,         Number("0Xf"));
    array[item++] = new TestCase( SECTION,  "Number('0XA')",        10,         Number("0XA"));
    array[item++] = new TestCase( SECTION,  "Number('0XB')",        11,         Number("0XB"));
    array[item++] = new TestCase( SECTION,  "Number('0XC')",        12,         Number("0XC"));
    array[item++] = new TestCase( SECTION,  "Number('0XD')",        13,         Number("0XD"));
    array[item++] = new TestCase( SECTION,  "Number('0XE')",        14,         Number("0XE"));
    array[item++] = new TestCase( SECTION,  "Number('0XF')",        15,         Number("0XF"));

    return ( array );
}

