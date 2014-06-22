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
    File Name:          9.8.1.js
    ECMA Section:       9.8.1 ToString Applied to the Number Type
    Description:        The operator ToString convers a number m to string
                        as follows:

                        1.  if m is NaN, return the string "NaN"
                        2.  if m is +0 or -0, return the string "0"
                        3.  if m is less than zero, return the string
                            concatenation of the string "-" and ToString(-m).
                        4.  If m is Infinity, return the string "Infinity".
                        5.  Otherwise, let n, k, and s be integers such that
                            k >= 1, 10k1 <= s < 10k, the number value for s10nk
                            is m, and k is as small as possible. Note that k is
                            the number of digits in the decimal representation
                            of s, that s is not divisible by 10, and that the
                            least significant digit of s is not necessarily
                            uniquely determined by these criteria.
                        6.  If k <= n <= 21, return the string consisting of the
                            k digits of the decimal representation of s (in order,
                            with no leading zeroes), followed by n-k occurences
                            of the character '0'.
                        7.  If 0 < n <= 21, return the string consisting of the
                            most significant n digits of the decimal
                            representation of s, followed by a decimal point
                            '.', followed by the remaining kn digits of the
                            decimal representation of s.
                        8.  If 6 < n <= 0, return the string consisting of the
                            character '0', followed by a decimal point '.',
                            followed by n occurences of the character '0',
                            followed by the k digits of the decimal
                            representation of s.
                        9.  Otherwise, if k = 1, return the string consisting
                            of the single digit of s, followed by lowercase
                            character 'e', followed by a plus sign '+' or minus
                            sign '' according to whether n1 is positive or
                            negative, followed by the decimal representation
                            of the integer abs(n1) (with no leading zeros).
                       10.  Return the string consisting of the most significant
                            digit of the decimal representation of s, followed
                            by a decimal point '.', followed by the remaining k1
                            digits of the decimal representation of s, followed
                            by the lowercase character 'e', followed by a plus
                            sign '+' or minus sign '' according to whether n1 is
                            positive or negative, followed by the decimal
                            representation of the integer abs(n1) (with no
                            leading zeros).

                    Note that if x is any number value other than 0, then
                    ToNumber(ToString(x)) is exactly the same number value as x.

                    As noted, the least significant digit of s is not always
                    uniquely determined by the requirements listed in step 5.
                    The following specification for step 5 was considered, but
                    not adopted:

    Author:         christine@netscape.com
    Date:           10 july 1997
*/

    var SECTION = "9.8.1";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " ToString applied to the Number type");
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

    array[item++] = new TestCase( SECTION,    "Number.NaN",       "NaN",                  Number.NaN + "" );
    array[item++] = new TestCase( SECTION,    "0",                "0",                    0 + "" );
    array[item++] = new TestCase( SECTION,    "-0",               "0",                   -0 + "" );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY", "Infinity",     Number.POSITIVE_INFINITY + "" );
    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY", "-Infinity",    Number.NEGATIVE_INFINITY + "" );
    array[item++] = new TestCase( SECTION,    "-1",               "-1",                   -1 + "" );

    // cases in step 6:  integers  1e21 > x >= 1 or -1 >= x > -1e21

    array[item++] = new TestCase( SECTION,    "1",                    "1",                    1 + "" );
    array[item++] = new TestCase( SECTION,    "10",                   "10",                   10 + "" );
    array[item++] = new TestCase( SECTION,    "100",                  "100",                  100 + "" );
    array[item++] = new TestCase( SECTION,    "1000",                 "1000",                 1000 + "" );
    array[item++] = new TestCase( SECTION,    "10000",                "10000",                10000 + "" );
    array[item++] = new TestCase( SECTION,    "10000000000",          "10000000000",          10000000000 + "" );
    array[item++] = new TestCase( SECTION,    "10000000000000000000", "10000000000000000000", 10000000000000000000 + "" );
    array[item++] = new TestCase( SECTION,    "100000000000000000000","100000000000000000000",100000000000000000000 + "" );

    array[item++] = new TestCase( SECTION,    "12345",                    "12345",                    12345 + "" );
    array[item++] = new TestCase( SECTION,    "1234567890",               "1234567890",               1234567890 + "" );

    array[item++] = new TestCase( SECTION,    "-1",                       "-1",                       -1 + "" );
    array[item++] = new TestCase( SECTION,    "-10",                      "-10",                      -10 + "" );
    array[item++] = new TestCase( SECTION,    "-100",                     "-100",                     -100 + "" );
    array[item++] = new TestCase( SECTION,    "-1000",                    "-1000",                    -1000 + "" );
    array[item++] = new TestCase( SECTION,    "-1000000000",              "-1000000000",              -1000000000 + "" );
    array[item++] = new TestCase( SECTION,    "-1000000000000000",        "-1000000000000000",        -1000000000000000 + "" );
    array[item++] = new TestCase( SECTION,    "-100000000000000000000",   "-100000000000000000000",   -100000000000000000000 + "" );
    array[item++] = new TestCase( SECTION,    "-1000000000000000000000",  "-1e+21",                   -1000000000000000000000 + "" );

    array[item++] = new TestCase( SECTION,    "-12345",                    "-12345",                  -12345 + "" );
    array[item++] = new TestCase( SECTION,    "-1234567890",               "-1234567890",             -1234567890 + "" );

    // cases in step 7: numbers with a fractional component, 1e21> x >1 or  -1 > x > -1e21,
    array[item++] = new TestCase( SECTION,    "1.0000001",                "1.0000001",                1.0000001 + "" );

    // cases in step 8:  fractions between 1 > x > -1, exclusive of 0 and -0

    // cases in step 9:  numbers with 1 significant digit >= 1e+21 or <= 1e-6

    array[item++] = new TestCase( SECTION,    "1000000000000000000000",   "1e+21",             1000000000000000000000 + "" );
    array[item++] = new TestCase( SECTION,    "10000000000000000000000",   "1e+22",            10000000000000000000000 + "" );

    //  cases in step 10:  numbers with more than 1 significant digit >= 1e+21 or <= 1e-6

    array[item++] = new TestCase( SECTION,    "1.2345",                    "1.2345",                  String( 1.2345));
    array[item++] = new TestCase( SECTION,    "1.234567890",               "1.23456789",             String( 1.234567890 ));


    array[item++] = new TestCase( SECTION,    ".12345",                   "0.12345",                String(.12345 )     );
    array[item++] = new TestCase( SECTION,    ".012345",                  "0.012345",               String(.012345)     );
    array[item++] = new TestCase( SECTION,    ".0012345",                 "0.0012345",              String(.0012345)    );
    array[item++] = new TestCase( SECTION,    ".00012345",                "0.00012345",             String(.00012345)   );
    array[item++] = new TestCase( SECTION,    ".000012345",               "0.000012345",            String(.000012345)  );
    array[item++] = new TestCase( SECTION,    ".0000012345",              "0.0000012345",           String(.0000012345) );
    array[item++] = new TestCase( SECTION,    ".00000012345",             "1.2345e-7",              String(.00000012345));

    array[item++] = new TestCase( SECTION,    "-1e21",                    "-1e+21",                 String(-1e21) );
    return ( array );
}
