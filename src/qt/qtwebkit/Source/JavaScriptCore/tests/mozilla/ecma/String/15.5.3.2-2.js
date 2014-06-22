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
    File Name:          15.5.3.2-2.js
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

                        This tests String.fromCharCode with multiple arguments.

    Author:             christine@netscape.com
    Date:               2 october 1997
*/

    var SECTION = "15.5.3.2-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.fromCharCode()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,
                                  "var MYSTRING = String.fromCharCode(eval(\"var args=''; for ( i = 0x0020; i < 0x007f; i++ ) { args += ( i == 0x007e ) ? i : i + ', '; } args;\")); MYSTRING",
                                  " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
                                  eval( "var MYSTRING = String.fromCharCode(" + eval("var args=''; for ( i = 0x0020; i < 0x007f; i++ ) { args += ( i == 0x007e ) ? i : i + ', '; } args;") +"); MYSTRING" ));

    array[item++] = new TestCase( SECTION,
                                    "MYSTRING.length",
                                    0x007f - 0x0020,
                                    MYSTRING.length );
     return array;
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
