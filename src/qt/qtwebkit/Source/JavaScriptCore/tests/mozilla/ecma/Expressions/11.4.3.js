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
    File Name:          typeof_1.js
    ECMA Section:       11.4.3 typeof operator
    Description:        typeof evaluates unary expressions:
                        undefined   "undefined"
                        null        "object"
                        Boolean     "boolean"
                        Number      "number"
                        String      "string"
                        Object      "object" [native, doesn't implement Call]
                        Object      "function" [native, implements [Call]]
                        Object      implementation dependent
                                    [not sure how to test this]
    Author:             christine@netscape.com
    Date:               june 30, 1997

*/

    var SECTION = "11.4.3";

    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = " The typeof operator";

    var testcases = new Array();


    testcases[testcases.length] = new TestCase( SECTION,     "typeof(void(0))",              "undefined",        typeof(void(0)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(null)",                 "object",           typeof(null) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(true)",                 "boolean",          typeof(true) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(false)",                "boolean",          typeof(false) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Boolean())",        "object",           typeof(new Boolean()) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Boolean(true))",    "object",           typeof(new Boolean(true)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Boolean())",            "boolean",          typeof(Boolean()) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Boolean(false))",       "boolean",          typeof(Boolean(false)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Boolean(true))",        "boolean",          typeof(Boolean(true)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(NaN)",                  "number",           typeof(Number.NaN) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Infinity)",             "number",           typeof(Number.POSITIVE_INFINITY) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(-Infinity)",            "number",           typeof(Number.NEGATIVE_INFINITY) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Math.PI)",              "number",           typeof(Math.PI) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(0)",                    "number",           typeof(0) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(1)",                    "number",           typeof(1) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(-1)",                   "number",           typeof(-1) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof('0')",                  "string",           typeof("0") );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Number())",             "number",           typeof(Number()) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Number(0))",            "number",           typeof(Number(0)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Number(1))",            "number",           typeof(Number(1)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Nubmer(-1))",           "number",           typeof(Number(-1)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Number())",         "object",           typeof(new Number()) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Number(0))",        "object",           typeof(new Number(0)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Number(1))",        "object",           typeof(new Number(1)) );

    // Math does not implement [[Construct]] or [[Call]] so its type is object.

    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Math)",                 "object",         typeof(Math) );

    testcases[testcases.length] = new TestCase( SECTION,     "typeof(Number.prototype.toString)", "function",    typeof(Number.prototype.toString) );

    testcases[testcases.length] = new TestCase( SECTION,     "typeof('a string')",           "string",           typeof("a string") );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof('')",                   "string",           typeof("") );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Date())",           "object",           typeof(new Date()) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Array(1,2,3))",     "object",           typeof(new Array(1,2,3)) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new String('string object'))",  "object",   typeof(new String("string object")) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(String('string primitive'))",    "string",  typeof(String("string primitive")) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(['array', 'of', 'strings'])",   "object",   typeof(["array", "of", "strings"]) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(new Function())",                "function",     typeof( new Function() ) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(parseInt)",                      "function",     typeof( parseInt ) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(test)",                          "function",     typeof( test ) );
    testcases[testcases.length] = new TestCase( SECTION,     "typeof(String.fromCharCode)",           "function",     typeof( String.fromCharCode )  );


    writeHeaderToLog( SECTION + " "+ TITLE);
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
