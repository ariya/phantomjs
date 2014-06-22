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
    File Name:          11.2.2-4-n.js
    ECMA Section:       11.2.2. The new operator
    Description:

    MemberExpression:
        PrimaryExpression
        MemberExpression[Expression]
        MemberExpression.Identifier
        new MemberExpression Arguments

    new NewExpression

    The production NewExpression : new NewExpression is evaluated as follows:

   1.   Evaluate NewExpression.
   2.   Call GetValue(Result(1)).
   3.   If Type(Result(2)) is not Object, generate a runtime error.
   4.   If Result(2) does not implement the internal [[Construct]] method,
        generate a runtime error.
   5.   Call the [[Construct]] method on Result(2), providing no arguments
        (that is, an empty list of arguments).
   6.   If Type(Result(5)) is not Object, generate a runtime error.
   7.   Return Result(5).

    The production MemberExpression : new MemberExpression Arguments is evaluated as follows:

   1.   Evaluate MemberExpression.
   2.   Call GetValue(Result(1)).
   3.   Evaluate Arguments, producing an internal list of argument values
        (section 0).
   4.   If Type(Result(2)) is not Object, generate a runtime error.
   5.   If Result(2) does not implement the internal [[Construct]] method,
        generate a runtime error.
   6.   Call the [[Construct]] method on Result(2), providing the list
        Result(3) as the argument values.
   7.   If Type(Result(6)) is not Object, generate a runtime error.
   8    .Return Result(6).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "11.2.2-4-n.js";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The new operator";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();


    var STRING = "";

    testcases[tc++] = new TestCase( SECTION,
                                    "STRING = '', var s = new STRING()",
                                    "error",
                                    s = new STRING() );
    test();

function TestFunction() {
    return arguments;
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
