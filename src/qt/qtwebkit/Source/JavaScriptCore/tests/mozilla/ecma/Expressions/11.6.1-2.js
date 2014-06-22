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
    File Name:          11.6.1-2.js
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
    ToPrimitive is a string.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.6.1-2";
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

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = 'string'; var EXP_2 = false; EXP_1 + EXP_2",
                                    "stringfalse",
                                    eval("var EXP_1 = 'string'; var EXP_2 = false; EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = true; var EXP_2 = 'string'; EXP_1 + EXP_2",
                                    "truestring",
                                    eval("var EXP_1 = true; var EXP_2 = 'string'; EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new Boolean(true); var EXP_2 = new String('string'); EXP_1 + EXP_2",
                                    "truestring",
                                    eval("var EXP_1 = new Boolean(true); var EXP_2 = new String('string'); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new Object(true); var EXP_2 = new Object('string'); EXP_1 + EXP_2",
                                    "truestring",
                                    eval("var EXP_1 = new Object(true); var EXP_2 = new Object('string'); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Boolean(false)); EXP_1 + EXP_2",
                                    "stringfalse",
                                    eval("var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Boolean(false)); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyObject(true); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2",
                                    "truestring",
                                    eval("var EXP_1 = new MyObject(true); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Boolean(false)); EXP_1 + EXP_2",
                                    "[object Object][object Object]",
                                    eval("var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Boolean(false)); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyValuelessObject('string'); var EXP_2 = new MyValuelessObject(false); EXP_1 + EXP_2",
                                    "stringfalse",
                                    eval("var EXP_1 = new MyValuelessObject('string'); var EXP_2 = new MyValuelessObject(false); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Boolean(false)); EXP_1 + EXP_2",
                                    "stringfalse",
                                    eval("var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Boolean(false)); EXP_1 + EXP_2") );

    // tests for number primitive, number object, Object object, a "MyObject" whose value is
    // a number primitive and a number object, and "MyValuelessObject", where the value is
    // set in the object's prototype, not the object itself.

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = 100; var EXP_2 = 'string'; EXP_1 + EXP_2",
                                    "100string",
                                    eval("var EXP_1 = 100; var EXP_2 = 'string'; EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new String('string'); var EXP_2 = new Number(-1); EXP_1 + EXP_2",
                                    "string-1",
                                    eval("var EXP_1 = new String('string'); var EXP_2 = new Number(-1); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new Object(100); var EXP_2 = new Object('string'); EXP_1 + EXP_2",
                                    "100string",
                                    eval("var EXP_1 = new Object(100); var EXP_2 = new Object('string'); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Number(-1)); EXP_1 + EXP_2",
                                    "string-1",
                                    eval("var EXP_1 = new Object(new String('string')); var EXP_2 = new Object(new Number(-1)); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyObject(100); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2",
                                    "100string",
                                    eval("var EXP_1 = new MyObject(100); var EXP_2 = new MyObject('string'); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Number(-1)); EXP_1 + EXP_2",
                                    "[object Object][object Object]",
                                    eval("var EXP_1 = new MyObject(new String('string')); var EXP_2 = new MyObject(new Number(-1)); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyValuelessObject(100); var EXP_2 = new MyValuelessObject('string'); EXP_1 + EXP_2",
                                    "100string",
                                    eval("var EXP_1 = new MyValuelessObject(100); var EXP_2 = new MyValuelessObject('string'); EXP_1 + EXP_2") );

    array[item++] = new TestCase(   SECTION,
                                    "var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Number(-1)); EXP_1 + EXP_2",
                                    "string-1",
                                    eval("var EXP_1 = new MyValuelessObject(new String('string')); var EXP_2 = new MyValuelessObject(new Number(-1)); EXP_1 + EXP_2") );
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
