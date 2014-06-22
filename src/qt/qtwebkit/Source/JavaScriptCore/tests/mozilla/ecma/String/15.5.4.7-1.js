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
    File Name:          15.5.4.7-1.js
    ECMA Section:       15.5.4.7 String.prototype.lastIndexOf( searchString, pos)
    Description:

    If the given searchString appears as a substring of the result of
    converting this object to a string, at one or more positions that are
    at or to the left of the specified position, then the index of the
    rightmost such position is returned; otherwise -1 is returned. If position
    is undefined or not supplied, the length of this string value is assumed,
    so as to search all of the string.

    When the lastIndexOf method is called with two arguments searchString and
    position, the following steps are taken:

   1.Call ToString, giving it the this value as its argument.
   2.Call ToString(searchString).
   3.Call ToNumber(position). (If position is undefined or not supplied, this step produces the value NaN).
   4.If Result(3) is NaN, use +; otherwise, call ToInteger(Result(3)).
   5.Compute the number of characters in Result(1).
   6.Compute min(max(Result(4), 0), Result(5)).
   7.Compute the number of characters in the string that is Result(2).
   8.Compute the largest possible integer k not larger than Result(6) such that k+Result(7) is not greater
      than Result(5), and for all nonnegative integers j less than Result(7), the character at position k+j of
      Result(1) is the same as the character at position j of Result(2); but if there is no such integer k, then
      compute the value -1.

   1.Return Result(8).

    Note that the lastIndexOf function is intentionally generic; it does not require that its this value be a
    String object. Therefore it can be transferred to other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               2 october 1997
*/
    var SECTION = "15.5.4.7-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.protoype.lastIndexOf";

    var TEST_STRING = new String( " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var j = 0;

    for ( k = 0, i = 0x0021; i < 0x007e; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.lastIndexOf(" +String.fromCharCode(i)+ ", 0)",
                                 -1,
                                 TEST_STRING.lastIndexOf( String.fromCharCode(i), 0 ) );
    }

    for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.lastIndexOf("+String.fromCharCode(i)+ ", "+ k +")",
                                 k,
                                 TEST_STRING.lastIndexOf( String.fromCharCode(i), k ) );
    }

    for ( k = 0, i = 0x0020; i < 0x007e; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.lastIndexOf("+String.fromCharCode(i)+ ", "+k+1+")",
                                 k,
                                 TEST_STRING.lastIndexOf( String.fromCharCode(i), k+1 ) );
    }

    for ( k = 9, i = 0x0021; i < 0x007d; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,

                                 "String.lastIndexOf("+(String.fromCharCode(i) +
                                                    String.fromCharCode(i+1)+
                                                    String.fromCharCode(i+2)) +", "+ 0 + ")",
                                 LastIndexOf( TEST_STRING, String.fromCharCode(i) +
                                 String.fromCharCode(i+1)+String.fromCharCode(i+2), 0),
                                 TEST_STRING.lastIndexOf( (String.fromCharCode(i)+
                                                       String.fromCharCode(i+1)+
                                                       String.fromCharCode(i+2)),
                                                      0 ) );
    }

    for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.lastIndexOf("+(String.fromCharCode(i) +
                                                    String.fromCharCode(i+1)+
                                                    String.fromCharCode(i+2)) +", "+ k +")",
                                 k,
                                 TEST_STRING.lastIndexOf( (String.fromCharCode(i)+
                                                       String.fromCharCode(i+1)+
                                                       String.fromCharCode(i+2)),
                                                       k ) );
    }
    for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.lastIndexOf("+(String.fromCharCode(i) +
                                                    String.fromCharCode(i+1)+
                                                    String.fromCharCode(i+2)) +", "+ k+1 +")",
                                 k,
                                 TEST_STRING.lastIndexOf( (String.fromCharCode(i)+
                                                       String.fromCharCode(i+1)+
                                                       String.fromCharCode(i+2)),
                                                       k+1 ) );
    }
    for ( k = 0, i = 0x0020; i < 0x007d; i++, j++, k++ ) {
        array[j] = new TestCase( SECTION,
                                 "String.lastIndexOf("+
                                            (String.fromCharCode(i) +
                                            String.fromCharCode(i+1)+
                                            String.fromCharCode(i+2)) +", "+ (k-1) +")",
                                 LastIndexOf( TEST_STRING, String.fromCharCode(i) +
                                 String.fromCharCode(i+1)+String.fromCharCode(i+2), k-1),
                                 TEST_STRING.lastIndexOf( (String.fromCharCode(i)+
                                                       String.fromCharCode(i+1)+
                                                       String.fromCharCode(i+2)),
                                                       k-1 ) );
    }

    array[j++] = new TestCase( SECTION,  "String.lastIndexOf(" +TEST_STRING + ", 0 )", 0, TEST_STRING.lastIndexOf( TEST_STRING, 0 ) );
//    array[j++] = new TestCase( SECTION,  "String.lastIndexOf(" +TEST_STRING + ", 1 )", 0, TEST_STRING.lastIndexOf( TEST_STRING, 1 ));
    array[j++] = new TestCase( SECTION,  "String.lastIndexOf(" +TEST_STRING + ")", 0, TEST_STRING.lastIndexOf( TEST_STRING ));

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

function LastIndexOf( string, search, position ) {
    string = String( string );
    search = String( search );

    position = Number( position )

    if ( isNaN( position ) ) {
        position = Infinity;
    } else {
        position = ToInteger( position );
    }

    result5= string.length;
    result6 = Math.min(Math.max(position, 0), result5);
    result7 = search.length;

    if (result7 == 0) {
        return Math.min(position, result5);
    }

    result8 = -1;

    for ( k = 0; k <= result6; k++ ) {
        if ( k+ result7 > result5 ) {
            break;
        }
        for ( j = 0; j < result7; j++ ) {
            if ( string.charAt(k+j) != search.charAt(j) ){
                break;
            }   else  {
                if ( j == result7 -1 ) {
                    result8 = k;
                }
            }
        }
    }

    return result8;
}
function ToInteger( n ) {
    n = Number( n );
    if ( isNaN(n) ) {
        return 0;
    }
    if ( Math.abs(n) == 0 || Math.abs(n) == Infinity ) {
        return n;
    }

    var sign = ( n < 0 ) ? -1 : 1;

    return ( sign * Math.floor(Math.abs(n)) );
}