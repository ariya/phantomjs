/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS  IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. 
* All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com  
* Date: 06 February 2001
*
* SUMMARY:  Arose from Bugzilla bug 78156:
* "m flag of regular expression does not work with $"
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=78156
*
* The m flag means a regular expression should search strings
* across multiple lines, i.e. across '\n', '\r'.
*/
//-------------------------------------------------------------------------------------------------
var i = 0;
var bug = 78156;
var summary = 'Testing regular expressions with  ^, $, and the m flag -';
var status = '';
var statusmessages = new Array();
var pattern = '';
var patterns = new Array();
var string = '';
var strings = new Array();
var actualmatch = '';
var actualmatches = new Array();
var expectedmatch = '';
var expectedmatches = new Array();

/*
 * All patterns have an m flag; all strings are multiline.
 * Looking for digit characters at beginning/end of lines.
 */

string = 'aaa\n789\r\nccc\r\n345';
    status = inSection(1);
    pattern = /^\d/gm;
    actualmatch = string.match(pattern);
    expectedmatch = ['7','3'];
    addThis();

    status = inSection(2);
    pattern = /\d$/gm;
    actualmatch = string.match(pattern);
    expectedmatch = ['9','5'];
    addThis();

string = 'aaa\n789\r\nccc\r\nddd';
    status = inSection(3);
    pattern = /^\d/gm;
    actualmatch = string.match(pattern);
    expectedmatch = ['7'];
    addThis();

    status = inSection(4);
    pattern = /\d$/gm;
    actualmatch = string.match(pattern);
    expectedmatch = ['9'];
    addThis();



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------



function addThis()
{
  statusmessages[i] = status;
  patterns[i] = pattern;
  strings[i] = string;
  actualmatches[i] = actualmatch;
  expectedmatches[i] = expectedmatch;
  i++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
