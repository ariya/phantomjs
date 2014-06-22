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
    File Name:          nesting-1.js
    Reference:          http://scopus.mcom.com/bugsplat/show_bug.cgi?id=122040
    Description:        Regression test for a nested function

    Author:             christine@netscape.com
    Date:               15 June 1998
*/

    var SECTION = "function/nesting-1.js";
    var VERSION = "JS_12";
    startTest();
    var TITLE   = "Regression test for 122040";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    function f(a) {function g(b) {return a+b;}; return g;}; f(7)

    testcases[tc++] = new TestCase( SECTION,
        'function f(a) {function g(b) {return a+b;}; return g;}; typeof f(7)',
        "function",
        typeof f(7) );

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
