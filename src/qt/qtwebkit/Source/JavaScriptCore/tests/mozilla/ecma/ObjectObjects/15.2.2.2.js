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
    File Name:          15.2.2.2.js
    ECMA Section:       15.2.2.2 new Object()
    Description:

    When the Object constructor is called with no argument, the following
    step is taken:

    1.  Create a new native ECMAScript object.
        The [[Prototype]] property of the newly constructed object is set to
        the Object prototype object.

        The [[Class]] property of the newly constructed object is set
        to "Object".

        The newly constructed object has no [[Value]] property.

        Return the newly created native object.

    Author:             christine@netscape.com
    Date:               7 october 1997
*/
    var SECTION = "15.2.2.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "new Object()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "typeof new Object()",   "object",       typeof new Object() );
    array[item++] = new TestCase( SECTION, "Object.prototype.toString()",   "[object Object]",  Object.prototype.toString() );
    array[item++] = new TestCase( SECTION, "(new Object()).toString()",  "[object Object]",   (new Object()).toString() );

    return ( array );
}
function test() {
    for ( tc = 0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}

function MyObject( value ) {
    this.value = value;
    this.valueOf = new Function( "return this.value" );
    this.toString = new Function( "return this.value+''" );
}