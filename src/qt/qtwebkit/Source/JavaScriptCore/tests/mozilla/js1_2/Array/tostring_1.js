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
    File Name:          tostring_1.js
    ECMA Section:       Array.toString()
    Description:

    This checks the ToString value of Array objects under JavaScript 1.2.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "JS1_2";
    var VERSION = "JS1_2";
    startTest();
    var TITLE   = "Array.toString()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var a = new Array();

    var VERSION = 0;

    /* This test assumes that if version() exists, it can set the JavaScript 
     * interpreter to an arbitrary version. To prevent unhandled exceptions in
     * other tests, jsc implements version() as a stub function, but 
     * JavaScriptCore doesn't support setting the JavaScript engine's version.
     
     * Commenting out the following lines forces the test to expect JavaScript
     * 1.5 results.
     
     * If JavaScriptCore changes to support versioning, this test should split
     * into a 1.2 test in js1_2/ and a 1.5 test in js1_5/.
     */
    
    /*
    if ( typeof version == "function" ) {
        version(120);
        VERSION = "120";
    } else {
        function version() { return 0; };
    }
    */

    testcases[tc++] = new TestCase ( SECTION,
                        "var a = new Array(); a.toString()",
                        ( VERSION == "120" ? "[]" : "" ),
                        a.toString() );

    a[0] = void 0;

    testcases[tc++] = new TestCase ( SECTION,
                        "a[0] = void 0; a.toString()",
                        ( VERSION == "120" ? "[, ]" : "" ),
                        a.toString() );


    testcases[tc++] = new TestCase( SECTION,
                        "a.length",
                        1,
                        a.length );

    a[1] = void 0;

    testcases[tc++] = new TestCase( SECTION,
                        "a[1] = void 0; a.toString()",
                        ( VERSION == "120" ? "[, , ]" : ","  ),
                        a.toString() );

    a[1] = "hi";

    testcases[tc++] = new TestCase( SECTION,
                        "a[1] = \"hi\"; a.toString()",
                        ( VERSION == "120" ? "[, \"hi\"]" : ",hi" ),
                        a.toString() );

    a[2] = void 0;

    testcases[tc++] = new TestCase( SECTION,
                        "a[2] = void 0; a.toString()",
                        ( VERSION == "120" ?"[, \"hi\", , ]":",hi,"),
                        a.toString() );

    var b = new Array(1000);
    var bstring = "";
    for ( blen=0; blen<999; blen++) {
        bstring += ",";
    }


    testcases[tc++] = new TestCase ( SECTION,
                        "var b = new Array(1000); b.toString()",
                        ( VERSION == "120" ? "[1000]" : bstring ),
                        b.toString() );


    testcases[tc++] = new TestCase( SECTION,
                        "b.length",
                        ( VERSION == "120" ? 1 : 1000 ),
                        b.length );

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
