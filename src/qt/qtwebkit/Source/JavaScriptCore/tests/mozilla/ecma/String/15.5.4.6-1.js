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
    File Name:          15.5.4.6-1.js
    ECMA Section:       15.5.4.6 String.prototype.indexOf( searchString, pos)
    Description:        If the given searchString appears as a substring of the
                        result of converting this object to a string, at one or
                        more positions that are at or to the right of the
                        specified position, then the index of the leftmost such
                        position is returned; otherwise -1 is returned.  If
                        positionis undefined or not supplied, 0 is assumed, so
                        as to search all of the string.

                        When the indexOf method is called with two arguments,
                        searchString and pos, the following steps are taken:

                        1. Call ToString, giving it the this value as its
                           argument.
                        2. Call ToString(searchString).
                        3. Call ToInteger(position). (If position is undefined
                           or not supplied, this step produces the value 0).
                        4. Compute the number of characters in Result(1).
                        5. Compute min(max(Result(3), 0), Result(4)).
                        6. Compute the number of characters in the string that
                           is Result(2).
                        7. Compute the smallest possible integer k not smaller
                           than Result(5) such that k+Result(6) is not greater
                           than Result(4), and for all nonnegative integers j
                           less than Result(6), the character at position k+j
                           of Result(1) is the same as the character at position
                           j of Result(2); but if there is no such integer k,
                           then compute the value -1.
                        8. Return Result(7).

                        Note that the indexOf function is intentionally generic;
                        it does not require that its this value be a String object.
                        Therefore it can be transferred to other kinds of objects
                        for use as a method.

    Author:             christine@netscape.com
    Date:               2 october 1997
*/
    var SECTION = "15.5.4.6-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.protoype.indexOf";

    var TEST_STRING = new String( " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var j = 0;

    for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.indexOf(" +String.fromCharCode(i)+ ", 0)",
                                 k,
                                 TEST_STRING.indexOf( String.fromCharCode(i), 0 ) );
    }

    for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.indexOf("+String.fromCharCode(i)+ ", "+ k +")",
                                 k,
                                 TEST_STRING.indexOf( String.fromCharCode(i), k ) );
    }

    for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.indexOf("+String.fromCharCode(i)+ ", "+k+1+")",
                                 -1,
                                 TEST_STRING.indexOf( String.fromCharCode(i), k+1 ) );
    }

    for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.indexOf("+(String.fromCharCode(i) +
                                                    String.fromCharCode(i+1)+
                                                    String.fromCharCode(i+2)) +", "+0+")",
                                 k,
                                 TEST_STRING.indexOf( (String.fromCharCode(i)+
                                                       String.fromCharCode(i+1)+
                                                       String.fromCharCode(i+2)),
                                                      0 ) );
    }

    for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.indexOf("+(String.fromCharCode(i) +
                                                    String.fromCharCode(i+1)+
                                                    String.fromCharCode(i+2)) +", "+ k +")",
                                 k,
                                 TEST_STRING.indexOf( (String.fromCharCode(i)+
                                                       String.fromCharCode(i+1)+
                                                       String.fromCharCode(i+2)),
                                                       k ) );
    }
    for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.indexOf("+(String.fromCharCode(i) +
                                                    String.fromCharCode(i+1)+
                                                    String.fromCharCode(i+2)) +", "+ k+1 +")",
                                 -1,
                                 TEST_STRING.indexOf( (String.fromCharCode(i)+
                                                       String.fromCharCode(i+1)+
                                                       String.fromCharCode(i+2)),
                                                       k+1 ) );
    }

    array[j++] = new TestCase( SECTION,  "String.indexOf(" +TEST_STRING + ", 0 )", 0, TEST_STRING.indexOf( TEST_STRING, 0 ) );
    array[j++] = new TestCase( SECTION,  "String.indexOf(" +TEST_STRING + ", 1 )", -1, TEST_STRING.indexOf( TEST_STRING, 1 ));

    return array;
}

function test() {
        writeLineToLog( "TEST_STRING = new String(\" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\")" );

        for ( tc = 0; tc < testcases.length; tc++ ) {

            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+ testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value "

        }
        stopTest();

        return ( testcases );
}

