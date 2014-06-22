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
    File Name:          new-001.js
    Section:
    Description:

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=76103

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "new-001";
    var VERSION = "JS1_3";
    var TITLE   = "new-001";
    var BUGNUMBER="31567";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    function Test_One (x) {
        this.v = x+1;
        return x*2
    }

    function Test_Two( x, y ) {
        this.v = x;
        return y;
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "Test_One(18)",
        36,
        Test_One(18) );

    testcases[tc++] = new TestCase(
        SECTION,
        "new Test_One(18)",
        "[object Object]",
        new Test_One(18) +"" );

    testcases[tc++] = new TestCase(
        SECTION,
        "new Test_One(18).v",
        19,
        new Test_One(18).v );

    testcases[tc++] = new TestCase(
        SECTION,
        "Test_Two(2,7)",
        7,
        Test_Two(2,7) );

    testcases[tc++] = new TestCase(
        SECTION,
        "new Test_Two(2,7)",
        "[object Object]",
        new Test_Two(2,7) +"" );

    testcases[tc++] = new TestCase(
        SECTION,
        "new Test_Two(2,7).v",
        2,
        new Test_Two(2,7).v );

    testcases[tc++] = new TestCase(
        SECTION,
        "new (Function)(\"x\", \"return x+3\")(5,6)",
        8,
        new (Function)("x","return x+3")(5,6) );

    testcases[tc++] = new TestCase(
        SECTION,
        "new new Test_Two(String, 2).v(0123)",
        "83",
        new new Test_Two(String, 2).v(0123) +"");

    testcases[tc++] = new TestCase(
        SECTION,
        "new new Test_Two(String, 2).v(0123).length",
        2,
        new new Test_Two(String, 2).v(0123).length );

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
