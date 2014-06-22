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
    File Name:          9.2.js
    ECMA Section:       9.2  Type Conversion:  ToBoolean
    Description:        rules for converting an argument to a boolean.
                        undefined           false
                        Null                false
                        Boolean             input argument( no conversion )
                        Number              returns false for 0, -0, and NaN
                                            otherwise return true
                        String              return false if the string is empty
                                            (length is 0) otherwise the result is
                                            true
                        Object              all return true

    Author:             christine@netscape.com
    Date:               14 july 1997
*/
    var SECTION = "9.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "ToBoolean";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    // special cases here

    testcases[tc++] = new TestCase( SECTION,   "Boolean()",                     false,  Boolean() );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(var x)",                false,  Boolean(eval("var x")) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(void 0)",               false,  Boolean(void 0) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(null)",                 false,  Boolean(null) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(false)",                false,  Boolean(false) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(true)",                 true,   Boolean(true) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(0)",                    false,  Boolean(0) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(-0)",                   false,  Boolean(-0) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(NaN)",                  false,  Boolean(Number.NaN) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean('')",                   false,  Boolean("") );

    // normal test cases here

    testcases[tc++] = new TestCase( SECTION,   "Boolean(Infinity)",             true,   Boolean(Number.POSITIVE_INFINITY) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(-Infinity)",            true,   Boolean(Number.NEGATIVE_INFINITY) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(Math.PI)",              true,   Boolean(Math.PI) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(1)",                    true,   Boolean(1) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(-1)",                   true,   Boolean(-1) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean([tab])",                true,   Boolean("\t") );
    testcases[tc++] = new TestCase( SECTION,   "Boolean('0')",                  true,   Boolean("0") );
    testcases[tc++] = new TestCase( SECTION,   "Boolean('string')",             true,   Boolean("string") );

    // ToBoolean (object) should always return true.
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new String() )",        true,   Boolean(new String()) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new String('') )",      true,   Boolean(new String("")) );

    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Boolean(true))",    true,   Boolean(new Boolean(true)) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Boolean(false))",   true,   Boolean(new Boolean(false)) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Boolean() )",       true,   Boolean(new Boolean()) );

    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Array())",          true,   Boolean(new Array()) );

    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Number())",         true,   Boolean(new Number()) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Number(-0))",       true,   Boolean(new Number(-0)) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Number(0))",        true,   Boolean(new Number(0)) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Number(NaN))",      true,   Boolean(new Number(Number.NaN)) );

    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Number(-1))",       true,   Boolean(new Number(-1)) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Number(Infinity))", true,   Boolean(new Number(Number.POSITIVE_INFINITY)) );
    testcases[tc++] = new TestCase( SECTION,   "Boolean(new Number(-Infinity))",true,   Boolean(new Number(Number.NEGATIVE_INFINITY)) );

    testcases[tc++] = new TestCase( SECTION,    "Boolean(new Object())",       true,       Boolean(new Object()) );
    testcases[tc++] = new TestCase( SECTION,    "Boolean(new Function())",     true,       Boolean(new Function()) );
    testcases[tc++] = new TestCase( SECTION,    "Boolean(new Date())",         true,       Boolean(new Date()) );
    testcases[tc++] = new TestCase( SECTION,    "Boolean(new Date(0))",         true,       Boolean(new Date(0)) );
    testcases[tc++] = new TestCase( SECTION,    "Boolean(Math)",         true,       Boolean(Math) );

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
