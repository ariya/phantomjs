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
    File Name:          15.4.3.1-1.js
    ECMA Section:       15.4.3.1 Array.prototype
    Description:        The initial value of Array.prototype is the built-in
                        Array prototype object (15.4.4).

    Author:             christine@netscape.com
    Date:               7 october 1997
*/

    var SECTION = "15.4.3.1-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Array.prototype";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    var ARRAY_PROTO = Array.prototype;

    array[item++] = new TestCase( SECTION,  "var props = ''; for ( p in Array  ) { props += p } props", "", eval("var props = ''; for ( p in Array  ) { props += p } props") );

    array[item++] = new TestCase( SECTION,  "Array.prototype = null; Array.prototype",   ARRAY_PROTO, eval("Array.prototype = null; Array.prototype") );

    array[item++] = new TestCase( SECTION,  "delete Array.prototype",                   false,       delete Array.prototype );
    array[item++] = new TestCase( SECTION,  "delete Array.prototype; Array.prototype",  ARRAY_PROTO, eval("delete Array.prototype; Array.prototype") );

    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
