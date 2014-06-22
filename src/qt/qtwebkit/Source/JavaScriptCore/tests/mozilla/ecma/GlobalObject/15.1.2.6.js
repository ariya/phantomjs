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
    File Name:          15.1.2.6.js
    ECMA Section:       15.1.2.6 isNaN( x )

    Description:        Applies ToNumber to its argument, then returns true if
                        the result isNaN and otherwise returns false.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.1.2.6";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "isNaN( x )";

    var BUGNUMBER = "77391";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "isNaN.length",      1,                  isNaN.length );
    array[item++] = new TestCase( SECTION, "var MYPROPS=''; for ( var p in isNaN ) { MYPROPS+= p }; MYPROPS", "", eval("var MYPROPS=''; for ( var p in isNaN ) { MYPROPS+= p }; MYPROPS") );
    array[item++] = new TestCase( SECTION, "isNaN.length = null; isNaN.length", 1,      eval("isNaN.length=null; isNaN.length") );
    array[item++] = new TestCase( SECTION, "delete isNaN.length",               false,  delete isNaN.length );
    array[item++] = new TestCase( SECTION, "delete isNaN.length; isNaN.length", 1,      eval("delete isNaN.length; isNaN.length") );

//    array[item++] = new TestCase( SECTION, "isNaN.__proto__",   Function.prototype, isNaN.__proto__ );

    array[item++] = new TestCase( SECTION, "isNaN()",           true,               isNaN() );
    array[item++] = new TestCase( SECTION, "isNaN( null )",     false,              isNaN(null) );
    array[item++] = new TestCase( SECTION, "isNaN( void 0 )",   true,               isNaN(void 0) );
    array[item++] = new TestCase( SECTION, "isNaN( true )",     false,              isNaN(true) );
    array[item++] = new TestCase( SECTION, "isNaN( false)",     false,              isNaN(false) );
    array[item++] = new TestCase( SECTION, "isNaN( ' ' )",      false,              isNaN( " " ) );

    array[item++] = new TestCase( SECTION, "isNaN( 0 )",        false,              isNaN(0) );
    array[item++] = new TestCase( SECTION, "isNaN( 1 )",        false,              isNaN(1) );
    array[item++] = new TestCase( SECTION, "isNaN( 2 )",        false,              isNaN(2) );
    array[item++] = new TestCase( SECTION, "isNaN( 3 )",        false,              isNaN(3) );
    array[item++] = new TestCase( SECTION, "isNaN( 4 )",        false,              isNaN(4) );
    array[item++] = new TestCase( SECTION, "isNaN( 5 )",        false,              isNaN(5) );
    array[item++] = new TestCase( SECTION, "isNaN( 6 )",        false,              isNaN(6) );
    array[item++] = new TestCase( SECTION, "isNaN( 7 )",        false,              isNaN(7) );
    array[item++] = new TestCase( SECTION, "isNaN( 8 )",        false,              isNaN(8) );
    array[item++] = new TestCase( SECTION, "isNaN( 9 )",        false,              isNaN(9) );

    array[item++] = new TestCase( SECTION, "isNaN( '0' )",        false,              isNaN('0') );
    array[item++] = new TestCase( SECTION, "isNaN( '1' )",        false,              isNaN('1') );
    array[item++] = new TestCase( SECTION, "isNaN( '2' )",        false,              isNaN('2') );
    array[item++] = new TestCase( SECTION, "isNaN( '3' )",        false,              isNaN('3') );
    array[item++] = new TestCase( SECTION, "isNaN( '4' )",        false,              isNaN('4') );
    array[item++] = new TestCase( SECTION, "isNaN( '5' )",        false,              isNaN('5') );
    array[item++] = new TestCase( SECTION, "isNaN( '6' )",        false,              isNaN('6') );
    array[item++] = new TestCase( SECTION, "isNaN( '7' )",        false,              isNaN('7') );
    array[item++] = new TestCase( SECTION, "isNaN( '8' )",        false,              isNaN('8') );
    array[item++] = new TestCase( SECTION, "isNaN( '9' )",        false,              isNaN('9') );


    array[item++] = new TestCase( SECTION, "isNaN( 0x0a )",    false,              isNaN( 0x0a ) );
    array[item++] = new TestCase( SECTION, "isNaN( 0xaa )",    false,              isNaN( 0xaa ) );
    array[item++] = new TestCase( SECTION, "isNaN( 0x0A )",    false,              isNaN( 0x0A ) );
    array[item++] = new TestCase( SECTION, "isNaN( 0xAA )",    false,              isNaN( 0xAA ) );

    array[item++] = new TestCase( SECTION, "isNaN( '0x0a' )",    false,              isNaN( "0x0a" ) );
    array[item++] = new TestCase( SECTION, "isNaN( '0xaa' )",    false,              isNaN( "0xaa" ) );
    array[item++] = new TestCase( SECTION, "isNaN( '0x0A' )",    false,              isNaN( "0x0A" ) );
    array[item++] = new TestCase( SECTION, "isNaN( '0xAA' )",    false,              isNaN( "0xAA" ) );

    array[item++] = new TestCase( SECTION, "isNaN( 077 )",      false,              isNaN( 077 ) );
    array[item++] = new TestCase( SECTION, "isNaN( '077' )",    false,              isNaN( "077" ) );


    array[item++] = new TestCase( SECTION, "isNaN( Number.NaN )",   true,              isNaN(Number.NaN) );
    array[item++] = new TestCase( SECTION, "isNaN( Number.POSITIVE_INFINITY )", false,  isNaN(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "isNaN( Number.NEGATIVE_INFINITY )", false,  isNaN(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "isNaN( Number.MAX_VALUE )",         false,  isNaN(Number.MAX_VALUE) );
    array[item++] = new TestCase( SECTION, "isNaN( Number.MIN_VALUE )",         false,  isNaN(Number.MIN_VALUE) );

    array[item++] = new TestCase( SECTION, "isNaN( NaN )",               true,      isNaN(NaN) );
    array[item++] = new TestCase( SECTION, "isNaN( Infinity )",          false,     isNaN(Infinity) );

    array[item++] = new TestCase( SECTION, "isNaN( 'Infinity' )",               false,  isNaN("Infinity") );
    array[item++] = new TestCase( SECTION, "isNaN( '-Infinity' )",              false,  isNaN("-Infinity") );

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
