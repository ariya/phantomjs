/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): 
*/

/**
 *  File Name:
 *  Reference:          http://bugzilla.mozilla.org/show_bug.cgi?id=4088
 *  Description:        Date parsing gets 12:30 AM wrong.
 *  New behavior:
 *  js> d = new Date('1/1/1999 13:30 AM')
 * Invalid Date
 * js> d = new Date('1/1/1999 13:30 PM')
 * Invalid Date
 * js> d = new Date('1/1/1999 12:30 AM')
 * Fri Jan 01 00:30:00 GMT-0800 (PST) 1999
 * js> d = new Date('1/1/1999 12:30 PM')
 * Fri Jan 01 12:30:00 GMT-0800 (PST) 1999
 *  Author:             christine@netscape.com
 */

    var SECTION = "15.9.4.2-1";       // provide a document reference (ie, ECMA section)
    var VERSION = "ECMA"; // Version of JavaScript or ECMA
    var TITLE   = "Regression Test for Date.parse";       // Provide ECMA section title or a description
    var BUGNUMBER = "http://bugzilla.mozilla.org/show_bug.cgi?id=4088";     // Provide URL to bugsplat or bugzilla report

    startTest();               // leave this alone

    AddTestCase( "new Date('1/1/1999 12:30 AM').toString()",
                new Date(1999,0,1,0,30).toString(),
                new Date('1/1/1999 12:30 AM').toString() );

    AddTestCase( "new Date('1/1/1999 12:30 PM').toString()",
                 new Date( 1999,0,1,12,30 ).toString(),
                 new Date('1/1/1999 12:30 PM').toString() );

    AddTestCase( "new Date('1/1/1999 13:30 AM')",
                "Invalid Date",
                new Date('1/1/1999 13:30 AM').toString() );


    AddTestCase( "new Date('1/1/1999 13:30 PM')",
                "Invalid Date",
                new Date('1/1/1999 13:30 PM').toString() );

    test();       // leave this alone.  this executes the test cases and
                  // displays results.
