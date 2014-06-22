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
* Contributor(s): igor@icesoft.com, pschwartau@netscape.com
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
* Date:    10 February 2003
* SUMMARY: Object.toSource() recursion should check stack overflow
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=192465
*
* MODIFIED: 27 February 2003
*
* We are adding an early return to this testcase, since it is causing
* big problems on Linux RedHat8! For a discussion of this issue, see
* http://bugzilla.mozilla.org/show_bug.cgi?id=174341#c24 and following.
*
*
* MODIFIED: 20 March 2003
*
* Removed the early return and changed |N| below from 1000 to 90.
* Note |make_deep_nest(N)| returns an object graph of length N(N+1).
* So the graph has now been reduced from 1,001,000 to 8190.
*
* With this reduction, the bug still manifests on my WinNT and Linux
* boxes (crash due to stack overflow). So the testcase is again of use
* on those boxes. At the same time, Linux RedHat8 boxes can now run
* the test in a reasonable amount of time.
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 192465;
var summary = 'Object.toSource() recursion should check stack overflow';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * We're just testing that this script will compile and run.
 * Set both |actual| and |expect| to a dummy value.
 */
status = inSection(1);
var N = 90;
try
{
  make_deep_nest(N);
}
catch (e)
{
  // An exception is OK, as the runtime can throw one in response to too deep
  // recursion. We haven't crashed; good! Continue on to set the dummy values -
}
actual = 1;
expect = 1;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


/*
 * EXAMPLE:
 *
 * If the global variable |N| is 2, then for |level| == 0, 1, 2, the return
 * value of this function will be toSource() of these objects, respectively:
 *
 * {next:{next:END}}
 * {next:{next:{next:{next:END}}}}
 * {next:{next:{next:{next:{next:{next:END}}}}}}
 *
 */
function make_deep_nest(level)
{
  var head = {};
  var cursor = head;

  for (var i=0; i!=N; ++i)
  {
    cursor.next = {};
    cursor = cursor.next;
  }

  cursor.toSource = function()
  {
    if (level != 0)
      return make_deep_nest(level - 1);
    return "END";
  }

  return head.toSource();
}


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
