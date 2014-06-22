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
    File Name:          switch-001.js
    Section:
    Description:

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=315767

    Verify that switches do not use strict equality in
    versions of JavaScript < 1.4.  It's now been decided that
    we won't put in version switches, so all switches will
    be ECMA.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "switch-001";
    var VERSION = "JS1_3";
    var TITLE   = "switch-001";
    var BUGNUMBER="315767";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    result = "fail:  did not enter switch";

    switch (true) {
        case 1:
            result = "fail:  version 130 should force strict equality";
            break;
        case true:
             result = "pass";
             break;
        default:
            result = "fail: evaluated default statement";
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "switch / case should use strict equality in version of JS < 1.4",
        "pass",
        result );

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
