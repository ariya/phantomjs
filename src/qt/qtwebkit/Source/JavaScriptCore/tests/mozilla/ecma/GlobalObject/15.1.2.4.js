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
    File Name:          15.1.2.4.js
    ECMA Section:       15.1.2.4  Function properties of the global object
                        escape( string )

    Description:
    The escape function computes a new version of a string value in which
    certain characters have been replaced by a hexadecimal escape sequence.
    The result thus contains no special characters that might have special
    meaning within a URL.

    For characters whose Unicode encoding is 0xFF or less, a two-digit
    escape sequence of the form %xx is used in accordance with RFC1738.
    For characters whose Unicode encoding is greater than 0xFF, a four-
    digit escape sequence of the form %uxxxx is used.

    When the escape function is called with one argument string, the
    following steps are taken:

    1.  Call ToString(string).
    2.  Compute the number of characters in Result(1).
    3.  Let R be the empty string.
    4.  Let k be 0.
    5.  If k equals Result(2), return R.
    6.  Get the character at position k within Result(1).
    7.  If Result(6) is one of the 69 nonblank ASCII characters
        ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz
        0123456789 @*_+-./, go to step 14.
    8.  Compute the 16-bit unsigned integer that is the Unicode character
        encoding of Result(6).
    9.  If Result(8), is less than 256, go to step 12.
   10.  Let S be a string containing six characters "%uwxyz" where wxyz are
        four hexadecimal digits encoding the value of Result(8).
   11.  Go to step 15.
   12.  Let S be a string containing three characters "%xy" where xy are two
        hexadecimal digits encoding the value of Result(8).
   13.  Go to step 15.
   14.  Let S be a string containing the single character Result(6).
   15.  Let R be a new string value computed by concatenating the previous value
        of R and S.
   16.  Increase k by 1.
   17.  Go to step 5.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.1.2.4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "escape(string)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "escape.length",         1,          escape.length );
    array[item++] = new TestCase( SECTION, "escape.length = null; escape.length",   1,  eval("escape.length = null; escape.length") );
    array[item++] = new TestCase( SECTION, "delete escape.length",                  false,  delete escape.length );
    array[item++] = new TestCase( SECTION, "delete escape.length; escape.length",   1,      eval("delete escape.length; escape.length") );
    array[item++] = new TestCase( SECTION, "var MYPROPS=''; for ( var p in escape ) { MYPROPS+= p}; MYPROPS",    "",    eval("var MYPROPS=''; for ( var p in escape ) { MYPROPS+= p}; MYPROPS") );

    array[item++] = new TestCase( SECTION, "escape()",              "undefined",    escape() );
    array[item++] = new TestCase( SECTION, "escape('')",            "",             escape('') );
    array[item++] = new TestCase( SECTION, "escape( null )",        "null",         escape(null) );
    array[item++] = new TestCase( SECTION, "escape( void 0 )",      "undefined",    escape(void 0) );
    array[item++] = new TestCase( SECTION, "escape( true )",        "true",         escape( true ) );
    array[item++] = new TestCase( SECTION, "escape( false )",       "false",        escape( false ) );

    array[item++] = new TestCase( SECTION, "escape( new Boolean(true) )",   "true", escape(new Boolean(true)) );
    array[item++] = new TestCase( SECTION, "escape( new Boolean(false) )",  "false",    escape(new Boolean(false)) );

    array[item++] = new TestCase( SECTION, "escape( Number.NaN  )",                 "NaN",      escape(Number.NaN) );
    array[item++] = new TestCase( SECTION, "escape( -0 )",                          "0",        escape( -0 ) );
    array[item++] = new TestCase( SECTION, "escape( 'Infinity' )",                  "Infinity", escape( "Infinity" ) );
    array[item++] = new TestCase( SECTION, "escape( Number.POSITIVE_INFINITY )",    "Infinity", escape( Number.POSITIVE_INFINITY ) );
    array[item++] = new TestCase( SECTION, "escape( Number.NEGATIVE_INFINITY )",    "-Infinity", escape( Number.NEGATIVE_INFINITY ) );

    var ASCII_TEST_STRING = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@*_+-./";

    array[item++] = new TestCase( SECTION, "escape( " +ASCII_TEST_STRING+" )",    ASCII_TEST_STRING,  escape( ASCII_TEST_STRING ) );

    // ASCII value less than

    for ( var CHARCODE = 0; CHARCODE < 32; CHARCODE++ ) {
        array[item++] = new TestCase( SECTION,
                            "escape(String.fromCharCode("+CHARCODE+"))",
                            "%"+ToHexString(CHARCODE),
                            escape(String.fromCharCode(CHARCODE))  );
    }
    for ( var CHARCODE = 128; CHARCODE < 256; CHARCODE++ ) {
        array[item++] = new TestCase( SECTION,
                            "escape(String.fromCharCode("+CHARCODE+"))",
                            "%"+ToHexString(CHARCODE),
                            escape(String.fromCharCode(CHARCODE))  );
    }

    for ( var CHARCODE = 256; CHARCODE < 1024; CHARCODE++ ) {
        array[item++] = new TestCase( SECTION,
                            "escape(String.fromCharCode("+CHARCODE+"))",
                            "%u"+ ToUnicodeString(CHARCODE),
                            escape(String.fromCharCode(CHARCODE))  );
    }
    for ( var CHARCODE = 65500; CHARCODE < 65536; CHARCODE++ ) {
        array[item++] = new TestCase( SECTION,
                            "escape(String.fromCharCode("+CHARCODE+"))",
                            "%u"+ ToUnicodeString(CHARCODE),
                            escape(String.fromCharCode(CHARCODE))  );
    }

    return ( array );
}

function ToUnicodeString( n ) {
    var string = ToHexString(n);

    for ( var PAD = (4 - string.length ); PAD > 0; PAD-- ) {
        string = "0" + string;
    }

    return string;
}
function ToHexString( n ) {
    var hex = new Array();

    for ( var mag = 1; Math.pow(16,mag) <= n ; mag++ ) {
        ;
    }

    for ( index = 0, mag -= 1; mag > 0; index++, mag-- ) {
        hex[index] = Math.floor( n / Math.pow(16,mag) );
        n -= Math.pow(16,mag) * Math.floor( n/Math.pow(16,mag) );
    }

    hex[hex.length] = n % 16;

    var string ="";

    for ( var index = 0 ; index < hex.length ; index++ ) {
        switch ( hex[index] ) {
            case 10:
                string += "A";
                break;
            case 11:
                string += "B";
                break;
            case 12:
                string += "C";
                break;
            case 13:
                string += "D";
                break;
            case 14:
                string += "E";
                break;
            case 15:
                string += "F";
                break;
            default:
                string += hex[index];
        }
    }

    if ( string.length == 1 ) {
        string = "0" + string;
    }
    return string;
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
