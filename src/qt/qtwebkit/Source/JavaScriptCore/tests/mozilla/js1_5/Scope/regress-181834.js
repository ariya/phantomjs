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
* Contributor(s): felix.meschberger@day.com, pschwartau@netscape.com
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
* Date:    25 November 2002
* SUMMARY: Testing scope
* See http://bugzilla.mozilla.org/show_bug.cgi?id=181834
*
* This bug only bit in Rhino interpreted mode, when the
* 'compile functions with dynamic scope' feature was set.
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 181834;
var summary = 'Testing scope';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * If N<=0, |outer_d| just gets incremented once,
 * so the return value should be 1 in this case.
 *
 * If N>0, we end up calling inner() N+1 times:
 * inner(N), inner(N-1), ... , inner(0).
 *
 * Each call to inner() increments |outer_d| by 1.
 * The last call, inner(0), returns the final value
 * of |outer_d|, which should be N+1.
 */
function outer(N)
{
  var outer_d = 0;
  return inner(N);

  function inner(level)
  {
    outer_d++;

    if (level > 0)
      return inner(level - 1);
    else
      return outer_d;
  }
}


/*
 * This only has meaning in Rhino -
 */
setDynamicScope(true);

/*
 * Recompile the function |outer| via eval() in order to
 * feel the effect of the dynamic scope mode we have set.
 */
var s = outer.toString();
eval(s);

status = inSection(1);
actual = outer(-5);
expect = 1;
addThis();

status = inSection(2);
actual = outer(0);
expect = 1;
addThis();

status = inSection(3);
actual = outer(5);
expect = 6;
addThis();


/*
 * Sanity check: do same steps with the dynamic flag off
 */
setDynamicScope(false);

/*
 * Recompile the function |outer| via eval() in order to
 * feel the effect of the dynamic scope mode we have set.
 */
eval(s);

status = inSection(4);
actual = outer(-5);
expect = 1;
addThis();

status = inSection(5);
actual = outer(0);
expect = 1;
addThis();

status = inSection(6);
actual = outer(5);
expect = 6;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function setDynamicScope(flag)
{
  if (this.Packages)
  {
    var cx = this.Packages.org.mozilla.javascript.Context.getCurrentContext();
    cx.setCompileFunctionsWithDynamicScope(flag);
  }
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
