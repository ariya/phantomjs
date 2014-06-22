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
    File Name:          12.2-1.js
    ECMA Section:       The variable statement
    Description:

    If the variable statement occurs inside a FunctionDeclaration, the
    variables are defined with function-local scope in that function, as
    described in section 10.1.3. Otherwise, they are defined with global
    scope, that is, they are created as members of the global object, as
    described in section 0. Variables are created when the execution scope
    is entered. A Block does not define a new execution scope. Only Program and
    FunctionDeclaration produce a new scope. Variables are initialized to the
    undefined value when created. A variable with an Initializer is assigned
    the value of its AssignmentExpression when the VariableStatement is executed,
    not when the variable is created.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "12.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The variable statement";

    writeHeaderToLog( SECTION +" "+ TITLE);

    var testcases = new Array();

    testcases[tc] = new TestCase(    "SECTION",
                                    "var x = 3; function f() { var a = x; var x = 23; return a; }; f()",
                                    void 0,
                                    eval("var x = 3; function f() { var a = x; var x = 23; return a; }; f()") );

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
