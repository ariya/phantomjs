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
    File Name:          15.5.4.9-1.js
    ECMA Section:       15.5.4.9 String.prototype.substring( start )
    Description:

    15.5.4.9 String.prototype.substring(start)

    Returns a substring of the result of converting this object to a string,
    starting from character position start and running to the end of the
    string. The result is a string value, not a String object.

    If the argument is NaN or negative, it is replaced with zero; if the
    argument is larger than the length of the string, it is replaced with the
    length of the string.

    When the substring method is called with one argument start, the following
    steps are taken:

    1.Call ToString, giving it the this value as its argument.
    2.Call ToInteger(start).
    3.Compute the number of characters in Result(1).
    4.Compute min(max(Result(2), 0), Result(3)).
    5.Return a string whose length is the difference between Result(3) and Result(4),
    containing characters from Result(1), namely the characters with indices Result(4)
    through Result(3)1, in ascending order.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.5.4.9-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.substring( start )";

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
                                    "var s = new String(''); s.substring()",
                                    "",
                                    eval("var s = new String(''); s.substring()") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring()",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.substring()") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(NaN)",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.substring(NaN)") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(-0.01)",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.substring(-0.01)") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(s.length)",
                                    "",
                                    eval("var s = new String('this is a string object'); s.substring(s.length)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(s.length+1)",
                                    "",
                                    eval("var s = new String('this is a string object'); s.substring(s.length+1)") );


    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(Infinity)",
                                    "",
                                    eval("var s = new String('this is a string object'); s.substring(Infinity)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.substring(-Infinity)",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.substring(-Infinity)") );

    // this is not a String object, start is not an integer


    array[item++] = new TestCase(   SECTION,
                                    "var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring()",
                                    "1,2,3,4,5",
                                    eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring()") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true)",
                                    ",2,3,4,5",
                                    eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring(true)") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4')",
                                    "3,4,5",
                                    eval("var s = new Array(1,2,3,4,5); s.substring = String.prototype.substring; s.substring('4')") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new Array(); s.substring = String.prototype.substring; s.substring('4')",
                                    "",
                                    eval("var s = new Array(); s.substring = String.prototype.substring; s.substring('4')") );

    // this is an object object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8)",
                                    "Object]",
                                    eval("var obj = new Object(); obj.substring = String.prototype.substring; obj.substring(8)") );

    // this is a function object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8)",
                                    "Function]",
                                    eval("var obj = new Function(); obj.substring = String.prototype.substring; obj.toString = Object.prototype.toString; obj.substring(8)") );
    // this is a number object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(false)",
                                    "NaN",
                                    eval("var obj = new Number(NaN); obj.substring = String.prototype.substring; obj.substring(false)") );

    // this is the Math object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI)",
                                    "ject Math]",
                                    eval("var obj = Math; obj.substring = String.prototype.substring; obj.substring(Math.PI)") );

    // this is a Boolean object

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array())",
                                    "false",
                                    eval("var obj = new Boolean(); obj.substring = String.prototype.substring; obj.substring(new Array())") );

    // this is a user defined object

    array[item++] = new TestCase( SECTION,
                                    "var obj = new MyObject( null ); obj.substring(0)",
                                    "null",
                                    eval( "var obj = new MyObject( null ); obj.substring(0)") );

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