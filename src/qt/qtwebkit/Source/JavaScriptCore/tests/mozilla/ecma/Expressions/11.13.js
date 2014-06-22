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
    File Name:          11.12.js
    ECMA Section:       11.12 Conditional Operator
    Description:
    Logi

    calORExpression ? AssignmentExpression : AssignmentExpression

    Semantics

    The production ConditionalExpression :
    LogicalORExpression ? AssignmentExpression : AssignmentExpression
    is evaluated as follows:

    1.  Evaluate LogicalORExpression.
    2.  Call GetValue(Result(1)).
    3.  Call ToBoolean(Result(2)).
    4.  If Result(3) is false, go to step 8.
    5.  Evaluate the first AssignmentExpression.
    6.  Call GetValue(Result(5)).
    7.  Return Result(6).
    8.  Evaluate the second AssignmentExpression.
    9.  Call GetValue(Result(8)).
   10.  Return Result(9).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.12";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Conditional operator( ? : )");
    test();
function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,    "true ? 'PASSED' : 'FAILED'",     "PASSED",       (true?"PASSED":"FAILED"));
    array[item++] = new TestCase( SECTION,    "false ? 'FAILED' : 'PASSED'",     "PASSED",      (false?"FAILED":"PASSED"));

    array[item++] = new TestCase( SECTION,    "1 ? 'PASSED' : 'FAILED'",     "PASSED",          (true?"PASSED":"FAILED"));
    array[item++] = new TestCase( SECTION,    "0 ? 'FAILED' : 'PASSED'",     "PASSED",          (false?"FAILED":"PASSED"));
    array[item++] = new TestCase( SECTION,    "-1 ? 'PASSED' : 'FAILED'",     "PASSED",          (true?"PASSED":"FAILED"));

    array[item++] = new TestCase( SECTION,    "NaN ? 'FAILED' : 'PASSED'",     "PASSED",          (Number.NaN?"FAILED":"PASSED"));

    array[item++] = new TestCase( SECTION,    "var VAR = true ? , : 'FAILED'", "PASSED",           (VAR = true ? "PASSED" : "FAILED") );

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
