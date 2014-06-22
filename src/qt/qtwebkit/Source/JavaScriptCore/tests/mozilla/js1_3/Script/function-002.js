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
    File Name:          function-002.js
    Section:
    Description:

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=249579

    function definitions in conditional statements should be allowed.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "function-002";
    var VERSION = "JS1_3";
    var TITLE   = "Regression test for 249579";
    var BUGNUMBER="249579";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase(
        SECTION,
        "0?function(){}:0",
        0,
        0?function(){}:0 );


    bar = true;
    foo = bar ? function () { return true; } : function() { return false; };

    testcases[tc++] = new TestCase(
        SECTION,
        "bar = true; foo = bar ? function () { return true; } : function() { return false; }; foo()",
        true,
        foo() );


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
