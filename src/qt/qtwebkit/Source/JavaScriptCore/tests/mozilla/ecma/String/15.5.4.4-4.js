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
    File Name:          15.5.4.4-4.js
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

                        This tests assiging charAt to primitive types..

    Author:             christine@netscape.com
    Date:               2 october 1997
*/
    var SECTION = "15.5.4.4-4";
    var VERSION = "ECMA_2";
    startTest();
    var TITLE   = "String.prototype.charAt";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
/*
    array[item++] = new TestCase( SECTION,     "x = null; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "n",     eval("x=null; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = null; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "u",     eval("x=null; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = null; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "l",     eval("x=null; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = null; x.__proto.charAt = String.prototype.charAt; x.charAt(3)",            "l",     eval("x=null; x.__proto__.charAt = String.prototype.charAt; x.charAt(3)") );

    array[item++] = new TestCase( SECTION,     "x = undefined; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "u",     eval("x=undefined; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = undefined; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "n",     eval("x=undefined; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = undefined; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "d",     eval("x=undefined; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = undefined; x.__proto.charAt = String.prototype.charAt; x.charAt(3)",            "e",     eval("x=undefined; x.__proto__.charAt = String.prototype.charAt; x.charAt(3)") );
*/
    array[item++] = new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "f",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "a",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "l",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(3)",            "s",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(3)") );
    array[item++] = new TestCase( SECTION,     "x = false; x.__proto.charAt = String.prototype.charAt; x.charAt(4)",            "e",     eval("x=false; x.__proto__.charAt = String.prototype.charAt; x.charAt(4)") );

    array[item++] = new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "t",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "r",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "u",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = true; x.__proto.charAt = String.prototype.charAt; x.charAt(3)",            "e",     eval("x=true; x.__proto__.charAt = String.prototype.charAt; x.charAt(3)") );

    array[item++] = new TestCase( SECTION,     "x = NaN; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "N",     eval("x=NaN; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = NaN; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "a",     eval("x=NaN; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = NaN; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "N",     eval("x=NaN; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );

    array[item++] = new TestCase( SECTION,     "x = 123; x.__proto.charAt = String.prototype.charAt; x.charAt(0)",            "1",     eval("x=123; x.__proto__.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = 123; x.__proto.charAt = String.prototype.charAt; x.charAt(1)",            "2",     eval("x=123; x.__proto__.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = 123; x.__proto.charAt = String.prototype.charAt; x.charAt(2)",            "3",     eval("x=123; x.__proto__.charAt = String.prototype.charAt; x.charAt(2)") );


    array[item++] = new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(0)",            "1",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(1)",            ",",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(2)",            "2",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(3)",            ",",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(3)") );
    array[item++] = new TestCase( SECTION,     "x = new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(4)",            "3",     eval("x=new Array(1,2,3); x.charAt = String.prototype.charAt; x.charAt(4)") );

    array[item++] = new TestCase( SECTION,  "x = new Array(); x.charAt = String.prototype.charAt; x.charAt(0)",                    "",      eval("x = new Array(); x.charAt = String.prototype.charAt; x.charAt(0)") );

    array[item++] = new TestCase( SECTION,     "x = new Number(123); x.charAt = String.prototype.charAt; x.charAt(0)",            "1",     eval("x=new Number(123); x.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = new Number(123); x.charAt = String.prototype.charAt; x.charAt(1)",            "2",     eval("x=new Number(123); x.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = new Number(123); x.charAt = String.prototype.charAt; x.charAt(2)",            "3",     eval("x=new Number(123); x.charAt = String.prototype.charAt; x.charAt(2)") );

    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(0)",            "[",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(1)",            "o",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(2)",            "b",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(3)",            "j",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(3)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(4)",            "e",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(4)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(5)",            "c",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(5)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(6)",            "t",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(6)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(7)",            " ",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(7)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(8)",            "O",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(8)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(9)",            "b",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(9)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(10)",            "j",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(10)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(11)",            "e",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(11)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(12)",            "c",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(12)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(13)",            "t",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(13)") );
    array[item++] = new TestCase( SECTION,     "x = new Object(); x.charAt = String.prototype.charAt; x.charAt(14)",            "]",     eval("x=new Object(); x.charAt = String.prototype.charAt; x.charAt(14)") );

    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(0)",            "[",    eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(0)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(1)",            "o",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(1)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(2)",            "b",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(2)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(3)",            "j",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(3)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(4)",            "e",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(4)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(5)",            "c",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(5)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(6)",            "t",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(6)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(7)",            " ",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(7)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(8)",            "F",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(8)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(9)",            "u",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(9)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(10)",            "n",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(10)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(11)",            "c",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(11)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(12)",            "t",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(12)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(13)",            "i",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(13)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(14)",            "o",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(14)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(15)",            "n",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(15)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(16)",            "]",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(16)") );
    array[item++] = new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(17)",            "",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(17)") );


    return array;
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
