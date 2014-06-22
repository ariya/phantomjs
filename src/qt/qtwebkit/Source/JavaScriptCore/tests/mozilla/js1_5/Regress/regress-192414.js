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
* Date:    08 February 2003
* SUMMARY: Parser recursion should check stack overflow
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=192414
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 192414;
var summary = 'Parser recursion should check stack overflow';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

/*
 * We will form an eval string to set the result-variable |actual|.
 * To get a feel for this, suppose N were 3. Then the eval string is
 * 'actual = (1&(1&(1&1)));' The expected value after eval() is 1.
 */
status = inSection(1);
var N = 10000;
var left = repeat_str('(1&', N);
var right = repeat_str(')', N);
var str = 'actual = '.concat(left, '1', right, ';');
try
{
  eval(str);
}
catch (e)
{
  /*
   * An exception during this eval is OK, as the runtime can throw one
   * in response to too deep recursion. We haven't crashed; good!
   */
  actual = 1;
}
expect = 1;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function repeat_str(str, repeat_count)
{
  var arr = new Array(--repeat_count);
  while (repeat_count != 0)
    arr[--repeat_count] = str;
  return str.concat.apply(str, arr);
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
