/**
    File Name:          switch_1.js
    Section:
    Description:

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=315767

    Verify that switches do not use strict equality in
    versions of JavaScript < 1.4

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
//    onerror = err;

    var SECTION = "script_1;
    var VERSION = "JS1_3";
    var TITLE   = "NativeScript";
    var BUGNUMBER="31567";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();


    testcases[tc++] = new TestCase( SECTION,


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
