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
    File Name:          11.6.1-3.js
    ECMA Section:       11.6.1 The addition operator ( + )
    Description:

    The addition operator either performs string concatenation or numeric
    addition.

    The production AdditiveExpression : AdditiveExpression + MultiplicativeExpression
    is evaluated as follows:

    1.  Evaluate AdditiveExpression.
    2.  Call GetValue(Result(1)).
    3.  Evaluate MultiplicativeExpression.
    4.  Call GetValue(Result(3)).
    5.  Call ToPrimitive(Result(2)).
    6.  Call ToPrimitive(Result(4)).
    7.  If Type(Result(5)) is String or Type(Result(6)) is String, go to step 12.
        (Note that this step differs from step 3 in the algorithm for comparison
        for the relational operators in using or instead of and.)
    8.  Call ToNumber(Result(5)).
    9.  Call ToNumber(Result(6)).
    10. Apply the addition operation to Result(8) and Result(9). See the discussion below (11.6.3).
    11. Return Result(10).
    12. Call ToString(Result(5)).
    13. Call ToString(Result(6)).
    14. Concatenate Result(12) followed by Result(13).
    15. Return Result(14).

    Note that no hint is provided in the calls to ToPrimitive in steps 5 and 6.
    All native ECMAScript objects except Date objects handle the absence of a
    hint as if the hint Number were given; Date objects handle the absence of a
    hint as if the hint String were given. Host objects may handle the absence
    of a hint in some other manner.

    This test does only covers cases where the Additive or Mulplicative expression
    is a Date.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.6.1-3";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " The Addition operator ( + )");
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
function getTestCases() {
    var array = new Array();
    var item = 0;

    // tests for boolean primitive, boolean object, Object object, a "MyObject" whose value is
    // a boolean primitive and a boolean object, and "MyValuelessObject", where the value is
    // set in the object's prototype, not the object itself.

    var DATE1 = new Date();

    array[item++] = new TestCase(   SECTION,
                                    "var DATE1 = new Date(); DATE1 + DATE1",
                                    DATE1.toString() + DATE1.toString(),
                                    DATE1 + DATE1 );

    array[item++] = new TestCase(   SECTION,
                                    "var DATE1 = new Date(); DATE1 + 0",
                                    DATE1.toString() + 0,
                                    DATE1 + 0 );

    array[item++] = new TestCase(   SECTION,
                                    "var DATE1 = new Date(); DATE1 + new Number(0)",
                                    DATE1.toString() + 0,
                                    DATE1 + new Number(0) );

    array[item++] = new TestCase(   SECTION,
                                    "var DATE1 = new Date(); DATE1 + true",
                                    DATE1.toString() + "true",
                                    DATE1 + true );

    array[item++] = new TestCase(   SECTION,
                                    "var DATE1 = new Date(); DATE1 + new Boolean(true)",
                                    DATE1.toString() + "true",
                                    DATE1 + new Boolean(true) );

    array[item++] = new TestCase(   SECTION,
                                    "var DATE1 = new Date(); DATE1 + new Boolean(true)",
                                    DATE1.toString() + "true",
                                    DATE1 + new Boolean(true) );

    var MYOB1 = new MyObject( DATE1 );
    var MYOB2 = new MyValuelessObject( DATE1 );
    var MYOB3 = new MyProtolessObject( DATE1 );
    var MYOB4 = new MyProtoValuelessObject( DATE1 );

    array[item++] = new TestCase(   SECTION,
                                    "MYOB1 = new MyObject(DATE1); MYOB1 + new Number(1)",
                                    "[object Object]1",
                                    MYOB1 + new Number(1) );

    array[item++] = new TestCase(   SECTION,
                                    "MYOB1 = new MyObject(DATE1); MYOB1 + 1",
                                    "[object Object]1",
                                    MYOB1 + 1 );

    array[item++] = new TestCase(   SECTION,
                                    "MYOB2 = new MyValuelessObject(DATE1); MYOB3 + 'string'",
                                    DATE1.toString() + "string",
                                    MYOB2 + 'string' );

    array[item++] = new TestCase(   SECTION,
                                    "MYOB2 = new MyValuelessObject(DATE1); MYOB3 + new String('string')",
                                    DATE1.toString() + "string",
                                    MYOB2 + new String('string') );
/*
    array[item++] = new TestCase(   SECTION,
                                    "MYOB3 = new MyProtolessObject(DATE1); MYOB3 + new Boolean(true)",
                                    DATE1.toString() + "true",
                                    MYOB3 + new Boolean(true) );
*/
    array[item++] = new TestCase(   SECTION,
                                    "MYOB1 = new MyObject(DATE1); MYOB1 + true",
                                    "[object Object]true",
                                    MYOB1 + true );

    return ( array );
}
function MyProtoValuelessObject() {
    this.valueOf = new Function ( "" );
    this.__proto__ = null;
}
function MyProtolessObject( value ) {
    this.valueOf = new Function( "return this.value" );
    this.__proto__ = null;
    this.value = value;
}
function MyValuelessObject(value) {
    this.__proto__ = new MyPrototypeObject(value);
}
function MyPrototypeObject(value) {
    this.valueOf = new Function( "return this.value;" );
    this.toString = new Function( "return (this.value + '');" );
    this.value = value;
}
function MyObject( value ) {
    this.valueOf = new Function( "return this.value" );
    this.value = value;
}
