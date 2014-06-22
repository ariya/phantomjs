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
* Portions created by the Initial Developer are Copyright (C) 2002
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
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
* Date:    26 June 2002
* SUMMARY: Testing array.join() when separator is a variable, not a literal
* See http://bugzilla.mozilla.org/show_bug.cgi?id=154338
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 154338;
var summary = 'Test array.join() when separator is a variable, not a literal';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * Note that x === 'H' and y === 'ome'.
 *
 * Yet for some reason, using |x| or |y| as the separator
 * in arr.join() was causing out-of-memory errors, whereas
 * using the literals 'H', 'ome' was not -
 *
 */
var x = 'Home'[0];
var y = ('Home'.split('H'))[1];


status = inSection(1);
var arr = Array('a', 'b');
actual = arr.join('H');
expect = 'aHb';
addThis();

status = inSection(2);
arr = Array('a', 'b');
actual = arr.join(x);
expect = 'aHb';
addThis();

status = inSection(3);
arr = Array('a', 'b');
actual = arr.join('ome');
expect = 'aomeb';
addThis();

status = inSection(4);
arr = Array('a', 'b');
actual = arr.join(y);
expect = 'aomeb';
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
