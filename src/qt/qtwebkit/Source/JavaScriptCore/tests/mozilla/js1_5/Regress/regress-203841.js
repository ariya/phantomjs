/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2003
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): briang@tonic.com, igor@fastmail.fm, pschwartau@netscape.com
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date:    29 April 2003
* SUMMARY: Testing merged if-clauses
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=203841
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 203841;
var summary = 'Testing merged if-clauses';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
var a = 0;
var b = 0;
var c = 0;
if (a == 5, b == 6) { c = 1; }
actual = c;
expect = 0;
addThis();

status = inSection(2);
a = 5;
b = 0;
c = 0;
if (a == 5, b == 6) { c = 1; }
actual = c;
expect = 0;
addThis();

status = inSection(3);
a = 5;
b = 6;
c = 0;
if (a == 5, b == 6) { c = 1; }
actual = c;
expect = 1;
addThis();

/*
 * Now get tricky and use the = operator inside the if-clause
 */
status = inSection(4);
a = 0;
b = 6;
c = 0;
if (a = 5, b == 6) { c = 1; }
actual = c;
expect = 1;
addThis();

status = inSection(5);
c = 0;
if (1, 1 == 6) { c = 1; }
actual = c;
expect = 0;
addThis();


/*
 * Now some tests involving arrays
 */
var x=[];

status = inSection(6); // get element case
c = 0;
if (x[1==2]) { c = 1; }
actual = c;
expect = 0;
addThis();

status = inSection(7); // set element case
c = 0;
if (x[1==2]=1) { c = 1; }
actual = c;
expect = 1;
addThis();

status = inSection(8); // delete element case
c = 0;
if (delete x[1==2]) { c = 1; }
actual = c;
expect = 1;
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
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
