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
    File Name:          15.6.3.1.js
    ECMA Section:       15.6.3.1 Boolean.prototype

    Description:        The initial valu eof Boolean.prototype is the built-in
                        Boolean prototype object (15.6.4).

                        The property shall have the attributes [DontEnum,
                        DontDelete, ReadOnly ].

                        It has the internal [[Call]] and [[Construct]]
                        properties (not tested), and the length property.

    Author:             christine@netscape.com
    Date:               june 27, 1997

*/

    var SECTION = "15.6.3.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Boolean.prototype";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Boolean.prototype.valueOf()",       false,   Boolean.prototype.valueOf() );
    array[item++] = new TestCase( SECTION,  "Boolean.length",          1,       Boolean.length );

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
