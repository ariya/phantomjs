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
    File Name:          15.9.1.1-1.js
    ECMA Section:       15.9.1.1 Time Range
    Description:
                            - leap seconds are ignored
                            - assume 86400000 ms / day
                            - numbers range fom +/- 9,007,199,254,740,991
                            - ms precision for any instant that is within
                              approximately +/-285,616 years from 1 jan 1970
                              UTC
                            - range of times supported is -100,000,000 days
                              to 100,000,000 days from 1 jan 1970 12:00 am
                            - time supported is 8.64e5*10e8 milliseconds from
                              1 jan 1970 UTC  (+/-273972.6027397 years)

                        -   this test generates its own data -- it does not
                            read data from a file.
    Author:             christine@netscape.com
    Date:               7 july 1997

    Static variables:
        FOUR_HUNDRED_YEARS

*/

function test() {
    writeHeaderToLog("15.8.1.1 Time Range");

    for (  M_SECS = 0, CURRENT_YEAR = 1970;
                 M_SECS < 8640000000000000;
                tc++,   M_SECS += FOUR_HUNDRED_YEARS, CURRENT_YEAR += 400 ) {

        testcases[tc] = new TestCase( SECTION,   "new Date("+M_SECS+")", CURRENT_YEAR, (new Date( M_SECS)).getUTCFullYear() );

        testcases[tc].passed =  writeTestCaseResult(
                                testcases[tc].expect,
                                testcases[tc].actual,
                                testcases[tc].description +" = "+
                                testcases[tc].actual );

        if ( ! testcases[tc].passed ) {
            testcases[tc].reason = "wrong year value";
        }
    }

    stopTest();

    return ( testcases );
}

//  every one hundred years contains:
//    24 years with 366 days
//
//  every four hundred years contains:
//    97 years with 366 days
//   303 years with 365 days
//
//   86400000*365*97    =    3067372800000
//  +86400000*366*303   =  + 9555408000000
//                      =    1.26227808e+13
    var FOUR_HUNDRED_YEARS = 1.26227808e+13;
    var SECTION         =  "15.9.1.1-1";
    var tc              = 0;
    var testcases       = new Array();

    test();
