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
    File Name:          10.2.3-1.js
    ECMA Section:       10.2.3 Function and Anonymous Code
    Description:

    The scope chain is initialized to contain the activation object followed
    by the global object. Variable instantiation is performed using the
    activation by the global object. Variable instantiation is performed using
    the activation object as the variable object and using property attributes
    { DontDelete }. The caller provides the this value. If the this value
    provided by the caller is not an object (including the case where it is
    null), then the this value is the global object.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "10.2.3-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Eval Code";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var o = new MyObject("hello")

    testcases[tc++] = new TestCase( SECTION,
                                    "var o = new MyObject('hello'); o.THIS == x",
                                    true,
                                    o.THIS == o );

    var o = MyFunction();

    testcases[tc++] = new TestCase( SECTION,
                                    "var o = MyFunction(); o == this",
                                    true,
                                    o == this );

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

function MyFunction( value ) {
    return this;
}
function MyObject( value ) {
 this.THIS = this;
}
