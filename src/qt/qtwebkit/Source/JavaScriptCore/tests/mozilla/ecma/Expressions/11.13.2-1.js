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
    File Name:          11.13.2-1.js
    ECMA Section:       11.13.2 Compound Assignment: *=
    Description:

    *= /= %= += -= <<= >>= >>>= &= ^= |=

    11.13.2 Compound assignment ( op= )

    The production AssignmentExpression :
    LeftHandSideExpression @ = AssignmentExpression, where @ represents one of
    the operators indicated above, is evaluated as follows:

    1.  Evaluate LeftHandSideExpression.
    2.  Call GetValue(Result(1)).
    3.  Evaluate AssignmentExpression.
    4.  Call GetValue(Result(3)).
    5.  Apply operator @ to Result(2) and Result(4).
    6.  Call PutValue(Result(1), Result(5)).
    7.  Return Result(5).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.13.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Compound Assignment: *=");
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    // NaN cases

    array[item++] = new TestCase( SECTION,    "VAR1 = NaN; VAR2=1; VAR1 *= VAR2",       Number.NaN, eval("VAR1 = Number.NaN; VAR2=1; VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = NaN; VAR2=1; VAR1 *= VAR2; VAR1", Number.NaN, eval("VAR1 = Number.NaN; VAR2=1; VAR1 *= VAR2; VAR1") );

    // number cases
    array[item++] = new TestCase( SECTION,    "VAR1 = 0; VAR2=1; VAR1 *= VAR2",         0,          eval("VAR1 = 0; VAR2=1; VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = 0; VAR2=1; VAR1 *= VAR2;VAR1",    0,          eval("VAR1 = 0; VAR2=1; VAR1 *= VAR2;VAR1") );

    array[item++] = new TestCase( SECTION,    "VAR1 = 0xFF; VAR2 = 0xA, VAR1 *= VAR2", 2550,      eval("VAR1 = 0XFF; VAR2 = 0XA, VAR1 *= VAR2") );

    // special multiplication cases

    array[item++] = new TestCase( SECTION,    "VAR1 = 0; VAR2= Infinity; VAR1 *= VAR2",    Number.NaN,      eval("VAR1 = 0; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 = -0; VAR2= Infinity; VAR1 *= VAR2",   Number.NaN,      eval("VAR1 = -0; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 = -0; VAR2= -Infinity; VAR1 *= VAR2",  Number.NaN,      eval("VAR1 = -0; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 = 0; VAR2= -Infinity; VAR1 *= VAR2",   Number.NaN,      eval("VAR1 = 0; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );

    array[item++] = new TestCase( SECTION,    "VAR1 = 0; VAR2= Infinity; VAR2 *= VAR1",    Number.NaN,      eval("VAR1 = 0; VAR2 = Number.POSITIVE_INFINITY; VAR2 *= VAR1; VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = -0; VAR2= Infinity; VAR2 *= VAR1",   Number.NaN,      eval("VAR1 = -0; VAR2 = Number.POSITIVE_INFINITY; VAR2 *= VAR1; VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = -0; VAR2= -Infinity; VAR2 *= VAR1",  Number.NaN,      eval("VAR1 = -0; VAR2 = Number.NEGATIVE_INFINITY; VAR2 *= VAR1; VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = 0; VAR2= -Infinity; VAR2 *= VAR1",   Number.NaN,      eval("VAR1 = 0; VAR2 = Number.NEGATIVE_INFINITY; VAR2 *= VAR1; VAR2") );

    array[item++] = new TestCase( SECTION,    "VAR1 = Infinity; VAR2= Infinity; VAR1 *= VAR2",   Number.POSITIVE_INFINITY,      eval("VAR1 = Number.POSITIVE_INFINITY; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 = Infinity; VAR2= -Infinity; VAR1 *= VAR2",  Number.NEGATIVE_INFINITY,      eval("VAR1 = Number.POSITIVE_INFINITY; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 =-Infinity; VAR2= Infinity; VAR1 *= VAR2",   Number.NEGATIVE_INFINITY,      eval("VAR1 = Number.NEGATIVE_INFINITY; VAR2 = Number.POSITIVE_INFINITY; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 =-Infinity; VAR2=-Infinity; VAR1 *= VAR2",   Number.POSITIVE_INFINITY,      eval("VAR1 = Number.NEGATIVE_INFINITY; VAR2 = Number.NEGATIVE_INFINITY; VAR1 *= VAR2; VAR1") );

    // string cases
    array[item++] = new TestCase( SECTION,    "VAR1 = 10; VAR2 = '255', VAR1 *= VAR2", 2550,       eval("VAR1 = 10; VAR2 = '255', VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = '255'; VAR2 = 10, VAR1 *= VAR2", 2550,       eval("VAR1 = '255'; VAR2 = 10, VAR1 *= VAR2") );

    array[item++] = new TestCase( SECTION,    "VAR1 = 10; VAR2 = '0XFF', VAR1 *= VAR2", 2550,       eval("VAR1 = 10; VAR2 = '0XFF', VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = '0xFF'; VAR2 = 0xA, VAR1 *= VAR2", 2550,      eval("VAR1 = '0XFF'; VAR2 = 0XA, VAR1 *= VAR2") );

    array[item++] = new TestCase( SECTION,    "VAR1 = '10'; VAR2 = '255', VAR1 *= VAR2", 2550,      eval("VAR1 = '10'; VAR2 = '255', VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = '10'; VAR2 = '0XFF', VAR1 *= VAR2", 2550,     eval("VAR1 = '10'; VAR2 = '0XFF', VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = '0xFF'; VAR2 = 0xA, VAR1 *= VAR2", 2550,      eval("VAR1 = '0XFF'; VAR2 = 0XA, VAR1 *= VAR2") );

    // boolean cases
    array[item++] = new TestCase( SECTION,    "VAR1 = true; VAR2 = false; VAR1 *= VAR2",    0,      eval("VAR1 = true; VAR2 = false; VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = true; VAR2 = true; VAR1 *= VAR2",    1,      eval("VAR1 = true; VAR2 = true; VAR1 *= VAR2") );

    // object cases
    array[item++] = new TestCase( SECTION,    "VAR1 = new Boolean(true); VAR2 = 10; VAR1 *= VAR2;VAR1",    10,      eval("VAR1 = new Boolean(true); VAR2 = 10; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 = new Number(11); VAR2 = 10; VAR1 *= VAR2; VAR1",    110,      eval("VAR1 = new Number(11); VAR2 = 10; VAR1 *= VAR2; VAR1") );
    array[item++] = new TestCase( SECTION,    "VAR1 = new Number(11); VAR2 = new Number(10); VAR1 *= VAR2",    110,      eval("VAR1 = new Number(11); VAR2 = new Number(10); VAR1 *= VAR2") );
    array[item++] = new TestCase( SECTION,    "VAR1 = new String('15'); VAR2 = new String('0xF'); VAR1 *= VAR2",    225,      eval("VAR1 = String('15'); VAR2 = new String('0xF'); VAR1 *= VAR2") );


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
