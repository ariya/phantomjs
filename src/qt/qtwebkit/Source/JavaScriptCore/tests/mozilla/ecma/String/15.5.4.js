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
    File Name:          15.5.4.js
    ECMA Section:       15.5.4 Properties of the String prototype object

    Description:
    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.5.4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Properties of the String Prototype objecta";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,
                                  "String.prototype.getClass = Object.prototype.toString; String.prototype.getClass()",
                                  "[object String]",
                                  eval("String.prototype.getClass = Object.prototype.toString; String.prototype.getClass()") );

    array[item++] = new TestCase( SECTION, "typeof String.prototype",   "object",   typeof String.prototype );
    array[item++] = new TestCase( SECTION, "String.prototype.valueOf()", "",        String.prototype.valueOf() );
    array[item++] = new TestCase( SECTION, "String.prototype +''",       "",        String.prototype + '' );
    array[item++] = new TestCase( SECTION, "String.prototype.length",    0,         String.prototype.length );
//    array[item++] = new TestCase( SECTION, "String.prototype.__proto__",    Object.prototype,   String.prototype.__proto__ );


    return ( array );
}
function test( array ) {
        for ( tc=0; tc < testcases.length; tc++ ) {
            testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";

        }

        stopTest();

    //  all tests must return an array of TestCase objects
        return ( testcases );
}
