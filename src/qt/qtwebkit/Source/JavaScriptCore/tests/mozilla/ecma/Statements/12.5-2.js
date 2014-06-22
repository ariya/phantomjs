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
    File Name:          12.5-2.js
    ECMA Section:       The if statement
    Description:

    The production IfStatement : if ( Expression ) Statement else Statement
    is evaluated as follows:

    1.Evaluate Expression.
    2.Call GetValue(Result(1)).
    3.Call ToBoolean(Result(2)).
    4.If Result(3) is false, go to step 7.
    5.Evaluate the first Statement.
    6.Return Result(5).
    7.Evaluate the second Statement.
    8.Return Result(7).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "12.5-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE = "The if statement" ;

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase(   SECTION,
                                    "var MYVAR; if ( true ) MYVAR='PASSED'; MYVAR",
                                    "PASSED",
                                    eval("var MYVAR; if ( true ) MYVAR='PASSED'; MYVAR") );

    testcases[tc++] = new TestCase(  SECTION,
                                    "var MYVAR; if ( false ) MYVAR='FAILED'; MYVAR;",
                                    "PASSED",
                                    eval("var MYVAR=\"PASSED\"; if ( false ) MYVAR='FAILED'; MYVAR;") );

    testcases[tc++] = new TestCase(   SECTION,
                                    "var MYVAR; if ( new Boolean(true) ) MYVAR='PASSED'; MYVAR",
                                    "PASSED",
                                    eval("var MYVAR; if ( new Boolean(true) ) MYVAR='PASSED'; MYVAR") );

    testcases[tc++] = new TestCase(   SECTION,
                                    "var MYVAR; if ( new Boolean(false) ) MYVAR='PASSED'; MYVAR",
                                    "PASSED",
                                    eval("var MYVAR; if ( new Boolean(false) ) MYVAR='PASSED'; MYVAR") );

    testcases[tc++] = new TestCase(   SECTION,
                                    "var MYVAR; if ( 1 ) MYVAR='PASSED'; MYVAR",
                                    "PASSED",
                                    eval("var MYVAR; if ( 1 ) MYVAR='PASSED'; MYVAR") );

    testcases[tc++] = new TestCase(  SECTION,
                                    "var MYVAR; if ( 0 ) MYVAR='FAILED'; MYVAR;",
                                    "PASSED",
                                    eval("var MYVAR=\"PASSED\"; if ( 0 ) MYVAR='FAILED'; MYVAR;") );

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
