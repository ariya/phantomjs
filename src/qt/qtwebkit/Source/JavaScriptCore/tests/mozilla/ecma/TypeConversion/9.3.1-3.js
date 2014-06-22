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
    File Name:          9.3.1-3.js
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


                        Test cases provided by waldemar.


    Author:             christine@netscape.com
    Date:               10 june 1998

*/

var SECTION = "9.3.1-3";
var VERSION = "ECMA_1";
    startTest();
var BUGNUMBER="129087";

var TITLE   = "Number To String, String To Number";

writeHeaderToLog( SECTION + " "+ TITLE);

var testcases = new Array();


// test case from http://scopus.mcom.com/bugsplat/show_bug.cgi?id=312954
var z = 0;

testcases[tc++] = new TestCase(
    SECTION,
    "var z = 0; print(1/-z)",
    -Infinity,
    1/-z );





// test cases from bug http://scopus.mcom.com/bugsplat/show_bug.cgi?id=122882



testcases[tc++] = new TestCase( SECTION,
    '- -"0x80000000"',
    2147483648,
    - -"0x80000000" );

testcases[tc++] = new TestCase( SECTION,
    '- -"0x100000000"',
    4294967296,
    - -"0x100000000" );

testcases[tc++] = new TestCase( SECTION,
    '- "-0x123456789abcde8"',
    NaN,
    - "-0x123456789abcde8" );

// Convert some large numbers to string


testcases[tc++] = new TestCase( SECTION,
                                "1e2000 +''",
                                "Infinity",
                                1e2000 +"" );

testcases[tc++] = new TestCase( SECTION,
                                "1e2000",
                                Infinity,
                                1e2000 );

testcases[tc++] = new TestCase( SECTION,
                                "-1e2000 +''",
                                "-Infinity",
                                -1e2000 +"" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"1e2000\"",
                                -Infinity,
                                -"1e2000" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"-1e2000\" +''",
                                "Infinity",
                                -"-1e2000" +"" );

testcases[tc++] = new TestCase( SECTION,
                                "1e-2000",
                                0,
                                1e-2000 );

testcases[tc++] = new TestCase( SECTION,
                                "1/1e-2000",
                                Infinity,
                                1/1e-2000 );

// convert some strings to large numbers

testcases[tc++] = new TestCase( SECTION,
                                "1/-1e-2000",
                                -Infinity,
                                1/-1e-2000 );

testcases[tc++] = new TestCase( SECTION,
                                "1/\"1e-2000\"",
                                Infinity,
                                1/"1e-2000" );

testcases[tc++] = new TestCase( SECTION,
                                "1/\"-1e-2000\"",
                                -Infinity,
                                1/"-1e-2000" );

testcases[tc++] = new TestCase( SECTION,
                                "parseFloat(\"1e2000\")",
                                Infinity,
                                parseFloat("1e2000") );

testcases[tc++] = new TestCase( SECTION,
                                "parseFloat(\"1e-2000\")",
                                0,
                                parseFloat("1e-2000") );

testcases[tc++] = new TestCase( SECTION,
                                "1.7976931348623157E+308",
                                1.7976931348623157e+308,
                                1.7976931348623157E+308 );

testcases[tc++] = new TestCase( SECTION,
                                "1.7976931348623158e+308",
                                1.7976931348623157e+308,
                                1.7976931348623158e+308 );

testcases[tc++] = new TestCase( SECTION,
                                "1.7976931348623159e+308",
                                Infinity,
                                1.7976931348623159e+308 );

s =
"17976931348623158079372897140530341507993413271003782693617377898044496829276475094664901797758720709633028641669288791094655554785194040263065748867150582068";

testcases[tc++] = new TestCase( SECTION,
                                "s = " + s +"; s +="+
"\"190890200070838367627385484581771153176447573027006985557136695962284291481986083493647529271907416844436551070434271155969950809304288017790417449779\""+

+"; s",
"17976931348623158079372897140530341507993413271003782693617377898044496829276475094664901797758720709633028641669288791094655554785194040263065748867150582068190890200070838367627385484581771153176447573027006985557136695962284291481986083493647529271907416844436551070434271155969950809304288017790417449779",
s +=
"190890200070838367627385484581771153176447573027006985557136695962284291481986083493647529271907416844436551070434271155969950809304288017790417449779"
);

s1 = s+1;

testcases[tc++] = new TestCase( SECTION,
"s1 = s+1; s1",
"179769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416692887910946555547851940402630657488671505820681908902000708383676273854845817711531764475730270069855571366959622842914819860834936475292719074168444365510704342711559699508093042880177904174497791",
s1 );

/***** This answer is preferred but -Infinity is also acceptable here *****/

testcases[tc++] = new TestCase( SECTION,
"-s1 == Infinity || s1 == 1.7976931348623157e+308",
true,
-s1 == Infinity || s1 == 1.7976931348623157e+308 );

s2 = s + 2;

testcases[tc++] = new TestCase( SECTION,
"s2 = s+2; s2",
"179769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416692887910946555547851940402630657488671505820681908902000708383676273854845817711531764475730270069855571366959622842914819860834936475292719074168444365510704342711559699508093042880177904174497792",
s2 );

