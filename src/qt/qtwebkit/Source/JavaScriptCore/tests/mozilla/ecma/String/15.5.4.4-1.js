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
    File Name:          15.5.4.4-1.js
    ECMA Section:       15.5.4.4 String.prototype.charAt(pos)
    Description:        Returns a string containing the character at position
                        pos in the string.  If there is no character at that
                        string, the result is the empty string.  The result is
                        a string value, not a String object.

                        When the charAt method is called with one argument,
                        pos, the following steps are taken:
                        1. Call ToString, with this value as its argument
                        2. Call ToInteger pos

                        In this test, this is a String, pos is an integer, and
                        all pos are in range.

    Author:             christine@netscape.com
    Date:               2 october 1997
*/
    var SECTION = "15.5.4.4-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.charAt";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var TEST_STRING = new String( " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    for (  i = 0x0020; i < 0x007e; i++, item++) {
        array[item] = new TestCase( SECTION,
                                "TEST_STRING.charAt("+item+")",
                                String.fromCharCode( i ),
                                TEST_STRING.charAt( item ) );
    }
    for ( i = 0x0020; i < 0x007e; i++, item++) {
        array[item] = new TestCase( SECTION,
                                "TEST_STRING.charAt("+item+") == TEST_STRING.substring( "+item +", "+ (item+1) + ")",
                                true,
                                TEST_STRING.charAt( item )  == TEST_STRING.substring( item, item+1 )
                                );
    }
    array[item++] = new TestCase( SECTION,  "String.prototype.charAt.length",       1,  String.prototype.charAt.length );

    return array;
}
function test() {
        writeLineToLog( "TEST_STRING = new String(\" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\")" );
        for ( tc = 0; tc < testcases.length; tc++ ) {
            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+ testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value "
        }
        stopTest();

    return ( testcases );
}
