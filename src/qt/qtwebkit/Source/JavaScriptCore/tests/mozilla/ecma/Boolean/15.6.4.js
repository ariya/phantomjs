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
    File Name:          15.6.4.js
    ECMA Section:       Properties of the Boolean Prototype Object
    Description:
    The Boolean prototype object is itself a Boolean object (its [[Class]] is "
    Boolean") whose value is false.

    The value of the internal [[Prototype]] property of the Boolean prototype
    object is the Object prototype object (15.2.3.1).

    In following descriptions of functions that are properties of the Boolean
    prototype object, the phrase "this Boolean object" refers to the object that
    is the this value for the invocation of the function; it is an error if
    this does not refer to an object for which the value of the internal
    [[Class]] property is "Boolean". Also, the phrase "this boolean value"
    refers to the boolean value represented by this Boolean object, that is,
    the value of the internal [[Value]] property of this Boolean object.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.6.4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Properties of the Boolean Prototype Object";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase( SECTION,
                                    "Boolean.prototype == false",
                                    true,
                                    Boolean.prototype == false );

    testcases[tc++] = new TestCase( SECTION,
                                    "Boolean.prototype.toString = Object.prototype.toString; Boolean.prototype.toString()",
                                    "[object Boolean]",
                                    eval("Boolean.prototype.toString = Object.prototype.toString; Boolean.prototype.toString()") );

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
