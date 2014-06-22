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
    File Name:          15.2.3.1-4.js
    ECMA Section:       15.2.3.1 Object.prototype

    Description:        The initial value of Object.prototype is the built-in
                        Object prototype object.

                        This property shall have the attributes [ DontEnum,
                        DontDelete ReadOnly ]

                        This tests the [DontDelete] property of Object.prototype

    Author:             christine@netscape.com
    Date:               28 october 1997

*/

    var SECTION = "15.2.3.1-4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Object.prototype";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase( SECTION,  "delete( Object.prototype ); Object.prototype",
                                            Object.prototype,
                                            "delete(Object.prototype); Object.prototype"
                                );
    return ( array );
}
function test( array ) {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual = eval(testcases[tc].actual);
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
