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
    File Name:          15.5.4.5-4.js
    ECMA Section:       15.5.4.5 String.prototype.charCodeAt(pos)

    Description:        Returns a nonnegative integer less than 2^16.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var VERSION = "0697";
    startTest();
    var SECTION = "15.5.4.5-4";

    writeHeaderToLog( SECTION + " String.prototype.charCodeAt(pos)" );
    var tc= 0;
    var testcases = getTestCases();

//  all tests must call a function that returns an array of TestCase objects.
    test();

function getTestCases() {
    var array = new Array();
    var MAXCHARCODE = Math.pow(2,16);
    var item=0, CHARCODE;

    for ( CHARCODE=0; CHARCODE <256; CHARCODE++ ) {
        array[item++] = new TestCase( SECTION,
                                     "(String.fromCharCode("+CHARCODE+")).charCodeAt(0)",
                                     CHARCODE,
                                     (String.fromCharCode(CHARCODE)).charCodeAt(0) );
    }
    for ( CHARCODE=256; CHARCODE < 65536; CHARCODE+=999 ) {
        array[item++] = new TestCase( SECTION,
                                     "(String.fromCharCode("+CHARCODE+")).charCodeAt(0)",
                                     CHARCODE,
                                     (String.fromCharCode(CHARCODE)).charCodeAt(0) );
    }

    array[item++] = new TestCase( SECTION, "(String.fromCharCode(65535)).charCodeAt(0)", 65535,     (String.fromCharCode(65535)).charCodeAt(0) );

    return ( array );
}
function test() {
        for ( tc = 0; tc < testcases.length; tc++ ) {
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
