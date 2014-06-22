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
    File Name:          15.5.4.10-1.js
    ECMA Section:       15.5.4.10 String.prototype.substring( start, end )
    Description:

    15.5.4.10 String.prototype.substring(start, end)

    Returns a substring of the result of converting this object to a string,
    starting from character position start and running to character position
    end of the string. The result is a string value, not a String object.

    If either argument is NaN or negative, it is replaced with zero; if either
    argument is larger than the length of the string, it is replaced with the
    length of the string.

    If start is larger than end, they are swapped.

    When the substring method is called with two arguments start and end, the
    following steps are taken:

    1.  Call ToString, giving it the this value as its argument.
    2.  Call ToInteger(start).
    3.  Call ToInteger (end).
    4.  Compute the number of characters in Result(1).
    5.  Compute min(max(Result(2), 0), Result(4)).
    6.  Compute min(max(Result(3), 0), Result(4)).
    7.  Compute min(Result(5), Result(6)).
    8.  Compute max(Result(5), Result(6)).
    9.  Return a string whose length is the difference between Result(8) and
        Result(7), containing characters from Result(1), namely the characters
        with indices Result(7) through Result(8)1, in ascending order.

    Note that the substring function is intentionally generic; it does not require
    that its this value be a String object. Therefore it can be transferred to other
    kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.5.4.10-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.substring( start, end )";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "String.prototype.substring.length",        2,          String.prototype.substring.length );
    array[item++] = new TestCase( SECTION,  "delete String.prototype.substring.length", false,      delete String.prototype.substring.length );
    array[item++] = new TestCase( SECTION,  "delete String.prototype.substring.length; String.prototype.substring.length", 2,      eval("delete String.prototype.substring.length; String.prototype.substring.length") );

    // test cases for when substring is called with no arguments.

    // this is a string object

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); typeof s.substring()",
                                    "string",
                                    eval("var s = new String('this is a string object'); typeof s.substring()") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String(''); s.substring(1,0)",
                                    "",
                                    eval("var s = new String(''); s.substring(1,0)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(true, false)",
                                    "t",
                                    eval("var s = new String('this is a string object'); s.substring(false, true)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(NaN, Infinity)",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.substring(NaN, Infinity)") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(Infinity, NaN)",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.substring(Infinity, NaN)") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(Infinity, Infinity)",
                                    "",
                                    eval("var s = new String('this is a string object'); s.substring(Infinity, Infinity)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(-0.01, 0)",
                                    "",
                                    eval("var s = new String('this is a string object'); s.substring(-0.01,0)") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(s.length, s.length)",
                                    "",
                                    eval("var s = new String('this is a string object'); s.substring(s.length, s.length)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(s.length+1, 0)",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.substring(s.length+1, 0)") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(-Infinity, -Infinity)",
                                    "",
                                    eval("var s = new String('this is a string object'); s.substring(-Infinity, -Infinity)") );

    // this is not a String object, start is not an integer


    array[item++] = new TestCase(   SECTION,
                                    "var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(Infinity,-Infinity)",
                                    "1,2,3,4,5",
                                    eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(Infinity,-Infinity)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true, false)",
                                    "1",
                                    eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true, false)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4', '5')",
                                    "3",
                                    eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4', '5')") );


    // this is an object object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8,0)",
                                    "[object ",
                                    eval("var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8,0)") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8,obj.toString().length)",
                                    "Object]",
                                    eval("var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8, obj.toString().length)") );

    // this is a function object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8, Infinity)",
                                    "Function]",
                                    eval("var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8,Infinity)") );
    // this is a number object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(Infinity, NaN)",
                                    "NaN",
                                    eval("var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(Infinity, NaN)") );

    // this is the Math object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI, -10)",
                                    "[ob",
                                    eval("var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI, -10)") );

    // this is a Boolean object

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array(), new Boolean(1))",
                                    "f",
                                    eval("var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array(), new Boolean(1))") );

    // this is a user defined object

    array[item++] = new TestCase( SECTION,
                                    "var obj = new MyObject( void 0 ); obj.substring(0, 100)",
                                    "undefined",
                                    eval( "var obj = new MyObject( void 0 ); obj.substring(0,100)") );

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
function MyObject( value ) {
    this.value = value;
    this.substring = String.prototype.substring;
    this.toString = new Function ( "return this.value+''" );
}