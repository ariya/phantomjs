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
    File Name:          7.6.js
    ECMA Section:       Punctuators
    Description:

    This tests verifies that all ECMA punctutors are recognized as a
    token separator, but does not attempt to verify the functionality
    of any punctuator.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "7.6";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Punctuators";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    // ==
    testcases[tc++] = new TestCase( SECTION,
                                    "var c,d;c==d",
                                    true,
                                    eval("var c,d;c==d") );

    // =

    testcases[tc++] = new TestCase( SECTION,
                                    "var a=true;a",
                                    true,
                                    eval("var a=true;a") );

    // >
    testcases[tc++] = new TestCase( SECTION,
                                    "var a=true,b=false;a>b",
                                    true,
                                    eval("var a=true,b=false;a>b") );

    // <
    testcases[tc++] = new TestCase( SECTION,
                                    "var a=true,b=false;a<b",
                                    false,
                                    eval("var a=true,b=false;a<b") );

    // <=
    testcases[tc++] = new TestCase( SECTION,
                                    "var a=0xFFFF,b=0X0FFF;a<=b",
                                    false,
                                    eval("var a=0xFFFF,b=0X0FFF;a<=b") );

    // >=
    testcases[tc++] = new TestCase( SECTION,
                                    "var a=0xFFFF,b=0XFFFE;a>=b",
                                    true,
                                    eval("var a=0xFFFF,b=0XFFFE;a>=b") );

    // !=
    testcases[tc++] = new TestCase( SECTION,
                                    "var a=true,b=false;a!=b",
                                    true,
                                    eval("var a=true,b=false;a!=b") );

    testcases[tc++] = new TestCase( SECTION,
                                    "var a=false,b=false;a!=b",
                                    false,
                                    eval("var a=false,b=false;a!=b") );
    // ,
        testcases[tc++] = new TestCase( SECTION,
                                    "var a=true,b=false;a,b",
                                    false,
                                    eval("var a=true,b=false;a,b") );
    // !
        testcases[tc++] = new TestCase( SECTION,
                                    "var a=true,b=false;!a",
                                    false,
                                    eval("var a=true,b=false;!a") );

    // ~
        testcases[tc++] = new TestCase( SECTION,
                                    "var a=true;~a",
                                    -2,
                                    eval("var a=true;~a") );
    // ?
        testcases[tc++] = new TestCase( SECTION,
                                    "var a=true; (a ? 'PASS' : '')",
                                    "PASS",
                                    eval("var a=true; (a ? 'PASS' : '')") );

    // :

        testcases[tc++] = new TestCase( SECTION,
                                    "var a=false; (a ? 'FAIL' : 'PASS')",
                                    "PASS",
                                    eval("var a=false; (a ? 'FAIL' : 'PASS')") );
    // .

        testcases[tc++] = new TestCase( SECTION,
                                        "var a=Number;a.NaN",
                                        NaN,
                                        eval("var a=Number;a.NaN") );

    // &&
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=true,b=true;if(a&&b)'PASS';else'FAIL'",
                                        "PASS",
                                        eval("var a=true,b=true;if(a&&b)'PASS';else'FAIL'") );

    // ||
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=false,b=false;if(a||b)'FAIL';else'PASS'",
                                        "PASS",
                                        eval("var a=false,b=false;if(a||b)'FAIL';else'PASS'") );
    // ++
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=false,b=false;++a",
                                        1,
                                        eval("var a=false,b=false;++a") );
    // --
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=true,b=false--a",
                                        0,
                                        eval("var a=true,b=false;--a") );
    // +

        testcases[tc++] = new TestCase( SECTION,
                                        "var a=true,b=true;a+b",
                                        2,
                                        eval("var a=true,b=true;a+b") );
    // -
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=true,b=true;a-b",
                                        0,
                                        eval("var a=true,b=true;a-b") );
    // *
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=true,b=true;a*b",
                                        1,
                                        eval("var a=true,b=true;a*b") );
    // /
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=true,b=true;a/b",
                                        1,
                                        eval("var a=true,b=true;a/b") );
    // &
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=3,b=2;a&b",
                                        2,
                                        eval("var a=3,b=2;a&b") );
    // |
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a|b",
                                        7,
                                        eval("var a=4,b=3;a|b") );

    // |
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a^b",
                                        7,
                                        eval("var a=4,b=3;a^b") );

    // %
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a|b",
                                        1,
                                        eval("var a=4,b=3;a%b") );

    // <<
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a<<b",
                                        32,
                                        eval("var a=4,b=3;a<<b") );

    //  >>
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=1;a>>b",
                                        2,
                                        eval("var a=4,b=1;a>>b") );

    //  >>>
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=1,b=1;a>>>b",
                                        0,
                                        eval("var a=1,b=1;a>>>b") );
    //  +=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a+=b;a",
                                        7,
                                        eval("var a=4,b=3;a+=b;a") );

    //  -=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a-=b;a",
                                        1,
                                        eval("var a=4,b=3;a-=b;a") );
    //  *=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a*=b;a",
                                        12,
                                        eval("var a=4,b=3;a*=b;a") );
    //  +=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a+=b;a",
                                        7,
                                        eval("var a=4,b=3;a+=b;a") );
    //  /=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=12,b=3;a/=b;a",
                                        4,
                                        eval("var a=12,b=3;a/=b;a") );

    //  &=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=5;a&=b;a",
                                        4,
                                        eval("var a=4,b=5;a&=b;a") );

    // |=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=5;a&=b;a",
                                        5,
                                        eval("var a=4,b=5;a|=b;a") );
    //  ^=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=5;a^=b;a",
                                        1,
                                        eval("var a=4,b=5;a^=b;a") );
    // %=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=12,b=5;a%=b;a",
                                        2,
                                        eval("var a=12,b=5;a%=b;a") );
    // <<=
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;a<<=b;a",
                                        32,
                                        eval("var a=4,b=3;a<<=b;a") );

    //  >>
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=1;a>>=b;a",
                                        2,
                                        eval("var a=4,b=1;a>>=b;a") );

    //  >>>
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=1,b=1;a>>>=b;a",
                                        0,
                                        eval("var a=1,b=1;a>>>=b;a") );

    // ()
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;(a)",
                                        4,
                                        eval("var a=4,b=3;(a)") );
    // {}
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=4,b=3;{b}",
                                        3,
                                        eval("var a=4,b=3;{b}") );

    // []
        testcases[tc++] = new TestCase( SECTION,
                                        "var a=new Array('hi');a[0]",
                                        "hi",
                                        eval("var a=new Array('hi');a[0]") );
    // []
        testcases[tc++] = new TestCase( SECTION,
                                        ";",
                                        void 0,
                                        eval(";") );
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
