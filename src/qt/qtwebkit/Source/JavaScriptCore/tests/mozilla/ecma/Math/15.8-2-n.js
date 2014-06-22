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
    File Name:          15.8-2.js
    ECMA Section:       15.8 The Math Object

    Description:

    The Math object is merely a single object that has some named properties,
    some of which are functions.

    The value of the internal [[Prototype]] property of the Math object is the
    Object prototype object (15.2.3.1).

    The Math object does not have a [[Construct]] property; it is not possible
    to use the Math object as a constructor with the new operator.

    The Math object does not have a [[Call]] property; it is not possible to
    invoke the Math object as a function.

    Recall that, in this specification, the phrase "the number value for x" has
    a technical meaning defined in section 8.5.

    Author:             christine@netscape.com
    Date:               12 november 1997

*/

    var SECTION = "15.8-2-n";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Math Object";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase( SECTION,
                                 "MYMATH = new Math()",
                                 "error",
                                 "MYMATH = new Math()" );

    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual = eval( testcases[tc].actual );

        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += "Math does not have the [Construct] property";
    }
    stopTest();
    return ( testcases );
}
