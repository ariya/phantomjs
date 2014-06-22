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
    File Name:          15.5.4.2.js
    ECMA Section:       15.5.4.2 String.prototype.toString

    Description:
    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.5.4.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.tostring";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "String.prototype.toString.__proto__",  Function.prototype, String.prototype.toString.__proto__ );
    array[item++] = new TestCase(   SECTION,
                                    "String.prototype.toString() == String.prototype.valueOf()",
                                    true,
                                    String.prototype.toString() == String.prototype.valueOf() );

    array[item++] = new TestCase(   SECTION, "String.prototype.toString()",     "",     String.prototype.toString() );
    array[item++] = new TestCase(   SECTION, "String.prototype.toString.length",    0,  String.prototype.toString.length );


    array[item++] = new TestCase(   SECTION,
                                    "TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()",
                                    true,
                                    eval("TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()") );
    array[item++] = new TestCase(   SECTION,
                                    "TESTSTRING = new String(true);TESTSTRING.valueOf() == TESTSTRING.toString()",
                                    true,
                                    eval("TESTSTRING = new String(true);TESTSTRING.valueOf() == TESTSTRING.toString()") );
    array[item++] = new TestCase(   SECTION,
                                    "TESTSTRING = new String(false);TESTSTRING.valueOf() == TESTSTRING.toString()",
                                    true,
                                    eval("TESTSTRING = new String(false);TESTSTRING.valueOf() == TESTSTRING.toString()") );
    array[item++] = new TestCase(   SECTION,
                                    "TESTSTRING = new String(Math.PI);TESTSTRING.valueOf() == TESTSTRING.toString()",
                                    true,
                                    eval("TESTSTRING = new String(Math.PI);TESTSTRING.valueOf() == TESTSTRING.toString()") );
    array[item++] = new TestCase(   SECTION,
                                    "TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()",
                                    true,
                                    eval("TESTSTRING = new String();TESTSTRING.valueOf() == TESTSTRING.toString()") );

    return ( array );
}
function test( array ) {
        for ( ; tc < testcases.length; tc++ ) {
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
