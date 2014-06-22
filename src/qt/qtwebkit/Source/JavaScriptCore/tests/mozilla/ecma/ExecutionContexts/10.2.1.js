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
    File Name:          10.2.1.js
    ECMA Section:       10.2.1 Global Code
    Description:

    The scope chain is created and initialized to contain the global object and
    no others.

    Variable instantiation is performed using the global object as the variable
    object and using empty property attributes.

    The this value is the global object.
    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "10.2.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Global Code";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var THIS = this;

    testcases[tc++] = new TestCase( SECTION,
                                    "this +''",
                                    GLOBAL,
                                    THIS + "" );

    var GLOBAL_PROPERTIES = new Array();
    var i = 0;

    for ( p in this ) {
        GLOBAL_PROPERTIES[i++] = p;
    }

    for ( i = 0; i < GLOBAL_PROPERTIES.length; i++ ) {
        testcases[tc++] = new TestCase( SECTION,
                                        GLOBAL_PROPERTIES[i] +" == void 0",
                                        false,
                                        eval("GLOBAL_PROPERTIES["+i+"] == void 0"));
    }

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
