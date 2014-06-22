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
    File Name:          10.1.4-1.js
    ECMA Section:       10.1.4 Scope Chain and Identifier Resolution
    Description:
    Every execution context has associated with it a scope chain. This is
    logically a list of objects that are searched when binding an Identifier.
    When control enters an execution context, the scope chain is created and
    is populated with an initial set of objects, depending on the type of
    code. When control leaves the execution context, the scope chain is
    destroyed.

    During execution, the scope chain of the execution context is affected
    only by WithStatement. When execution enters a with block, the object
    specified in the with statement is added to the front of the scope chain.
    When execution leaves a with block, whether normally or via a break or
    continue statement, the object is removed from the scope chain. The object
    being removed will always be the first object in the scope chain.

    During execution, the syntactic production PrimaryExpression : Identifier
    is evaluated using the following algorithm:

    1.  Get the next object in the scope chain. If there isn't one, go to step 5.
    2.  Call the [[HasProperty]] method of Result(l), passing the Identifier as
        the property.
    3.  If Result(2) is true, return a value of type Reference whose base object
        is Result(l) and whose property name is the Identifier.
    4.  Go to step 1.
    5.  Return a value of type Reference whose base object is null and whose
        property name is the Identifier.
    The result of binding an identifier is always a value of type Reference with
    its member name component equal to the identifier string.
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "10.1.4-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");

    var testcases = getTestCases();
    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {

        var MYOBJECT = new MyObject();
        var INPUT = 2;
        testcases[tc].description += ( INPUT +"" );

        with ( MYOBJECT ) {
                eval( INPUT );
        }

        testcases[tc].actual = eval( INPUT );
        testcases[tc].expect = INPUT;

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

    array[item++] = new TestCase( "SECTION", "with MyObject, eval should be [object Global].eval " );

    return ( array );
}

function MyObject() {
    this.eval = new Function( "x", "return(Math.pow(Number(x),2))" );
}