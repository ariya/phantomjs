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
    File Name:          11.12-3.js
    ECMA Section:       11.12
    Description:

    The grammar for a ConditionalExpression in ECMAScript is a little bit
    different from that in C and Java, which each allow the second
    subexpression to be an Expression but restrict the third expression to
    be a ConditionalExpression.  The motivation for this difference in
    ECMAScript is to allow an assignment expression to be governed by either
    arm of a conditional and to eliminate the confusing and fairly useless
    case of a comma expression as the center expression.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "11.12-3";
    var VERSION = "ECMA_1";
    startTest();
    writeHeaderToLog( SECTION + " Conditional operator ( ? : )");

    var testcases = new Array();

    // the following expression should NOT be an error in JS.

    testcases[tc] = new TestCase( SECTION,
                                    "var MYVAR =  true ? ('FAIL1', 'PASSED') : 'FAIL2'; MYVAR",
                                    "PASSED",
                                    "var MYVAR =  true ? ('FAIL1', 'PASSED') : 'FAIL2'; MYVAR" );

    // get around potential parse time error by putting expression in an eval statement

    testcases[tc].actual = eval ( testcases[tc].actual );

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