// ***** This answer is preferred but -1.7976931348623157e+308 is also acceptable here *****
testcases[tc++] = new TestCase( SECTION,
"-s2 == -Infinity || -s2 == -1.7976931348623157e+308 ",
true,
-s2 == -Infinity || -s2 == -1.7976931348623157e+308 );

s3 = s+3;

testcases[tc++] = new TestCase( SECTION,
"s3 = s+3; s3",
"179769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416692887910946555547851940402630657488671505820681908902000708383676273854845817711531764475730270069855571366959622842914819860834936475292719074168444365510704342711559699508093042880177904174497793",
s3 );

//***** This answer is preferred but -1.7976931348623157e+308 is also acceptable here *****

testcases[tc++] = new TestCase( SECTION,
"-s3 == -Infinity || -s3 == -1.7976931348623157e+308",
true,
-s3 == -Infinity || -s3 == -1.7976931348623157e+308 );


//***** This answer is preferred but Infinity is also acceptable here *****

testcases[tc++] = new TestCase( SECTION,
"parseInt(s1,10) == 1.7976931348623157e+308 || parseInt(s1,10) == Infinity",
true,
parseInt(s1,10) == 1.7976931348623157e+308 || parseInt(s1,10) == Infinity );

//***** This answer is preferred but 1.7976931348623157e+308 is also acceptable here *****
testcases[tc++] = new TestCase( SECTION,
"parseInt(s2,10) == Infinity || parseInt(s2,10) == 1.7976931348623157e+308",
true ,
parseInt(s2,10) == Infinity || parseInt(s2,10) == 1.7976931348623157e+308 );

//***** This answer is preferred but Infinity is also acceptable here *****

testcases[tc++] = new TestCase( SECTION,
"parseInt(s1) == 1.7976931348623157e+308 || parseInt(s1) == Infinity",
true,
parseInt(s1) == 1.7976931348623157e+308 || parseInt(s1) == Infinity);

//***** This answer is preferred but 1.7976931348623157e+308 is also acceptable here *****
testcases[tc++] = new TestCase( SECTION,
"parseInt(s2) == Infinity || parseInt(s2) == 1.7976931348623157e+308",
true,
parseInt(s2) == Infinity || parseInt(s2) == 1.7976931348623157e+308 );

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

/***** These test the round-to-nearest-or-even-if-equally-close rule *****/

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

testcases[tc++] = new TestCase( SECTION,
"parseInt(\"000000100000000100100011010001010110011110001001101010111100\",2)",
9027215253084860,
parseInt("000000100000000100100011010001010110011110001001101010111100",2) );

testcases[tc++] = new TestCase( SECTION,
"parseInt(\"000000100000000100100011010001010110011110001001101010111101\",2)",
9027215253084860,
parseInt("000000100000000100100011010001010110011110001001101010111101",2) );

testcases[tc++] = new TestCase( SECTION,
"parseInt(\"000000100000000100100011010001010110011110001001101010111111\",2)",
9027215253084864,
parseInt("000000100000000100100011010001010110011110001001101010111111",2) );

testcases[tc++] = new TestCase( SECTION,
"parseInt(\"0000001000000001001000110100010101100111100010011010101111010\",2)",
18054430506169720,
parseInt("0000001000000001001000110100010101100111100010011010101111010",2));

testcases[tc++] = new TestCase( SECTION,
"parseInt(\"0000001000000001001000110100010101100111100010011010101111011\",2)",
18054430506169724,
parseInt("0000001000000001001000110100010101100111100010011010101111011",2) );

testcases[tc++] = new TestCase( SECTION,
"parseInt(\"0000001000000001001000110100010101100111100010011010101111100\",2)",
18054430506169724,
parseInt("0000001000000001001000110100010101100111100010011010101111100",2));

testcases[tc++] = new TestCase( SECTION,
"parseInt(\"0000001000000001001000110100010101100111100010011010101111110\",2)",
18054430506169728,
parseInt("0000001000000001001000110100010101100111100010011010101111110",2));

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"yz\",35)",
                                34,
                                parseInt("yz",35) );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"yz\",36)",
                                1259,
                                parseInt("yz",36) );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"yz\",37)",
                                NaN,
                                parseInt("yz",37) );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"+77\")",
                                77,
                                parseInt("+77") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"-77\",9)",
                                -70,
                                parseInt("-77",9) );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"\\u20001234\\u2000\")",
                                1234,
                                parseInt("\u20001234\u2000") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"123456789012345678\")",
                                123456789012345680,
                                parseInt("123456789012345678") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"9\",8)",
                                NaN,
                                parseInt("9",8) );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"1e2\")",
                                1,
                                parseInt("1e2") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"1.9999999999999999999\")",
                                1,
                                parseInt("1.9999999999999999999") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"0x10\")",
                                16,
                                parseInt("0x10") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"0x10\",10)",
                                0,
                                parseInt("0x10",10) );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"0022\")",
                                22,
                                parseInt("0022") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"0022\",10)",
                                22,
                                parseInt("0022",10) );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"0x1000000000000080\")",
                                1152921504606847000,
                                parseInt("0x1000000000000080") );

