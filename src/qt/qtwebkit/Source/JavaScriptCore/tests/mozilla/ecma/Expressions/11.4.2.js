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
    File Name:          11.4.2.js
    ECMA Section:       11.4.2 the Void Operator
    Description:        always returns undefined (?)
    Author:             christine@netscape.com
    Date:               7 july 1997

*/
    var SECTION = "11.4.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The void operator";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "void(new String('string object'))",      void 0,  void(new String( 'string object' )) );
    array[item++] = new TestCase( SECTION,   "void('string primitive')",               void 0,  void("string primitive") );
    array[item++] = new TestCase( SECTION,   "void(Number.NaN)",                       void 0,  void(Number.NaN) );
    array[item++] = new TestCase( SECTION,   "void(Number.POSITIVE_INFINITY)",         void 0,  void(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "void(1)",                                void 0,  void(1) );
    array[item++] = new TestCase( SECTION,   "void(0)",                                void 0,  void(0) );
    array[item++] = new TestCase( SECTION,   "void(-1)",                               void 0,  void(-1) );
    array[item++] = new TestCase( SECTION,   "void(Number.NEGATIVE_INFINITY)",         void 0,  void(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "void(Math.PI)",                          void 0,  void(Math.PI) );
    array[item++] = new TestCase( SECTION,   "void(true)",                             void 0,  void(true) );
    array[item++] = new TestCase( SECTION,   "void(false)",                            void 0,  void(false) );
    array[item++] = new TestCase( SECTION,   "void(null)",                             void 0,  void(null) );
    array[item++] = new TestCase( SECTION,   "void new String('string object')",      void 0,  void new String( 'string object' ) );
    array[item++] = new TestCase( SECTION,   "void 'string primitive'",               void 0,  void "string primitive" );
    array[item++] = new TestCase( SECTION,   "void Number.NaN",                       void 0,  void Number.NaN );
    array[item++] = new TestCase( SECTION,   "void Number.POSITIVE_INFINITY",         void 0,  void Number.POSITIVE_INFINITY );
    array[item++] = new TestCase( SECTION,   "void 1",                                void 0,  void 1 );
    array[item++] = new TestCase( SECTION,   "void 0",                                void 0,  void 0 );
    array[item++] = new TestCase( SECTION,   "void -1",                               void 0,  void -1 );
    array[item++] = new TestCase( SECTION,   "void Number.NEGATIVE_INFINITY",         void 0,  void Number.NEGATIVE_INFINITY );
    array[item++] = new TestCase( SECTION,   "void Math.PI",                          void 0,  void Math.PI );
    array[item++] = new TestCase( SECTION,   "void true",                             void 0,  void true );
    array[item++] = new TestCase( SECTION,   "void false",                            void 0,  void false );
    array[item++] = new TestCase( SECTION,   "void null",                             void 0,  void null );

//     array[item++] = new TestCase( SECTION,   "void()",                                 void 0,  void() );

    return ( array );
}

function test() {
    for ( i = 0; i < testcases.length; i++ ) {
            testcases[i].passed = writeTestCaseResult(
                    testcases[i].expect,
                    testcases[i].actual,
                    testcases[i].description +" = "+ testcases[i].actual );
            testcases[i].reason += ( testcases[i].passed ) ? "" : "wrong value "
    }
    stopTest();
    return ( testcases );
}
