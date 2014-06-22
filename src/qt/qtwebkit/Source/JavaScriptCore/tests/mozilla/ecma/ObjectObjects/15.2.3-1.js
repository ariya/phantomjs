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
    File Name:          15.2.3-1.js
    ECMA Section:       15.2.3 Properties of the Object Constructor

    Description:        The value of the internal [[Prototype]] property of the
                        Object constructor is the Function prototype object.

                        Besides the call and construct propreties and the length
                        property, the Object constructor has properties described
                        in 15.2.3.1.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.2.3";
    var VERSION = "ECMA_2";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " Properties of the Object Constructor");
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Object.__proto__",     Function.prototype,     Object.__proto__ );
    array[item++] = new TestCase( SECTION,  "Object.length",        1,                      Object.length );

    return ( array );
}
function test( array ) {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual = eval( testcases[tc].actual );
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
