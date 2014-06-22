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
                        3. Compute the number of characters  in Result(1)
                        4. If Result(2) is less than 0 is or not less than
                           Result(3), return the empty string
                        5. Return a string of length 1 containing one character
                           from result (1), the character at position Result(2).

                        Note that the charAt function is intentionally generic;
                        it does not require that its this value be a String
                        object.  Therefore it can be transferred to other kinds
                        of objects for use as a method.

    Author:             christine@netscape.com
    Date:               2 october 1997
*/
    var SECTION = "15.5.4.4-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.charAt";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(0)", "t",    eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(1)", "r",    eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(2)", "u",    eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(3)", "e",    eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(3)") );
    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(4)", "",     eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(4)") );
    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(-1)", "",    eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(-1)") );

    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(true)", "r",    eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(true)") );
    array[item++] = new TestCase( SECTION,     "x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(false)", "t",    eval("x = new Boolean(true); x.charAt=String.prototype.charAt;x.charAt(false)") );

    array[item++] = new TestCase( SECTION,     "x = new String(); x.charAt(0)",    "",     eval("x=new String();x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = new String(); x.charAt(1)",    "",     eval("x=new String();x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = new String(); x.charAt(-1)",   "",     eval("x=new String();x.charAt(-1)") );

    array[item++] = new TestCase( SECTION,     "x = new String(); x.charAt(NaN)",  "",     eval("x=new String();x.charAt(Number.NaN)") );
    array[item++] = new TestCase( SECTION,     "x = new String(); x.charAt(Number.POSITIVE_INFINITY)",   "",     eval("x=new String();x.charAt(Number.POSITIVE_INFINITY)") );
    array[item++] = new TestCase( SECTION,     "x = new String(); x.charAt(Number.NEGATIVE_INFINITY)",   "",     eval("x=new String();x.charAt(Number.NEGATIVE_INFINITY)") );

    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(0)",  "1",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(0)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(1)",  "2",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(1)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(2)",  "3",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(2)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(3)",  "4",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(3)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(4)",  "5",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(4)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(5)",  "6",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(5)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(6)",  "7",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(6)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(7)",  "8",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(7)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(8)",  "9",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(8)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(9)",  "0",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(9)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(10)",  "",       eval("var MYOB = new MyObject(1234567890); MYOB.charAt(10)") );

    array[item++] = new TestCase( SECTION,      "var MYOB = new MyObject(1234567890); MYOB.charAt(Math.PI)",  "4",        eval("var MYOB = new MyObject(1234567890); MYOB.charAt(Math.PI)") );

    // MyOtherObject.toString will return "[object Object]

    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(0)",  "[",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(0)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(1)",  "o",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(1)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(2)",  "b",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(2)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(3)",  "j",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(3)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(4)",  "e",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(4)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(5)",  "c",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(5)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(6)",  "t",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(6)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(7)",  " ",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(7)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(8)",  "O",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(8)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(9)",  "b",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(9)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(10)",  "j",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(10)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(11)",  "e",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(11)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(12)",  "c",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(12)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(13)",  "t",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(13)") );
    array[item++] = new TestCase( SECTION,      "var MYOB = new MyOtherObject(1234567890); MYOB.charAt(14)",  "]",        eval("var MYOB = new MyOtherObject(1234567890); MYOB.charAt(14)") );

    return (array );
}

function test() {
        for ( tc = 0; tc < testcases.length; tc++ ) {
            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+
                    testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed )
                                    ? ""
                                    : "wrong value "
        }

        stopTest();
        return ( testcases );
}

function MyObject( value ) {
    this.value      = value;
    this.valueOf    = new Function( "return this.value;" );
    this.toString   = new Function( "return this.value +''" );
    this.charAt     = String.prototype.charAt;
}
function MyOtherObject(value) {
    this.value      = value;
    this.valueOf    = new Function( "return this.value;" );
    this.charAt     = String.prototype.charAt;
}
