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
 *  File Name:          boolean-001.js
 *  Description:
 *
 *  http://scopus.mcom.com/bugsplat/show_bug.cgi?id=99232
 *
 *  eval("function f(){}function g(){}") at top level is an error for JS1.2
 *     and above (missing ; between named function expressions), but declares f
 *     and g as functions below 1.2.
 *
 * Fails to produce error regardless of version:
 * js> version(100)
 * 120
 * js> eval("function f(){}function g(){}")
 * js> version(120);
 * 100
 * js> eval("function f(){}function g(){}")
 * js>
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "function-001.js";
    var VERSION = "JS1_1";
    startTest();
    var TITLE   = "functions not separated by semicolons are errors in version 120 and higher";
    var BUGNUMBER="99232";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase(
        SECTION,
        "eval(\"function f(){}function g(){}\")",
        undefined,
        eval("function f(){}function g(){}") );

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
