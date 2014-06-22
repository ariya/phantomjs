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
    File Name:          15.6.4.2.js
    ECMA Section:       15.6.4.2-1 Boolean.prototype.toString()
    Description:        If this boolean value is true, then the string "true"
                        is returned; otherwise this boolean value must be false,
                        and the string "false" is returned.

                        The toString function is not generic; it generates
                        a runtime error if its this value is not a Boolean
                        object.  Therefore it cannot be transferred to other
                        kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               june 27, 1997
*/

    var SECTION = "15.6.4.2-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Boolean.prototype.toString()"
    writeHeaderToLog( SECTION + TITLE );

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "new Boolean(1)",       "true",   (new Boolean(1)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(0)",       "false",  (new Boolean(0)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(-1)",      "true",   (new Boolean(-1)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean('1')",     "true",   (new Boolean("1")).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean('0')",     "true",   (new Boolean("0")).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(true)",    "true",   (new Boolean(true)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(false)",   "false",  (new Boolean(false)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean('true')",  "true",   (new Boolean('true')).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean('false')", "true",   (new Boolean('false')).toString() );

    array[item++] = new TestCase( SECTION,   "new Boolean('')",      "false",  (new Boolean('')).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(null)",    "false",  (new Boolean(null)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(void(0))", "false",  (new Boolean(void(0))).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(-Infinity)", "true", (new Boolean(Number.NEGATIVE_INFINITY)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(NaN)",     "false",  (new Boolean(Number.NaN)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean()",        "false",  (new Boolean()).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(x=1)",     "true",   (new Boolean(x=1)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(x=0)",     "false",  (new Boolean(x=0)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(x=false)", "false",  (new Boolean(x=false)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(x=true)",  "true",   (new Boolean(x=true)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(x=null)",  "false",  (new Boolean(x=null)).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(x='')",    "false",  (new Boolean(x="")).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(x=' ')",   "true",   (new Boolean(x=" ")).toString() );

    array[item++] = new TestCase( SECTION,   "new Boolean(new MyObject(true))",     "true",   (new Boolean(new MyObject(true))).toString() );
    array[item++] = new TestCase( SECTION,   "new Boolean(new MyObject(false))",    "true",   (new Boolean(new MyObject(false))).toString() );

    return ( array );
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
    this.valueOf = new Function( "return this.value" );
    return this;
}