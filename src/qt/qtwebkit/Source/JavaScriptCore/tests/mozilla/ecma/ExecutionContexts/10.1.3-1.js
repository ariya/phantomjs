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
    File Name:          10.1.3-1.js
    ECMA Section:       10.1.3
    Description:

    For each formal parameter, as defined in the FormalParameterList, create
    a property of the variable object whose name is the Identifier and whose
    attributes are determined by the type of code. The values of the
    parameters are supplied by the caller. If the caller supplies fewer
    parameter values than there are formal parameters, the extra formal
    parameters have value undefined. If two or more formal parameters share
    the same name, hence the same property, the corresponding property is
    given the value that was supplied for the last parameter with this name.
    If the value of this last parameter was not supplied by the caller,
    the value of the corresponding property is undefined.


    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=104191

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "10.1.3-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Variable Instantiation:  Formal Parameters";
    var BUGNUMBER="104191";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var myfun1 = new Function( "a", "a", "return a" );
    var myfun2 = new Function( "a", "b", "a", "return a" );

    function myfun3(a, b, a) {
        return a;
    }

    // myfun1, myfun2, myfun3 tostring


    testcases[tc++] = new TestCase(
        SECTION,
        String(myfun2) +"; myfun2(2,4,8)",
        8,
        myfun2(2,4,8) );

    testcases[tc++] = new TestCase(
        SECTION,
        "myfun2(2,4)",
        void 0,
        myfun2(2,4));

    testcases[tc++] = new TestCase(
        SECTION,
        String(myfun3) +"; myfun3(2,4,8)",
        8,
        myfun3(2,4,8) );

    testcases[tc++] = new TestCase(
        SECTION,
        "myfun3(2,4)",
        void 0,
        myfun3(2,4) );





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