testcases[tc++] = new TestCase( SECTION,
                                "parseInt(\"0x1000000000000081\")",
                                1152921504606847200,
                                parseInt("0x1000000000000081") );

s =
"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

testcases[tc++] = new TestCase( SECTION, "s = "+
"\"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\";"+
"s",
"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
s );


testcases[tc++] = new TestCase( SECTION, "s +="+
"\"0000000000000000000000000000000000000\"; s",
"0xFFFFFFFFFFFFF800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
s += "0000000000000000000000000000000000000" );

testcases[tc++] = new TestCase( SECTION, "-s",
-1.7976931348623157e+308,
-s );

s =
"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

testcases[tc++] = new TestCase( SECTION, "s ="+
"\"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\";"+
"s",
"0xFFFFFFFFFFFFF80000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
s );

testcases[tc++] = new TestCase( SECTION,
"s += \"0000000000000000000000000000000000001\"",
"0xFFFFFFFFFFFFF800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
s += "0000000000000000000000000000000000001" );

testcases[tc++] = new TestCase( SECTION,
"-s",
-1.7976931348623157e+308,
-s );

s =
"0xFFFFFFFFFFFFFC0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

testcases[tc++] = new TestCase( SECTION,
"s ="+
"\"0xFFFFFFFFFFFFFC0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\";"+
"s",
"0xFFFFFFFFFFFFFC0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
s );


testcases[tc++] = new TestCase( SECTION,
"s += \"0000000000000000000000000000000000000\"",
"0xFFFFFFFFFFFFFC00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
s += "0000000000000000000000000000000000000");


testcases[tc++] = new TestCase( SECTION,
"-s",
-Infinity,
-s );

s =
"0xFFFFFFFFFFFFFB0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

testcases[tc++] = new TestCase( SECTION,
"s = "+
"\"0xFFFFFFFFFFFFFB0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\";s",
"0xFFFFFFFFFFFFFB0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
s);

testcases[tc++] = new TestCase( SECTION,
"s += \"0000000000000000000000000000000000001\"",
"0xFFFFFFFFFFFFFB00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
s += "0000000000000000000000000000000000001" );

testcases[tc++] = new TestCase( SECTION,
"-s",
-1.7976931348623157e+308,
-s );

testcases[tc++] = new TestCase( SECTION,
"s += \"0\"",
"0xFFFFFFFFFFFFFB000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010",
s += "0" );

testcases[tc++] = new TestCase( SECTION,
"-s",
-Infinity,
-s );

testcases[tc++] = new TestCase( SECTION,
"parseInt(s)",
Infinity,
parseInt(s) );

testcases[tc++] = new TestCase( SECTION,
"parseInt(s,32)",
0,
parseInt(s,32) );

testcases[tc++] = new TestCase( SECTION,
"parseInt(s,36)",
Infinity,
parseInt(s,36) );

testcases[tc++] = new TestCase( SECTION,
                                "-\"\"",
                                0,
                                -"" );

testcases[tc++] = new TestCase( SECTION,
                                "-\" \"",
                                0,
                                -" " );

testcases[tc++] = new TestCase( SECTION,
                                "-\"999\"",
                                -999,
                                -"999" );

testcases[tc++] = new TestCase( SECTION,
                                "-\" 999\"",
                                -999,
                                -" 999" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"\\t999\"",
                                -999,
                                -"\t999" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"013  \"",
                                -13,
                                -"013  " );

testcases[tc++] = new TestCase( SECTION,
                                "-\"999\\t\"",
                                -999,
                                -"999\t" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"-Infinity\"",
                                Infinity,
                                -"-Infinity" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"-infinity\"",
                                NaN,
                                -"-infinity" );


testcases[tc++] = new TestCase( SECTION,
                                "-\"+Infinity\"",
                                -Infinity,
                                -"+Infinity" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"+Infiniti\"",
                                NaN,
                                -"+Infiniti" );

testcases[tc++] = new TestCase( SECTION,
                                "- -\"0x80000000\"",
                                2147483648,
                                - -"0x80000000" );

testcases[tc++] = new TestCase( SECTION,
                                "- -\"0x100000000\"",
                                4294967296,
                                - -"0x100000000" );

testcases[tc++] = new TestCase( SECTION,
                                "- \"-0x123456789abcde8\"",
                                NaN,
                                - "-0x123456789abcde8" );

// the following two tests are not strictly ECMA 1.0

testcases[tc++] = new TestCase( SECTION,
                                "-\"\\u20001234\\u2001\"",
                                -1234,
                                -"\u20001234\u2001" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"\\u20001234\\0\"",
                                NaN,
                                -"\u20001234\0" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"0x10\"",
                                -16,
                                -"0x10" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"+\"",
                                NaN,
                                -"+" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"-\"",
                                NaN,
                                -"-" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"-0-\"",
                                NaN,
                                -"-0-" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"1e-\"",
                                NaN,
                                -"1e-" );

testcases[tc++] = new TestCase( SECTION,
                                "-\"1e-1\"",
                                -0.1,
                                -"1e-1" );

test();

function test(){
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
