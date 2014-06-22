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
    File Name:          12.6.1-1.js
    ECMA Section:       The while statement
    Description:


    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "12.6.1-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The While statement";
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase( SECTION,
                                    "var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) break; } MYVAR ",
                                    1,
                                    eval("var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) break; } MYVAR "));

    testcases[tc++] = new TestCase( SECTION,
                                    "var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) continue; else break; } MYVAR ",
                                    100,
                                    eval("var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) continue; else break; } MYVAR "));

    testcases[tc++] = new TestCase( SECTION,
                                    "function MYFUN( arg1 ) { while ( arg1++ < 100 ) { if ( arg1 < 100 ) return arg1; } }; MYFUN(1)",
                                    2,
                                    eval("function MYFUN( arg1 ) { while ( arg1++ < 100 ) { if ( arg1 < 100 ) return arg1; } }; MYFUN(1)"));
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
