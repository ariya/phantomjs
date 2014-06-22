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
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): dbaron@fas.harvard.edu, pschwartau@netscape.com
* Date: 09 October 2001
*
* SUMMARY: Regression test for Bugzilla bug 102725
* See http://bugzilla.mozilla.org/show_bug.cgi?id=102725
* "gcc -O2 problems converting numbers to strings"
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 102725;
var summary = 'Testing converting numbers to strings';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * Successive calls to foo.toString() were producing different answers!
 */
status = inSection(1);
foo = (new Date()).getTime();
actual = foo.toString();
expect = foo.toString();
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
