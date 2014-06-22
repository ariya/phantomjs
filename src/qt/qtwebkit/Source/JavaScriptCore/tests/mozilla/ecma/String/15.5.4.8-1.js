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
    File Name:          15.5.4.8-1.js
    ECMA Section:       15.5.4.8 String.prototype.split( separator )
    Description:

    Returns an Array object into which substrings of the result of converting
    this object to a string have been stored. The substrings are determined by
    searching from left to right for occurrences of the given separator; these
    occurrences are not part of any substring in the returned array, but serve
    to divide up this string value. The separator may be a string of any length.

    As a special case, if the separator is the empty string, the string is split
    up into individual characters; the length of the result array equals the
    length of the string, and each substring contains one character.

    If the separator is not supplied, then the result array contains just one
    string, which is the string.

    Author:    christine@netscape.com, pschwartau@netscape.com
    Date:      12 November 1997
    Modified:  14 July 2002
    Reason:    See http://bugzilla.mozilla.org/show_bug.cgi?id=155289
               ECMA-262 Ed.3  Section 15.5.4.14
               The length property of the split method is 2
*
*/

    var SECTION = "15.5.4.8-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.split";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "String.prototype.split.length",        2,          String.prototype.split.length );
    array[item++] = new TestCase( SECTION,  "delete String.prototype.split.length", false,      delete String.prototype.split.length );
    array[item++] = new TestCase( SECTION,  "delete String.prototype.split.length; String.prototype.split.length", 2,      eval("delete String.prototype.split.length; String.prototype.split.length") );

    // test cases for when split is called with no arguments.

    // this is a string object

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); typeof s.split()",
                                    "object",
                                    eval("var s = new String('this is a string object'); typeof s.split()") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); Array.prototype.getClass = Object.prototype.toString; (s.split()).getClass()",
                                    "[object Array]",
                                    eval("var s = new String('this is a string object'); Array.prototype.getClass = Object.prototype.toString; (s.split()).getClass()") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.split().length",
                                    1,
                                    eval("var s = new String('this is a string object'); s.split().length") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('this is a string object'); s.split()[0]",
                                    "this is a string object",
                                    eval("var s = new String('this is a string object'); s.split()[0]") );

    // this is an object object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Object(); obj.split = String.prototype.split; typeof obj.split()",
                                    "object",
                                    eval("var obj = new Object(); obj.split = String.prototype.split; typeof obj.split()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Object(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
                                    "[object Array]",
                                    eval("var obj = new Object(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Object(); obj.split = String.prototype.split; obj.split().length",
                                    1,
                                    eval("var obj = new Object(); obj.split = String.prototype.split; obj.split().length") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Object(); obj.split = String.prototype.split; obj.split()[0]",
                                    "[object Object]",
                                    eval("var obj = new Object(); obj.split = String.prototype.split; obj.split()[0]") );

    // this is a function object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Function(); obj.split = String.prototype.split; typeof obj.split()",
                                    "object",
                                    eval("var obj = new Function(); obj.split = String.prototype.split; typeof obj.split()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Function(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
                                    "[object Array]",
                                    eval("var obj = new Function(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Function(); obj.split = String.prototype.split; obj.split().length",
                                    1,
                                    eval("var obj = new Function(); obj.split = String.prototype.split; obj.split().length") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Function(); obj.split = String.prototype.split; obj.toString = Object.prototype.toString; obj.split()[0]",
                                    "[object Function]",
                                    eval("var obj = new Function(); obj.split = String.prototype.split; obj.toString = Object.prototype.toString; obj.split()[0]") );

    // this is a number object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Number(NaN); obj.split = String.prototype.split; typeof obj.split()",
                                    "object",
                                    eval("var obj = new Number(NaN); obj.split = String.prototype.split; typeof obj.split()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Number(Infinity); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
                                    "[object Array]",
                                    eval("var obj = new Number(Infinity); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Number(-1234567890); obj.split = String.prototype.split; obj.split().length",
                                    1,
                                    eval("var obj = new Number(-1234567890); obj.split = String.prototype.split; obj.split().length") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Number(-1e21); obj.split = String.prototype.split; obj.split()[0]",
                                    "-1e+21",
                                    eval("var obj = new Number(-1e21); obj.split = String.prototype.split; obj.split()[0]") );


    // this is the Math object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = Math; obj.split = String.prototype.split; typeof obj.split()",
                                    "object",
                                    eval("var obj = Math; obj.split = String.prototype.split; typeof obj.split()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = Math; obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
                                    "[object Array]",
                                    eval("var obj = Math; obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = Math; obj.split = String.prototype.split; obj.split().length",
                                    1,
                                    eval("var obj = Math; obj.split = String.prototype.split; obj.split().length") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = Math; obj.split = String.prototype.split; obj.split()[0]",
                                    "[object Math]",
                                    eval("var obj = Math; obj.split = String.prototype.split; obj.split()[0]") );

    // this is an array object
    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; typeof obj.split()",
                                    "object",
                                    eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; typeof obj.split()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
                                    "[object Array]",
                                    eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split().length",
                                    1,
                                    eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split().length") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split()[0]",
                                    "1,2,3,4,5",
                                    eval("var obj = new Array(1,2,3,4,5); obj.split = String.prototype.split; obj.split()[0]") );

    // this is a Boolean object

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Boolean(); obj.split = String.prototype.split; typeof obj.split()",
                                    "object",
                                    eval("var obj = new Boolean(); obj.split = String.prototype.split; typeof obj.split()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Boolean(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.getClass()",
                                    "[object Array]",
                                    eval("var obj = new Boolean(); obj.split = String.prototype.split; Array.prototype.getClass = Object.prototype.toString; obj.split().getClass()") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Boolean(); obj.split = String.prototype.split; obj.split().length",
                                    1,
                                    eval("var obj = new Boolean(); obj.split = String.prototype.split; obj.split().length") );

    array[item++] = new TestCase(   SECTION,
                                    "var obj = new Boolean(); obj.split = String.prototype.split; obj.split()[0]",
                                    "false",
                                    eval("var obj = new Boolean(); obj.split = String.prototype.split; obj.split()[0]") );


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
