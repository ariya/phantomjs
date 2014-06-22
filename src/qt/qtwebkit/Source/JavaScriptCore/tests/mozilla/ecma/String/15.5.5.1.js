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
    File Name:          15.5.5.1
    ECMA Section:       String.length
    Description:

    The number of characters in the String value represented by this String
    object.

    Once a String object is created, this property is unchanging. It has the
    attributes { DontEnum, DontDelete, ReadOnly }.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.5.5.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.length";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String(); s.length",
                                    0,
                                    eval("var s = new String(); s.length") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String(); s.length = 10; s.length",
                                    0,
                                    eval("var s = new String(); s.length = 10; s.length") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String(); var props = ''; for ( var p in s ) {  props += p; };  props",
                                    "",
                                    eval("var s = new String(); var props = ''; for ( var p in s ) {  props += p; };  props") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String(); delete s.length",
                                    false,
                                    eval("var s = new String(); delete s.length") );

    array[item++] = new TestCase(   SECTION,
                                    "var s = new String('hello'); delete s.length; s.length",
                                    5,
                                    eval("var s = new String('hello'); delete s.length; s.length") );
    return array;

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
