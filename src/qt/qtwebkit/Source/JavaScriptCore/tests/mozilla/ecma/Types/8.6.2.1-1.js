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
    File Name:          8.6.2.1-1.js
    ECMA Section:       8.6.2.1 Get (Value)
    Description:

    When the [[Get]] method of O is called with property name P, the following
    steps are taken:

    1.  If O doesn't have a property with name P, go to step 4.
    2.  Get the value of the property.
    3.  Return Result(2).
    4.  If the [[Prototype]] of O is null, return undefined.
    5.  Call the [[Get]] method of [[Prototype]] with property name P.
    6.  Return Result(5).

    This tests [[Get]] (Value).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "8.6.2.1-1";
    var VERSION = "ECMA_1";
    startTest();
    var testcases = getTestCases();

    writeHeaderToLog( SECTION + " [[Get]] (Value)");
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

    array[item++] = new TestCase( SECTION,  "var OBJ = new MyValuelessObject(true); OBJ.valueOf()",     true,           eval("var OBJ = new MyValuelessObject(true); OBJ.valueOf()") );
//    array[item++] = new TestCase( SECTION,  "var OBJ = new MyProtoValuelessObject(true); OBJ + ''",     "undefined",    eval("var OBJ = new MyProtoValuelessObject(); OBJ + ''") );
    array[item++] = new TestCase( SECTION,  "var OBJ = new MyProtolessObject(true); OBJ.valueOf()",     true,           eval("var OBJ = new MyProtolessObject(true); OBJ.valueOf()") );
    array[item++] = new TestCase( SECTION,  "var OBJ = new MyObject(true); OBJ.valueOf()",              true,           eval("var OBJ = new MyObject(true); OBJ.valueOf()") );

    array[item++] = new TestCase( SECTION,  "var OBJ = new MyValuelessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()",     Number.POSITIVE_INFINITY,           eval("var OBJ = new MyValuelessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()") );
//    array[item++] = new TestCase( SECTION,  "var OBJ = new MyProtoValuelessObject(Number.POSITIVE_INFINITY); OBJ + ''",     "undefined",                        eval("var OBJ = new MyProtoValuelessObject(); OBJ + ''") );
    array[item++] = new TestCase( SECTION,  "var OBJ = new MyProtolessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()",     Number.POSITIVE_INFINITY,           eval("var OBJ = new MyProtolessObject(Number.POSITIVE_INFINITY); OBJ.valueOf()") );
    array[item++] = new TestCase( SECTION,  "var OBJ = new MyObject(Number.POSITIVE_INFINITY); OBJ.valueOf()",              Number.POSITIVE_INFINITY,           eval("var OBJ = new MyObject(Number.POSITIVE_INFINITY); OBJ.valueOf()") );

    array[item++] = new TestCase( SECTION,  "var OBJ = new MyValuelessObject('string'); OBJ.valueOf()",     'string',           eval("var OBJ = new MyValuelessObject('string'); OBJ.valueOf()") );
//    array[item++] = new TestCase( SECTION,  "var OBJ = new MyProtoValuelessObject('string'); OJ + ''",     "undefined",      eval("var OBJ = new MyProtoValuelessObject(); OBJ + ''") );
    array[item++] = new TestCase( SECTION,  "var OBJ = new MyProtolessObject('string'); OBJ.valueOf()",     'string',           eval("var OBJ = new MyProtolessObject('string'); OBJ.valueOf()") );
    array[item++] = new TestCase( SECTION,  "var OBJ = new MyObject('string'); OBJ.valueOf()",              'string',           eval("var OBJ = new MyObject('string'); OBJ.valueOf()") );

    return ( array );
}
function MyProtoValuelessObject(value) {
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