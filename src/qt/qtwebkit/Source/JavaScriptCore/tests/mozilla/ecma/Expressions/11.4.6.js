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
    File Name:          11.4.6.js
    ECMA Section:       11.4.6 Unary + Operator
    Description:        convert operand to Number type
    Author:             christine@netscape.com
    Date:               7 july 1997
*/

    var SECTION = "11.4.6";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();
    var BUGNUMBER="77391";

    writeHeaderToLog( SECTION + " Unary + operator");
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
    array[item++] = new TestCase( SECTION,  "+('')",           0,      +("") );
    array[item++] = new TestCase( SECTION,  "+(' ')",          0,      +(" ") );
    array[item++] = new TestCase( SECTION,  "+(\\t)",          0,      +("\t") );
    array[item++] = new TestCase( SECTION,  "+(\\n)",          0,      +("\n") );
    array[item++] = new TestCase( SECTION,  "+(\\r)",          0,      +("\r") );
    array[item++] = new TestCase( SECTION,  "+(\\f)",          0,      +("\f") );

    array[item++] = new TestCase( SECTION,  "+(String.fromCharCode(0x0009)",   0,  +(String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "+(String.fromCharCode(0x0020)",   0,  +(String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "+(String.fromCharCode(0x000C)",   0,  +(String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "+(String.fromCharCode(0x000B)",   0,  +(String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "+(String.fromCharCode(0x000D)",   0,  +(String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "+(String.fromCharCode(0x000A)",   0,  +(String.fromCharCode(0x000A)) );

    //  a StringNumericLiteral may be preceeded or followed by whitespace and/or
    //  line terminators

    array[item++] = new TestCase( SECTION,  "+( '   ' +  999 )",        999,    +( '   '+999) );
    array[item++] = new TestCase( SECTION,  "+( '\\n'  + 999 )",       999,    +( '\n' +999) );
    array[item++] = new TestCase( SECTION,  "+( '\\r'  + 999 )",       999,    +( '\r' +999) );
    array[item++] = new TestCase( SECTION,  "+( '\\t'  + 999 )",       999,    +( '\t' +999) );
    array[item++] = new TestCase( SECTION,  "+( '\\f'  + 999 )",       999,    +( '\f' +999) );

    array[item++] = new TestCase( SECTION,  "+( 999 + '   ' )",        999,    +( 999+'   ') );
    array[item++] = new TestCase( SECTION,  "+( 999 + '\\n' )",        999,    +( 999+'\n' ) );
    array[item++] = new TestCase( SECTION,  "+( 999 + '\\r' )",        999,    +( 999+'\r' ) );
    array[item++] = new TestCase( SECTION,  "+( 999 + '\\t' )",        999,    +( 999+'\t' ) );
    array[item++] = new TestCase( SECTION,  "+( 999 + '\\f' )",        999,    +( 999+'\f' ) );

    array[item++] = new TestCase( SECTION,  "+( '\\n'  + 999 + '\\n' )",         999,    +( '\n' +999+'\n' ) );
    array[item++] = new TestCase( SECTION,  "+( '\\r'  + 999 + '\\r' )",         999,    +( '\r' +999+'\r' ) );
    array[item++] = new TestCase( SECTION,  "+( '\\t'  + 999 + '\\t' )",         999,    +( '\t' +999+'\t' ) );
    array[item++] = new TestCase( SECTION,  "+( '\\f'  + 999 + '\\f' )",         999,    +( '\f' +999+'\f' ) );

    array[item++] = new TestCase( SECTION,  "+( '   ' +  '999' )",     999,    +( '   '+'999') );
    array[item++] = new TestCase( SECTION,  "+( '\\n'  + '999' )",       999,    +( '\n' +'999') );
    array[item++] = new TestCase( SECTION,  "+( '\\r'  + '999' )",       999,    +( '\r' +'999') );
    array[item++] = new TestCase( SECTION,  "+( '\\t'  + '999' )",       999,    +( '\t' +'999') );
    array[item++] = new TestCase( SECTION,  "+( '\\f'  + '999' )",       999,    +( '\f' +'999') );

    array[item++] = new TestCase( SECTION,  "+( '999' + '   ' )",        999,    +( '999'+'   ') );
    array[item++] = new TestCase( SECTION,  "+( '999' + '\\n' )",        999,    +( '999'+'\n' ) );
    array[item++] = new TestCase( SECTION,  "+( '999' + '\\r' )",        999,    +( '999'+'\r' ) );
    array[item++] = new TestCase( SECTION,  "+( '999' + '\\t' )",        999,    +( '999'+'\t' ) );
    array[item++] = new TestCase( SECTION,  "+( '999' + '\\f' )",        999,    +( '999'+'\f' ) );

    array[item++] = new TestCase( SECTION,  "+( '\\n'  + '999' + '\\n' )",         999,    +( '\n' +'999'+'\n' ) );
    array[item++] = new TestCase( SECTION,  "+( '\\r'  + '999' + '\\r' )",         999,    +( '\r' +'999'+'\r' ) );
    array[item++] = new TestCase( SECTION,  "+( '\\t'  + '999' + '\\t' )",         999,    +( '\t' +'999'+'\t' ) );
    array[item++] = new TestCase( SECTION,  "+( '\\f'  + '999' + '\\f' )",         999,    +( '\f' +'999'+'\f' ) );

    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0009) +  '99' )",    99,     +( String.fromCharCode(0x0009) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0020) +  '99' )",    99,     +( String.fromCharCode(0x0020) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000C) +  '99' )",    99,     +( String.fromCharCode(0x000C) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000B) +  '99' )",    99,     +( String.fromCharCode(0x000B) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000D) +  '99' )",    99,     +( String.fromCharCode(0x000D) +  '99' ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000A) +  '99' )",    99,     +( String.fromCharCode(0x000A) +  '99' ) );

    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x0009)",    99,     +( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0020) +  '99' + String.fromCharCode(0x0020)",    99,     +( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000C) +  '99' + String.fromCharCode(0x000C)",    99,     +( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000D) +  '99' + String.fromCharCode(0x000D)",    99,     +( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000B) +  '99' + String.fromCharCode(0x000B)",    99,     +( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000A) +  '99' + String.fromCharCode(0x000A)",    99,     +( String.fromCharCode(0x0009) +  '99' + String.fromCharCode(0x000A)) );

    array[item++] = new TestCase( SECTION,  "+( '99' + String.fromCharCode(0x0009)",    99,     +( '99' + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "+( '99' + String.fromCharCode(0x0020)",    99,     +( '99' + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "+( '99' + String.fromCharCode(0x000C)",    99,     +( '99' + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "+( '99' + String.fromCharCode(0x000D)",    99,     +( '99' + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "+( '99' + String.fromCharCode(0x000B)",    99,     +( '99' + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "+( '99' + String.fromCharCode(0x000A)",    99,     +( '99' + String.fromCharCode(0x000A)) );

    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0009) +  99 )",    99,     +( String.fromCharCode(0x0009) +  99 ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0020) +  99 )",    99,     +( String.fromCharCode(0x0020) +  99 ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000C) +  99 )",    99,     +( String.fromCharCode(0x000C) +  99 ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000B) +  99 )",    99,     +( String.fromCharCode(0x000B) +  99 ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000D) +  99 )",    99,     +( String.fromCharCode(0x000D) +  99 ) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000A) +  99 )",    99,     +( String.fromCharCode(0x000A) +  99 ) );

    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x0009)",    99,     +( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x0020) +  99 + String.fromCharCode(0x0020)",    99,     +( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000C) +  99 + String.fromCharCode(0x000C)",    99,     +( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000D) +  99 + String.fromCharCode(0x000D)",    99,     +( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000B) +  99 + String.fromCharCode(0x000B)",    99,     +( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "+( String.fromCharCode(0x000A) +  99 + String.fromCharCode(0x000A)",    99,     +( String.fromCharCode(0x0009) +  99 + String.fromCharCode(0x000A)) );

    array[item++] = new TestCase( SECTION,  "+( 99 + String.fromCharCode(0x0009)",    99,     +( 99 + String.fromCharCode(0x0009)) );
    array[item++] = new TestCase( SECTION,  "+( 99 + String.fromCharCode(0x0020)",    99,     +( 99 + String.fromCharCode(0x0020)) );
    array[item++] = new TestCase( SECTION,  "+( 99 + String.fromCharCode(0x000C)",    99,     +( 99 + String.fromCharCode(0x000C)) );
    array[item++] = new TestCase( SECTION,  "+( 99 + String.fromCharCode(0x000D)",    99,     +( 99 + String.fromCharCode(0x000D)) );
    array[item++] = new TestCase( SECTION,  "+( 99 + String.fromCharCode(0x000B)",    99,     +( 99 + String.fromCharCode(0x000B)) );
    array[item++] = new TestCase( SECTION,  "+( 99 + String.fromCharCode(0x000A)",    99,     +( 99 + String.fromCharCode(0x000A)) );


    // StrNumericLiteral:::StrDecimalLiteral:::Infinity

    array[item++] = new TestCase( SECTION,  "+('Infinity')",   Math.pow(10,10000),   +("Infinity") );
    array[item++] = new TestCase( SECTION,  "+('-Infinity')", -Math.pow(10,10000),   +("-Infinity") );
    array[item++] = new TestCase( SECTION,  "+('+Infinity')",  Math.pow(10,10000),   +("+Infinity") );

    // StrNumericLiteral:::   StrDecimalLiteral ::: DecimalDigits . DecimalDigits opt ExponentPart opt

    array[item++] = new TestCase( SECTION,  "+('0')",          0,          +("0") );
    array[item++] = new TestCase( SECTION,  "+('-0')",         -0,         +("-0") );
    array[item++] = new TestCase( SECTION,  "+('+0')",          0,         +("+0") );

    array[item++] = new TestCase( SECTION,  "+('1')",          1,          +("1") );
    array[item++] = new TestCase( SECTION,  "+('-1')",         -1,         +("-1") );
    array[item++] = new TestCase( SECTION,  "+('+1')",          1,         +("+1") );

    array[item++] = new TestCase( SECTION,  "+('2')",          2,          +("2") );
    array[item++] = new TestCase( SECTION,  "+('-2')",         -2,         +("-2") );
    array[item++] = new TestCase( SECTION,  "+('+2')",          2,         +("+2") );

    array[item++] = new TestCase( SECTION,  "+('3')",          3,          +("3") );
    array[item++] = new TestCase( SECTION,  "+('-3')",         -3,         +("-3") );
    array[item++] = new TestCase( SECTION,  "+('+3')",          3,         +("+3") );

    array[item++] = new TestCase( SECTION,  "+('4')",          4,          +("4") );
    array[item++] = new TestCase( SECTION,  "+('-4')",         -4,         +("-4") );
    array[item++] = new TestCase( SECTION,  "+('+4')",          4,         +("+4") );

    array[item++] = new TestCase( SECTION,  "+('5')",          5,          +("5") );
    array[item++] = new TestCase( SECTION,  "+('-5')",         -5,         +("-5") );
    array[item++] = new TestCase( SECTION,  "+('+5')",          5,         +("+5") );

    array[item++] = new TestCase( SECTION,  "+('6')",          6,          +("6") );
    array[item++] = new TestCase( SECTION,  "+('-6')",         -6,         +("-6") );
    array[item++] = new TestCase( SECTION,  "+('+6')",          6,         +("+6") );

    array[item++] = new TestCase( SECTION,  "+('7')",          7,          +("7") );
    array[item++] = new TestCase( SECTION,  "+('-7')",         -7,         +("-7") );
    array[item++] = new TestCase( SECTION,  "+('+7')",          7,         +("+7") );

    array[item++] = new TestCase( SECTION,  "+('8')",          8,          +("8") );
    array[item++] = new TestCase( SECTION,  "+('-8')",         -8,         +("-8") );
    array[item++] = new TestCase( SECTION,  "+('+8')",          8,         +("+8") );

    array[item++] = new TestCase( SECTION,  "+('9')",          9,          +("9") );
    array[item++] = new TestCase( SECTION,  "+('-9')",         -9,         +("-9") );
    array[item++] = new TestCase( SECTION,  "+('+9')",          9,         +("+9") );

    array[item++] = new TestCase( SECTION,  "+('3.14159')",    3.14159,    +("3.14159") );
    array[item++] = new TestCase( SECTION,  "+('-3.14159')",   -3.14159,   +("-3.14159") );
    array[item++] = new TestCase( SECTION,  "+('+3.14159')",   3.14159,    +("+3.14159") );

    array[item++] = new TestCase( SECTION,  "+('3.')",         3,          +("3.") );
    array[item++] = new TestCase( SECTION,  "+('-3.')",        -3,         +("-3.") );
    array[item++] = new TestCase( SECTION,  "+('+3.')",        3,          +("+3.") );

    array[item++] = new TestCase( SECTION,  "+('3.e1')",       30,         +("3.e1") );
    array[item++] = new TestCase( SECTION,  "+('-3.e1')",      -30,        +("-3.e1") );
    array[item++] = new TestCase( SECTION,  "+('+3.e1')",      30,         +("+3.e1") );

    array[item++] = new TestCase( SECTION,  "+('3.e+1')",       30,         +("3.e+1") );
    array[item++] = new TestCase( SECTION,  "+('-3.e+1')",      -30,        +("-3.e+1") );
    array[item++] = new TestCase( SECTION,  "+('+3.e+1')",      30,         +("+3.e+1") );

    array[item++] = new TestCase( SECTION,  "+('3.e-1')",       .30,         +("3.e-1") );
    array[item++] = new TestCase( SECTION,  "+('-3.e-1')",      -.30,        +("-3.e-1") );
    array[item++] = new TestCase( SECTION,  "+('+3.e-1')",      .30,         +("+3.e-1") );

    // StrDecimalLiteral:::  .DecimalDigits ExponentPart opt

    array[item++] = new TestCase( SECTION,  "+('.00001')",     0.00001,    +(".00001") );
    array[item++] = new TestCase( SECTION,  "+('+.00001')",    0.00001,    +("+.00001") );
    array[item++] = new TestCase( SECTION,  "+('-0.0001')",    -0.00001,   +("-.00001") );

    array[item++] = new TestCase( SECTION,  "+('.01e2')",      1,          +(".01e2") );
    array[item++] = new TestCase( SECTION,  "+('+.01e2')",     1,          +("+.01e2") );
    array[item++] = new TestCase( SECTION,  "+('-.01e2')",     -1,         +("-.01e2") );

    array[item++] = new TestCase( SECTION,  "+('.01e+2')",      1,         +(".01e+2") );
    array[item++] = new TestCase( SECTION,  "+('+.01e+2')",     1,         +("+.01e+2") );
    array[item++] = new TestCase( SECTION,  "+('-.01e+2')",     -1,        +("-.01e+2") );

    array[item++] = new TestCase( SECTION,  "+('.01e-2')",      0.0001,    +(".01e-2") );
    array[item++] = new TestCase( SECTION,  "+('+.01e-2')",     0.0001,    +("+.01e-2") );
    array[item++] = new TestCase( SECTION,  "+('-.01e-2')",     -0.0001,   +("-.01e-2") );

    //  StrDecimalLiteral:::    DecimalDigits ExponentPart opt

    array[item++] = new TestCase( SECTION,  "+('1234e5')",     123400000,  +("1234e5") );
    array[item++] = new TestCase( SECTION,  "+('+1234e5')",    123400000,  +("+1234e5") );
    array[item++] = new TestCase( SECTION,  "+('-1234e5')",    -123400000, +("-1234e5") );

    array[item++] = new TestCase( SECTION,  "+('1234e+5')",    123400000,  +("1234e+5") );
    array[item++] = new TestCase( SECTION,  "+('+1234e+5')",   123400000,  +("+1234e+5") );
    array[item++] = new TestCase( SECTION,  "+('-1234e+5')",   -123400000, +("-1234e+5") );

    array[item++] = new TestCase( SECTION,  "+('1234e-5')",     0.01234,  +("1234e-5") );
    array[item++] = new TestCase( SECTION,  "+('+1234e-5')",    0.01234,  +("+1234e-5") );
    array[item++] = new TestCase( SECTION,  "+('-1234e-5')",    -0.01234, +("-1234e-5") );

    // StrNumericLiteral::: HexIntegerLiteral

    array[item++] = new TestCase( SECTION,  "+('0x0')",        0,          +("0x0"));
    array[item++] = new TestCase( SECTION,  "+('0x1')",        1,          +("0x1"));
    array[item++] = new TestCase( SECTION,  "+('0x2')",        2,          +("0x2"));
    array[item++] = new TestCase( SECTION,  "+('0x3')",        3,          +("0x3"));
    array[item++] = new TestCase( SECTION,  "+('0x4')",        4,          +("0x4"));
    array[item++] = new TestCase( SECTION,  "+('0x5')",        5,          +("0x5"));
    array[item++] = new TestCase( SECTION,  "+('0x6')",        6,          +("0x6"));
    array[item++] = new TestCase( SECTION,  "+('0x7')",        7,          +("0x7"));
    array[item++] = new TestCase( SECTION,  "+('0x8')",        8,          +("0x8"));
    array[item++] = new TestCase( SECTION,  "+('0x9')",        9,          +("0x9"));
    array[item++] = new TestCase( SECTION,  "+('0xa')",        10,         +("0xa"));
    array[item++] = new TestCase( SECTION,  "+('0xb')",        11,         +("0xb"));
    array[item++] = new TestCase( SECTION,  "+('0xc')",        12,         +("0xc"));
    array[item++] = new TestCase( SECTION,  "+('0xd')",        13,         +("0xd"));
    array[item++] = new TestCase( SECTION,  "+('0xe')",        14,         +("0xe"));
    array[item++] = new TestCase( SECTION,  "+('0xf')",        15,         +("0xf"));
    array[item++] = new TestCase( SECTION,  "+('0xA')",        10,         +("0xA"));
    array[item++] = new TestCase( SECTION,  "+('0xB')",        11,         +("0xB"));
    array[item++] = new TestCase( SECTION,  "+('0xC')",        12,         +("0xC"));
    array[item++] = new TestCase( SECTION,  "+('0xD')",        13,         +("0xD"));
    array[item++] = new TestCase( SECTION,  "+('0xE')",        14,         +("0xE"));
    array[item++] = new TestCase( SECTION,  "+('0xF')",        15,         +("0xF"));

    array[item++] = new TestCase( SECTION,  "+('0X0')",        0,          +("0X0"));
    array[item++] = new TestCase( SECTION,  "+('0X1')",        1,          +("0X1"));
    array[item++] = new TestCase( SECTION,  "+('0X2')",        2,          +("0X2"));
    array[item++] = new TestCase( SECTION,  "+('0X3')",        3,          +("0X3"));
    array[item++] = new TestCase( SECTION,  "+('0X4')",        4,          +("0X4"));
    array[item++] = new TestCase( SECTION,  "+('0X5')",        5,          +("0X5"));
    array[item++] = new TestCase( SECTION,  "+('0X6')",        6,          +("0X6"));
    array[item++] = new TestCase( SECTION,  "+('0X7')",        7,          +("0X7"));
    array[item++] = new TestCase( SECTION,  "+('0X8')",        8,          +("0X8"));
    array[item++] = new TestCase( SECTION,  "+('0X9')",        9,          +("0X9"));
    array[item++] = new TestCase( SECTION,  "+('0Xa')",        10,         +("0Xa"));
    array[item++] = new TestCase( SECTION,  "+('0Xb')",        11,         +("0Xb"));
    array[item++] = new TestCase( SECTION,  "+('0Xc')",        12,         +("0Xc"));
    array[item++] = new TestCase( SECTION,  "+('0Xd')",        13,         +("0Xd"));
    array[item++] = new TestCase( SECTION,  "+('0Xe')",        14,         +("0Xe"));
    array[item++] = new TestCase( SECTION,  "+('0Xf')",        15,         +("0Xf"));
    array[item++] = new TestCase( SECTION,  "+('0XA')",        10,         +("0XA"));
    array[item++] = new TestCase( SECTION,  "+('0XB')",        11,         +("0XB"));
    array[item++] = new TestCase( SECTION,  "+('0XC')",        12,         +("0XC"));
    array[item++] = new TestCase( SECTION,  "+('0XD')",        13,         +("0XD"));
    array[item++] = new TestCase( SECTION,  "+('0XE')",        14,         +("0XE"));
    array[item++] = new TestCase( SECTION,  "+('0XF')",        15,         +("0XF"));

    return array;

}