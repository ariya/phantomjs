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
    File Name:          12.6.3-1.js
    ECMA Section:       12.6.3 The for...in Statement
    Description:
    The production IterationStatement : for ( LeftHandSideExpression in Expression )
    Statement is evaluated as follows:

    1.  Evaluate the Expression.
    2.  Call GetValue(Result(1)).
    3.  Call ToObject(Result(2)).
    4.  Let C be "normal completion".
    5.  Get the name of the next property of Result(3) that doesn't have the
        DontEnum attribute. If there is no such property, go to step 14.
    6.  Evaluate the LeftHandSideExpression (it may be evaluated repeatedly).
    7.  Call PutValue(Result(6), Result(5)).  PutValue( V, W ):
        1.  If Type(V) is not Reference, generate a runtime error.
        2.  Call GetBase(V).
        3.  If Result(2) is null, go to step 6.
        4.  Call the [[Put]] method of Result(2), passing GetPropertyName(V)
            for the property name and W for the value.
        5.  Return.
        6.  Call the [[Put]] method for the global object, passing
            GetPropertyName(V) for the property name and W for the value.
        7.  Return.
    8.  Evaluate Statement.
    9.  If Result(8) is a value completion, change C to be "normal completion
        after value V" where V is the value carried by Result(8).
    10. If Result(8) is a break completion, go to step 14.
    11. If Result(8) is a continue completion, go to step 5.
    12. If Result(8) is a return completion, return Result(8).
    13. Go to step 5.
    14. Return C.

    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "12.6.3-4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The for..in statment";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    //  for ( LeftHandSideExpression in Expression )
    //  LeftHandSideExpression:NewExpression:MemberExpression

    var count = 0;
    function f() {     count++; return new Array("h","e","l","l","o"); }

    var result = "";
    for ( p in f() ) { result += f()[p] };

    testcases[testcases.length] = new TestCase( SECTION,
        "count = 0; result = \"\"; "+
        "function f() { count++; return new Array(\"h\",\"e\",\"l\",\"l\",\"o\"); }"+
        "for ( p in f() ) { result += f()[p] }; count",
        6,
        count );

    testcases[testcases.length] = new TestCase( SECTION,
        "result",
        "hello",
        result );



    //  LeftHandSideExpression:NewExpression:MemberExpression [ Expression ]
    //  LeftHandSideExpression:NewExpression:MemberExpression . Identifier
    //  LeftHandSideExpression:NewExpression:new MemberExpression Arguments
    //  LeftHandSideExpression:NewExpression:PrimaryExpression:( Expression )
    //  LeftHandSideExpression:CallExpression:MemberExpression Arguments
    //  LeftHandSideExpression:CallExpression Arguments
    //  LeftHandSideExpression:CallExpression [ Expression ]
    //  LeftHandSideExpression:CallExpression . Identifier

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

