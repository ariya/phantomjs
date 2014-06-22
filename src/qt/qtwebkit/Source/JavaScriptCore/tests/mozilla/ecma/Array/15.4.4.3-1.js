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
    File Name:    15.4.4.3-1.js
    ECMA Section: 15.4.4.3-1 Array.prototype.join()
    Description:  The elements of this object are converted to strings and
                  these strings are then concatenated, separated by comma
                  characters. The result is the same as if the built-in join
                  method were invoiked for this object with no argument.
    Author:       christine@netscape.com, pschwartau@netscape.com
    Date:         07 October 1997
    Modified:     14 July 2002
    Reason:       See http://bugzilla.mozilla.org/show_bug.cgi?id=155285
                  ECMA-262 Ed.3  Section 15.4.4.5 Array.prototype.join()
                  Step 3: If |separator| is |undefined|, let |separator|
                          be the single-character string ","
*
*/

    var SECTION = "15.4.4.3-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Array.prototype.join()");

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var ARR_PROTOTYPE = Array.prototype;

    array[item++] = new TestCase( SECTION, "Array.prototype.join.length",           1,      Array.prototype.join.length );
    array[item++] = new TestCase( SECTION, "delete Array.prototype.join.length",    false,  delete Array.prototype.join.length );
    array[item++] = new TestCase( SECTION, "delete Array.prototype.join.length; Array.prototype.join.length",    1, eval("delete Array.prototype.join.length; Array.prototype.join.length") );

    // case where array length is 0

    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(); TEST_ARRAY.join()",
                                    "",
                                    eval("var TEST_ARRAY = new Array(); TEST_ARRAY.join()") );

    // array length is 0, but spearator is specified

    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(); TEST_ARRAY.join(' ')",
                                    "",
                                    eval("var TEST_ARRAY = new Array(); TEST_ARRAY.join(' ')") );

    // length is greater than 0, separator is supplied
    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('&')",
                                    "&&true&false&123&[object Object]&true",
                                    eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('&')") );

    // length is greater than 0, separator is empty string
    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('')",
                                    "truefalse123[object Object]true",
                                    eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('')") );
    // length is greater than 0, separator is undefined
    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join(void 0)",
                                    ",,true,false,123,[object Object],true",
                                    eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join(void 0)") );

    // length is greater than 0, separator is not supplied
    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join()",
                                    ",,true,false,123,[object Object],true",
                                    eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join()") );

    // separator is a control character
    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('\v')",
                                    unescape("%u000B%u000Btrue%u000Bfalse%u000B123%u000B[object Object]%u000Btrue"),
                                    eval("var TEST_ARRAY = new Array(null, void 0, true, false, 123, new Object(), new Boolean(true) ); TEST_ARRAY.join('\v')") );

    // length of array is 1
    array[item++] = new TestCase(   SECTION,
                                    "var TEST_ARRAY = new Array(true) ); TEST_ARRAY.join('\v')",
                                    "true",
                                    eval("var TEST_ARRAY = new Array(true); TEST_ARRAY.join('\v')") );


    SEPARATOR = "\t"
    TEST_LENGTH = 100;
    TEST_STRING = "";
    ARGUMENTS = "";
    TEST_RESULT = "";

    for ( var index = 0; index < TEST_LENGTH; index++ ) {
        ARGUMENTS   += index;
        ARGUMENTS   += ( index == TEST_LENGTH -1 ) ? "" : ",";

        TEST_RESULT += index;
        TEST_RESULT += ( index == TEST_LENGTH -1 ) ? "" : SEPARATOR;
    }

    TEST_ARRAY = eval( "new Array( "+ARGUMENTS +")" );

    array[item++] = new TestCase( SECTION, "TEST_ARRAY.join("+SEPARATOR+")",   TEST_RESULT,    TEST_ARRAY.join( SEPARATOR ) );

    array[item++] = new TestCase( SECTION, "(new Array( Boolean(true), Boolean(false), null,  void 0, Number(1e+21), Number(1e-7))).join()",
                                       "true,false,,,1e+21,1e-7",
                                       (new Array( Boolean(true), Boolean(false), null,  void 0, Number(1e+21), Number(1e-7))).join() );

    // this is not an Array object
    array[item++] = new TestCase(   SECTION,
                                    "var OB = new Object_1('true,false,111,0.5,1.23e6,NaN,void 0,null'); OB.join(':')",
                                    "true:false:111:0.5:1230000:NaN::",
                                    eval("var OB = new Object_1('true,false,111,0.5,1.23e6,NaN,void 0,null'); OB.join(':')") );



    return ( array );
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
function Object_1( value ) {
    this.array = value.split(",");
    this.length = this.array.length;
    for ( var i = 0; i < this.length; i++ ) {
        this[i] = eval(this.array[i]);
    }
    this.join = Array.prototype.join;
    this.getClass = Object.prototype.toString;
}
