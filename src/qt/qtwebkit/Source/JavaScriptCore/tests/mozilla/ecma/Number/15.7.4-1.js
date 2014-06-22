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
    File Name:          15.7.4-1.js
    ECMA Section:       15.7.4.1 Properties of the Number Prototype Object
    Description:
    Author:             christine@netscape.com
    Date:               16 september 1997
*/


    var SECTION = "15.7.4-1";
    var VERSION = "ECMA_1";
    startTest();
    writeHeaderToLog( SECTION + "Properties of the Number prototype object");

    var testcases = getTestCases();

    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase(SECTION, "Number.prototype.valueOf()",      0,                  Number.prototype.valueOf() );
    array[item++] = new TestCase(SECTION, "typeof(Number.prototype)",        "object",           typeof(Number.prototype) );
    array[item++] = new TestCase(SECTION, "Number.prototype.constructor == Number",    true,     Number.prototype.constructor == Number );
//    array[item++] = new TestCase(SECTION, "Number.prototype == Number.__proto__",      true,   Number.prototype == Number.__proto__ );
    return ( array );
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