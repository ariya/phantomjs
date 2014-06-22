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
    File Name:          15.5.3.2-1.js
    ECMA Section:       15.5.3.2  String.fromCharCode( char0, char1, ... )
    Description:        Return a string value containing as many characters
                        as the number of arguments.  Each argument specifies
                        one character of the resulting string, with the first
                        argument specifying the first character, and so on,
                        from left to right.  An argument is converted to a
                        character by applying the operation ToUint16 and
                        regarding the resulting 16bit integeras the Unicode
                        encoding of a character.  If no arguments are supplied,
                        the result is the empty string.

                        This test covers Basic Latin (range U+0020 - U+007F)

    Author:             christine@netscape.com
    Date:               2 october 1997
*/

    var SECTION = "15.5.3.2-3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.fromCharCode()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    for ( CHARCODE = 0; CHARCODE < 256; CHARCODE++ ) {
        array[item++] = new TestCase(   SECTION,
                                        "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
                                        ToUint16(CHARCODE),
                                        (String.fromCharCode(CHARCODE)).charCodeAt(0)
                                     );
    }
    for ( CHARCODE = 256; CHARCODE < 65536; CHARCODE+=333 ) {
        array[item++] = new TestCase(   SECTION,
                                        "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
                                        ToUint16(CHARCODE),
                                        (String.fromCharCode(CHARCODE)).charCodeAt(0)
                                     );
    }
    for ( CHARCODE = 65535; CHARCODE < 65538; CHARCODE++ ) {
        array[item++] = new TestCase(   SECTION,
                                        "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
                                        ToUint16(CHARCODE),
                                        (String.fromCharCode(CHARCODE)).charCodeAt(0)
                                     );
    }
    for ( CHARCODE = Math.pow(2,32)-1; CHARCODE < Math.pow(2,32)+1; CHARCODE++ ) {
        array[item++] = new TestCase(   SECTION,
                                        "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
                                        ToUint16(CHARCODE),
                                        (String.fromCharCode(CHARCODE)).charCodeAt(0)
                                     );
    }
    for ( CHARCODE = 0; CHARCODE > -65536; CHARCODE-=3333 ) {
        array[item++] = new TestCase(   SECTION,
                                        "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
                                        ToUint16(CHARCODE),
                                        (String.fromCharCode(CHARCODE)).charCodeAt(0)
                                     );
    }
    array[item++] = new TestCase( SECTION, "(String.fromCharCode(65535)).charCodeAt(0)",    65535,  (String.fromCharCode(65535)).charCodeAt(0) );
    array[item++] = new TestCase( SECTION, "(String.fromCharCode(65536)).charCodeAt(0)",    0,      (String.fromCharCode(65536)).charCodeAt(0) );
    array[item++] = new TestCase( SECTION, "(String.fromCharCode(65537)).charCodeAt(0)",    1,      (String.fromCharCode(65537)).charCodeAt(0) );

     return array;
}
function ToUint16( num ) {
    num = Number( num );
    if ( isNaN( num ) || num == 0 || num == Number.POSITIVE_INFINITY || num == Number.NEGATIVE_INFINITY ) {
        return 0;
    }

    var sign = ( num < 0 ) ? -1 : 1;

    num = sign * Math.floor( Math.abs( num ) );
    num = num % Math.pow(2,16);
    num = ( num > -65536 && num < 0) ? 65536 + num : num;
    return num;
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
