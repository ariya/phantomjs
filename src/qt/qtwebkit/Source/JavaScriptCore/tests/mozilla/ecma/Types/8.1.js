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
    File Name:          8.1.js
    ECMA Section:       The undefined type
    Description:

    The Undefined type has exactly one value, called undefined. Any variable
    that has not been assigned a value is of type Undefined.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "8.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The undefined type";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase( SECTION,
                                    "var x; typeof x",
                                    "undefined",
                                    eval("var x; typeof x") );

    testcases[tc++] = new TestCase( SECTION,
                                    "var x; typeof x == 'undefined",
                                    true,
                                    eval("var x; typeof x == 'undefined'") );

    testcases[tc++] = new TestCase( SECTION,
                                    "var x; x == void 0",
                                    true,
                                    eval("var x; x == void 0") );
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
