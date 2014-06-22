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
    File Name:          15.5.4.12-2.js
    ECMA Section:       15.5.4.12 String.prototype.toUpperCase()
    Description:

    Returns a string equal in length to the length of the result of converting
    this object to a string. The result is a string value, not a String object.

    Every character of the result is equal to the corresponding character of the
    string, unless that character has a Unicode 2.0 uppercase equivalent, in which
    case the uppercase equivalent is used instead. (The canonical Unicode 2.0 case
    mapping shall be used, which does not depend on implementation or locale.)

    Note that the toUpperCase function is intentionally generic; it does not require
    that its this value be a String object. Therefore it can be transferred to other
    kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.5.4.12-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.toUpperCase()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var TEST_STRING = "";
    var EXPECT_STRING = "";

    // basic latin test

    for ( var i = 0; i < 0x007A; i++ ) {
        var u = new Unicode(i);
        TEST_STRING += String.fromCharCode(i);
        EXPECT_STRING += String.fromCharCode( u.upper );
    }

    // don't print out the value of the strings since they contain control
    // characters that break the driver
    var isEqual = EXPECT_STRING == (new String( TEST_STRING )).toUpperCase();

    array[item++] = new TestCase( SECTION,
                                      "isEqual",
                                      true,
                                      isEqual);
    return array;
}
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
function MyObject( value ) {
    this.value = value;
    this.substring = String.prototype.substring;
    this.toString = new Function ( "return this.value+''" );
}
function Unicode( c ) {
    u = GetUnicodeValues( c );
    this.upper = u[0];
    this.lower = u[1]
    return this;
}
function GetUnicodeValues( c ) {
    u = new Array();

    u[0] = c;
    u[1] = c;

    // upper case Basic Latin

    if ( c >= 0x0041 && c <= 0x005A) {
        u[0] = c;
        u[1] = c + 32;
        return u;
    }

    // lower case Basic Latin
    if ( c >= 0x0061 && c <= 0x007a ) {
        u[0] = c - 32;
        u[1] = c;
        return u;
    }

    // upper case Latin-1 Supplement
    if ( c == 0x00B5 ) {
        u[0] = 0x039C;
        u[1] = c;
        return u;
    }
    if ( (c >= 0x00C0 && c <= 0x00D6) || (c >= 0x00D8 && c<=0x00DE) ) {
        u[0] = c;
        u[1] = c + 32;
        return u;
    }

    // lower case Latin-1 Supplement
    if ( (c >= 0x00E0 && c <= 0x00F6) || (c >= 0x00F8 && c <= 0x00FE) ) {
        u[0] = c - 32;
        u[1] = c;
        return u;
    }
    if ( c == 0x00FF ) {
        u[0] = 0x0178;
        u[1] = c;
        return u;
    }
    // Latin Extended A
    if ( (c >= 0x0100 && c < 0x0138) || (c > 0x0149 && c < 0x0178) ) {
        // special case for capital I
        if ( c == 0x0130 ) {
            u[0] = c;
            u[1] = 0x0069;
            return u;
        }
        if ( c == 0x0131 ) {
            u[0] = 0x0049;
            u[1] = c;
            return u;
        }

        if ( c % 2 == 0 ) {
        // if it's even, it's a capital and the lower case is c +1
            u[0] = c;
            u[1] = c+1;
        } else {
        // if it's odd, it's a lower case and upper case is c-1
            u[0] = c-1;
            u[1] = c;
        }
        return u;
    }
    if ( c == 0x0178 ) {
        u[0] = c;
        u[1] = 0x00FF;
        return u;
    }

    if ( (c >= 0x0139 && c < 0x0149) || (c > 0x0178 && c < 0x017F) ) {
        if ( c % 2 == 1 ) {
        // if it's odd, it's a capital and the lower case is c +1
            u[0] = c;
            u[1] = c+1;
        } else {
        // if it's even, it's a lower case and upper case is c-1
            u[0] = c-1;
            u[1] = c;
        }
        return u;
    }
    if ( c == 0x017F ) {
        u[0] = 0x0053;
        u[1] = c;
    }

    // Latin Extended B
    // need to improve this set

    if ( c >= 0x0200 && c <= 0x0217 ) {
        if ( c % 2 == 0 ) {
            u[0] = c;
            u[1] = c+1;
        } else {
            u[0] = c-1;
            u[1] = c;
        }
        return u;
    }

    // Latin Extended Additional
    // Range: U+1E00 to U+1EFF
    // http://www.unicode.org/Unicode.charts/glyphless/U1E00.html

    // Spacing Modifier Leters
    // Range: U+02B0 to U+02FF

    // Combining Diacritical Marks
    // Range: U+0300 to U+036F

    // skip Greek for now
    // Greek
    // Range: U+0370 to U+03FF

    // Cyrillic
    // Range: U+0400 to U+04FF

    if ( c >= 0x0400 && c <= 0x040F) {
        u[0] = c;
        u[1] = c + 80;
        return u;
    }


    if ( c >= 0x0410  && c <= 0x042F ) {
        u[0] = c;
        u[1] = c + 32;
        return u;
    }

    if ( c >= 0x0430 && c<= 0x044F ) {
        u[0] = c - 32;
        u[1] = c;
        return u;

    }
    if ( c >= 0x0450 && c<= 0x045F ) {
        u[0] = c -80;
        u[1] = c;
        return u;
    }

    if ( c >= 0x0460 && c <= 0x047F ) {
        if ( c % 2 == 0 ) {
            u[0] = c;
            u[1] = c +1;
        } else {
            u[0] = c - 1;
            u[1] = c;
        }
        return u;
    }

    // Armenian
    // Range: U+0530 to U+058F
    if ( c >= 0x0531 && c <= 0x0556 ) {
        u[0] = c;
        u[1] = c + 48;
        return u;
    }
    if ( c >= 0x0561 && c < 0x0587 ) {
        u[0] = c - 48;
        u[1] = c;
        return u;
    }

    // Hebrew
    // Range: U+0590 to U+05FF


    // Arabic
    // Range: U+0600 to U+06FF

    // Devanagari
    // Range: U+0900 to U+097F


    // Bengali
    // Range: U+0980 to U+09FF


    // Gurmukhi
    // Range: U+0A00 to U+0A7F


    // Gujarati
    // Range: U+0A80 to U+0AFF


    // Oriya
    // Range: U+0B00 to U+0B7F
    // no capital / lower case


    // Tamil
    // Range: U+0B80 to U+0BFF
    // no capital / lower case


    // Telugu
    // Range: U+0C00 to U+0C7F
    // no capital / lower case


    // Kannada
    // Range: U+0C80 to U+0CFF
    // no capital / lower case


    // Malayalam
    // Range: U+0D00 to U+0D7F

    // Thai
    // Range: U+0E00 to U+0E7F


    // Lao
    // Range: U+0E80 to U+0EFF


    // Tibetan
    // Range: U+0F00 to U+0FBF

    // Georgian
    // Range: U+10A0 to U+10F0

    // Hangul Jamo
    // Range: U+1100 to U+11FF

    // Greek Extended
    // Range: U+1F00 to U+1FFF
    // skip for now


    // General Punctuation
    // Range: U+2000 to U+206F

    // Superscripts and Subscripts
    // Range: U+2070 to U+209F

    // Currency Symbols
    // Range: U+20A0 to U+20CF


    // Combining Diacritical Marks for Symbols
    // Range: U+20D0 to U+20FF
    // skip for now


    // Number Forms
    // Range: U+2150 to U+218F
    // skip for now


    // Arrows
    // Range: U+2190 to U+21FF

    // Mathematical Operators
    // Range: U+2200 to U+22FF

    // Miscellaneous Technical
    // Range: U+2300 to U+23FF

    // Control Pictures
    // Range: U+2400 to U+243F

    // Optical Character Recognition
    // Range: U+2440 to U+245F

    // Enclosed Alphanumerics
    // Range: U+2460 to U+24FF

    // Box Drawing
    // Range: U+2500 to U+257F

    // Block Elements
    // Range: U+2580 to U+259F

    // Geometric Shapes
    // Range: U+25A0 to U+25FF

    // Miscellaneous Symbols
    // Range: U+2600 to U+26FF

    // Dingbats
    // Range: U+2700 to U+27BF

    // CJK Symbols and Punctuation
    // Range: U+3000 to U+303F

    // Hiragana
    // Range: U+3040 to U+309F

    // Katakana
    // Range: U+30A0 to U+30FF

    // Bopomofo
    // Range: U+3100 to U+312F

    // Hangul Compatibility Jamo
    // Range: U+3130 to U+318F

    // Kanbun
    // Range: U+3190 to U+319F


    // Enclosed CJK Letters and Months
    // Range: U+3200 to U+32FF

    // CJK Compatibility
    // Range: U+3300 to U+33FF

    // Hangul Syllables
    // Range: U+AC00 to U+D7A3

    // High Surrogates
    // Range: U+D800 to U+DB7F

    // Private Use High Surrogates
    // Range: U+DB80 to U+DBFF

    // Low Surrogates
    // Range: U+DC00 to U+DFFF

    // Private Use Area
    // Range: U+E000 to U+F8FF

    // CJK Compatibility Ideographs
    // Range: U+F900 to U+FAFF

    // Alphabetic Presentation Forms
    // Range: U+FB00 to U+FB4F

    // Arabic Presentation Forms-A
    // Range: U+FB50 to U+FDFF

    // Combining Half Marks
    // Range: U+FE20 to U+FE2F

    // CJK Compatibility Forms
    // Range: U+FE30 to U+FE4F

    // Small Form Variants
    // Range: U+FE50 to U+FE6F

    // Arabic Presentation Forms-B
    // Range: U+FE70 to U+FEFF

    // Halfwidth and Fullwidth Forms
    // Range: U+FF00 to U+FFEF

    if ( c >= 0xFF21 && c <= 0xFF3A ) {
        u[0] = c;
        u[1] = c + 32;
        return u;
    }

    if ( c >= 0xFF41 && c <= 0xFF5A ) {
        u[0] = c - 32;
        u[1] = c;
        return u;
    }

    // Specials
    // Range: U+FFF0 to U+FFFF

    return u;
}

function DecimalToHexString( n ) {
    n = Number( n );
    var h = "0x";

    for ( var i = 3; i >= 0; i-- ) {
        if ( n >= Math.pow(16, i) ){
            var t = Math.floor( n  / Math.pow(16, i));
            n -= t * Math.pow(16, i);
            if ( t >= 10 ) {
                if ( t == 10 ) {
                    h += "A";
                }
                if ( t == 11 ) {
                    h += "B";
                }
                if ( t == 12 ) {
                    h += "C";
                }
                if ( t == 13 ) {
                    h += "D";
                }
                if ( t == 14 ) {
                    h += "E";
                }
                if ( t == 15 ) {
                    h += "F";
                }
            } else {
                h += String( t );
            }
        } else {
            h += "0";
        }
    }

    return h;
}

