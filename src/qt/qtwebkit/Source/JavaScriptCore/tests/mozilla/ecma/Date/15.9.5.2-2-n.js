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
    File Name:          15.9.5.2-2.js
    ECMA Section:       15.9.5.2 Date.prototype.toString
    Description:
    This function returns a string value. The contents of the string are
    implementation dependent, but are intended to represent the Date in a
    convenient, human-readable form in the current time zone.

    The toString function is not generic; it generates a runtime error if its
    this value is not a Date object. Therefore it cannot be transferred to
    other kinds of objects for use as a method.


    This verifies that calling toString on an object that is not a string
    generates a runtime error.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.9.5.2-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Date.prototype.toString";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var OBJ = new MyObject( new Date(0) );

    testcases[tc++] = new TestCase( SECTION,
                                    "var OBJ = new MyObject( new Date(0) ); OBJ.toString()",
                                    "error",
                                    OBJ.toString() );
    test();

function MyObject( value ) {
    this.value = value;
    this.valueOf = new Function( "return this.value" );
    this.toString = Date.prototype.toString;
    return this;
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
