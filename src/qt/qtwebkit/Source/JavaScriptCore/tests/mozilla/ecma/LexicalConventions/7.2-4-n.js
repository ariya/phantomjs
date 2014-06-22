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
    File Name:          7.2.js
    ECMA Section:       7.2 Line Terminators
    Description:        - readability
                        - separate tokens
                        - may occur between any two tokens
                        - cannot occur within any token, not even a string
                        - affect the process of automatic semicolon insertion.

                        white space characters are:
                        unicode     name            formal name     string representation
                        \u000A      line feed       <LF>            \n
                        \u000D      carriage return <CR>            \r

                        this test uses onerror to capture line numbers.  because
                        we use on error, we can only have one test case per file.

    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "7.2-6";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Line Terminators";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function test() {
        // this is line 33

        a = "\nb";
        eval( a );

        // if we get this far, the test failed.
        testcases[tc].passed = writeTestCaseResult(
                   "failure on line" + testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[0].passed = false;

        testcases[tc].reason = "test should have caused runtime error ";

        stopTest();

        return ( testcases );
}



function getTestCases() {
    var array = new Array();
    var item = 0;

    array[0] = new TestCase( "7.2",    "a = \\nb",     "error",     "");

    return ( array );
}
