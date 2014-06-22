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
    File Name:          15.4-1.js
    ECMA Section:       15.4 Array Objects

    Description:        Every Array object has a length property whose value
                        is always an integer with positive sign and less than
                        Math.pow(2,32).

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.4-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Array Objects";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr[Math.pow(2,32)-2]",
                                    "hi",
                                    eval("var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr[Math.pow(2,32)-2]")
                                );
    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr.length",
                                    (Math.pow(2,32)-1),
                                    eval("var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr.length")
                                );
    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr[Math.pow(2,32)-3]",
                                    "hi",
                                    eval("var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr[Math.pow(2,32)-3]")
                                );
    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr.length",
                                    (Math.pow(2,32)-2),
                                    eval("var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr.length")
                                );

    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr[Math.pow(2,31)-2]",
                                    "hi",
                                    eval("var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr[Math.pow(2,31)-2]")
                                );
    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr.length",
                                    (Math.pow(2,31)-1),
                                    eval("var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr.length")
                                );

    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr[Math.pow(2,31)-1]",
                                    "hi",
                                    eval("var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr[Math.pow(2,31)-1]")
                                );
    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr.length",
                                    (Math.pow(2,31)),
                                    eval("var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr.length")
                                );


    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr[Math.pow(2,31)]",
                                    "hi",
                                    eval("var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr[Math.pow(2,31)]")
                                );
    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr.length",
                                    (Math.pow(2,31)+1),
                                    eval("var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr.length")
                                );

    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr[Math.pow(2,30)-2]",
                                    "hi",
                                    eval("var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr[Math.pow(2,30)-2]")
                                );
    array[item++] = new TestCase(   SECTION,
                                    "var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr.length",
                                    (Math.pow(2,30)-1),
                                    eval("var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr.length")
                                );
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
