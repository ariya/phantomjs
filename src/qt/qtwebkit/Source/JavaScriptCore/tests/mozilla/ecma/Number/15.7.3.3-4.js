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
    File Name:          15.7.3.3-4.js
    ECMA Section:       15.7.3.3 Number.MIN_VALUE
    Description:        All value properties of the Number object should have
                        the attributes [DontEnum, DontDelete, ReadOnly]

                        this test checks the DontEnum attribute of Number.MIN_VALUE

    Author:             christine@netscape.com
    Date:               16 september 1997
*/


    var SECTION = "15.7.3.3-4";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Number.MIN_VALUE:  DontEnum Attribute");

    var testcases = getTestCases();
    test( testcases );

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase(
                    SECTION,
                    "var string = ''; for ( prop in Number ) { string += ( prop == 'MIN_VALUE' ) ? prop : '' } string;",
                    "",
                    eval("var string = ''; for ( prop in Number ) { string += ( prop == 'MIN_VALUE' ) ? prop : '' } string;")
                    );

    return ( array );
}

function test() {
    for ( tc = 0; tc < testcases.length; tc++ ) {
         testcases[tc].passed = writeTestCaseResult(
                        testcases[tc].expect,
                        testcases[tc].actual,
                        testcases[tc].description +" = "+
                        testcases[tc].actual );

          testcases[tc].reason += "property should not be enumerated ";
    }
    stopTest();
    return ( testcases );
}
