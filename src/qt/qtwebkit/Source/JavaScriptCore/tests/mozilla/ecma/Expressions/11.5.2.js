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
    File Name:          11.5.2.js
    ECMA Section:       11.5.2 Applying the / operator
    Description:

    The / operator performs division, producing the quotient of its operands.
    The left operand is the dividend and the right operand is the divisor.
    ECMAScript does not perform integer division. The operands and result of all
    division operations are double-precision floating-point numbers.
    The result of division is determined by the specification of IEEE 754 arithmetic:

      If either operand is NaN, the result is NaN.
      The sign of the result is positive if both operands have the same sign, negative if the operands have different
      signs.
      Division of an infinity by an infinity results in NaN.
      Division of an infinity by a zero results in an infinity. The sign is determined by the rule already stated above.
      Division of an infinity by a non-zero finite value results in a signed infinity. The sign is determined by the rule
      already stated above.
      Division of a finite value by an infinity results in zero. The sign is determined by the rule already stated above.
      Division of a zero by a zero results in NaN; division of zero by any other finite value results in zero, with the sign
      determined by the rule already stated above.
      Division of a non-zero finite value by a zero results in a signed infinity. The sign is determined by the rule
      already stated above.
      In the remaining cases, where neither an infinity, nor a zero, nor NaN is involved, the quotient is computed and
      rounded to the nearest representable value using IEEE 754 round-to-nearest mode. If the magnitude is too
      large to represent, we say the operation overflows; the result is then an infinity of appropriate sign. If the
      magnitude is too small to represent, we say the operation underflows and the result is a zero of the appropriate
      sign. The ECMAScript language requires support of gradual underflow as defined by IEEE 754.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.5.2";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();
    var BUGNUMBER="111202";

    writeHeaderToLog( SECTION + " Applying the / operator");
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

   // if either operand is NaN, the result is NaN.

    array[item++] = new TestCase( SECTION,    "Number.NaN / Number.NaN",    Number.NaN,     Number.NaN / Number.NaN );
    array[item++] = new TestCase( SECTION,    "Number.NaN / 1",             Number.NaN,     Number.NaN / 1 );
    array[item++] = new TestCase( SECTION,    "1 / Number.NaN",             Number.NaN,     1 / Number.NaN );

    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.NaN",    Number.NaN,     Number.POSITIVE_INFINITY / Number.NaN );
    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.NaN",    Number.NaN,     Number.NEGATIVE_INFINITY / Number.NaN );

    // Division of an infinity by an infinity results in NaN.

    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.NEGATIVE_INFINITY",    Number.NaN,   Number.NEGATIVE_INFINITY / Number.NEGATIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.NEGATIVE_INFINITY",    Number.NaN,   Number.POSITIVE_INFINITY / Number.NEGATIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.POSITIVE_INFINITY",    Number.NaN,   Number.NEGATIVE_INFINITY / Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.POSITIVE_INFINITY",    Number.NaN,   Number.POSITIVE_INFINITY / Number.POSITIVE_INFINITY );

    // Division of an infinity by a zero results in an infinity.

    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / 0",   Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY / 0 );
    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / 0",   Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY / 0 );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / -0",  Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY / -0 );
    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / -0",  Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY / -0 );

    // Division of an infinity by a non-zero finite value results in a signed infinity.

    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / 1 ",          Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY / 1 );
    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / -1 ",         Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY / -1 );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / 1 ",          Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY / 1 );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / -1 ",         Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY / -1 );

    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.MAX_VALUE ",          Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY / Number.MAX_VALUE );
    array[item++] = new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / -Number.MAX_VALUE ",         Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY / -Number.MAX_VALUE );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.MAX_VALUE ",          Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY / Number.MAX_VALUE );
    array[item++] = new TestCase( SECTION,    "Number.POSITIVE_INFINITY / -Number.MAX_VALUE ",         Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY / -Number.MAX_VALUE );

    // Division of a finite value by an infinity results in zero.

    array[item++] = new TestCase( SECTION,    "1 / Number.NEGATIVE_INFINITY",   -0,             1 / Number.NEGATIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "1 / Number.POSITIVE_INFINITY",   0,              1 / Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "-1 / Number.POSITIVE_INFINITY",  -0,             -1 / Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "-1 / Number.NEGATIVE_INFINITY",  0,              -1 / Number.NEGATIVE_INFINITY );

    array[item++] = new TestCase( SECTION,    "Number.MAX_VALUE / Number.NEGATIVE_INFINITY",   -0,             Number.MAX_VALUE / Number.NEGATIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "Number.MAX_VALUE / Number.POSITIVE_INFINITY",   0,              Number.MAX_VALUE / Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "-Number.MAX_VALUE / Number.POSITIVE_INFINITY",  -0,             -Number.MAX_VALUE / Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "-Number.MAX_VALUE / Number.NEGATIVE_INFINITY",  0,              -Number.MAX_VALUE / Number.NEGATIVE_INFINITY );

    // Division of a zero by a zero results in NaN

    array[item++] = new TestCase( SECTION,    "0 / -0",                         Number.NaN,     0 / -0 );
    array[item++] = new TestCase( SECTION,    "-0 / 0",                         Number.NaN,     -0 / 0 );
    array[item++] = new TestCase( SECTION,    "-0 / -0",                        Number.NaN,     -0 / -0 );
    array[item++] = new TestCase( SECTION,    "0 / 0",                          Number.NaN,     0 / 0 );

    // division of zero by any other finite value results in zero

    array[item++] = new TestCase( SECTION,    "0 / 1",                          0,              0 / 1 );
    array[item++] = new TestCase( SECTION,    "0 / -1",                        -0,              0 / -1 );
    array[item++] = new TestCase( SECTION,    "-0 / 1",                        -0,              -0 / 1 );
    array[item++] = new TestCase( SECTION,    "-0 / -1",                       0,               -0 / -1 );

    // Division of a non-zero finite value by a zero results in a signed infinity.

    array[item++] = new TestCase( SECTION,    "1 / 0",                          Number.POSITIVE_INFINITY,   1/0 );
    array[item++] = new TestCase( SECTION,    "1 / -0",                         Number.NEGATIVE_INFINITY,   1/-0 );
    array[item++] = new TestCase( SECTION,    "-1 / 0",                         Number.NEGATIVE_INFINITY,   -1/0 );
    array[item++] = new TestCase( SECTION,    "-1 / -0",                        Number.POSITIVE_INFINITY,   -1/-0 );

    array[item++] = new TestCase( SECTION,    "0 / Number.POSITIVE_INFINITY",   0,      0 / Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "0 / Number.NEGATIVE_INFINITY",   -0,     0 / Number.NEGATIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "-0 / Number.POSITIVE_INFINITY",  -0,     -0 / Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,    "-0 / Number.NEGATIVE_INFINITY",  0,      -0 / Number.NEGATIVE_INFINITY );

    return ( array );
}
