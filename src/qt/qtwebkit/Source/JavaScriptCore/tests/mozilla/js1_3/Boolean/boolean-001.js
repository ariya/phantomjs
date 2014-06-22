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
 *  In JavaScript 1.2, new Boolean(false) evaluates to false.
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "boolean-001.js";
    var VERSION = "JS_1.3";
    var TITLE   = "new Boolean(false) should evaluate to false";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    BooleanTest( "new Boolean(true)",  new Boolean(true),  true );
    BooleanTest( "new Boolean(false)", new Boolean(false), true );
    BooleanTest( "true",               true,               true );
    BooleanTest( "false",              false,              false );

    test();

function BooleanTest( string, object, expect ) {
    if ( object ) {
        result = true;
    } else {
        result = false;
    }

    testcases[tc++] = new TestCase(
        SECTION,
        string,
        expect,
        result );
}

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
