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
    File Name:          15.1.2.1-2.js
    ECMA Section:       15.1.2.1 eval(x)

                        Parse x as an ECMAScript Program.  If the parse fails,
                        generate a runtime error.  Evaluate the program.  If
                        result is "Normal completion after value V", return
                        the value V.  Else, return undefined.
    Description:
    Author:             christine@netscape.com
    Date:               16 september 1997
*/
    var SECTION = "15.1.2.1-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "eval(x)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[0] = new TestCase(    SECTION,
                                "d = new Date(0); with (d) { x = getUTCMonth() +'/'+ getUTCDate() +'/'+ getUTCFullYear(); } x",
                                "0/1/1970",
                                eval( "d = new Date(0); with (d) { x = getUTCMonth() +'/'+ getUTCDate() +'/'+ getUTCFullYear(); } x" )
                            );
    return ( array );
}

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
