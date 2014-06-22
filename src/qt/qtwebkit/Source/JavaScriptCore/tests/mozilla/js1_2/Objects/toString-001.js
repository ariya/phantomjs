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
    File Name:          toString_1.js
    ECMA Section:       Object.toString()
    Description:

    This checks the ToString value of Object objects under JavaScript 1.2.

    In JavaScript 1.2, Object.toString()

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "JS1_2";
    var VERSION = "JS1_2";
    startTest();
    var TITLE   = "Object.toString()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var o = new Object();

    testcases[testcases.length] = new TestCase( SECTION,
        "var o = new Object(); o.toString()",
        "{}",
        o.toString() );

    o = {};

    testcases[testcases.length] = new TestCase( SECTION,
        "o = {}; o.toString()",
        "{}",
        o.toString() );

    o = { name:"object", length:0, value:"hello" }

    testcases[testcases.length] = new TestCase( SECTION,
        "o = { name:\"object\", length:0, value:\"hello\" }; o.toString()",
        true,
        checkObjectToString(o.toString(), ['name:"object"', 'length:0',
                                           'value:"hello"']));

     o = { name:"object", length:0, value:"hello",
        toString:new Function( "return this.value+''" ) }

    testcases[testcases.length] = new TestCase( SECTION,
        "o = { name:\"object\", length:0, value:\"hello\", "+
            "toString:new Function( \"return this.value+''\" ) }; o.toString()",
        "hello",
        o.toString() );



    test();

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

/**
 * checkObjectToString
 *
 * In JS1.2, Object.prototype.toString returns a representation of the 
 * object's properties as a string. However, the order of the properties
 * in the resulting string is not specified. This function compares the
 * resulting string with an array of strings to make sure that the 
 * resulting string is some permutation of the strings in the array.
 */
function checkObjectToString(s, a) {
    var m = /^\{(.*)\}$/(s);
    if (!m)
       return false;	// should begin and end with curly brackets
    var a2 = m[1].split(", ");
    if (a.length != a2.length)
        return false;	// should be same length
    a.sort();
    a2.sort();
    for (var i=0; i < a.length; i++) {
        if (a[i] != a2[i])
           return false;	// should have identical elements 
    }
    return true;
}

