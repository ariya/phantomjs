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
    File Name:     delete-001.js
    Section:       regress
    Description:

    Regression test for
    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=108736

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "JS1_2";
    var VERSION = "JS1_2";
    var TITLE   = "The variable statment";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    // delete all properties of the global object
    // per ecma, this does not affect variables in the global object declared
    // with var or functions

    for ( p in this ) {
        delete p;
    }

    var result ="";

    for ( p in this ) {
        result += String( p );
    }

    // not too picky here... just want to make sure we didn't crash or something

    testcases[testcases.length] = new TestCase( SECTION,
        "delete all properties of the global object",
        "PASSED",
        result == "" ? "FAILED" : "PASSED" );


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
