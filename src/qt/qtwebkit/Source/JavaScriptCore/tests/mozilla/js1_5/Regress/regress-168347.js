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
* Contributor(s):  desale@netscape.com, pschwartau@netscape.com
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
* Date:    13 Sep 2002
* SUMMARY: Testing F.toString()
* See http://bugzilla.mozilla.org/show_bug.cgi?id=168347
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 168347;
var summary = "Testing F.toString()";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var FtoString = '';
var sFunc = '';

sFunc += 'function F()';
sFunc += '{';
sFunc += '  var f = arguments.callee;';
sFunc += '  f.i = 0;';
sFunc += '';
sFunc += '  try';
sFunc += '  {';
sFunc += '    f.i = f.i + 1;';
sFunc += '    print("i = i+1 succeeded \ti = " + f.i);';
sFunc += '  }';
sFunc += '  catch(e)';
sFunc += '  {';
sFunc += '    print("i = i+1 failed with " + e + "\ti = " + f.i);';
sFunc += '  }';
sFunc += '';
sFunc += '  try';
sFunc += '  {';
sFunc += '    ++f.i;';
sFunc += '    print("++i succeeded \t\ti = " + f.i);';
sFunc += '  }';
sFunc += '  catch(e)';
sFunc += '  {';
sFunc += '    print("++i failed with " + e + "\ti = " + f.i);';
sFunc += '  }';
sFunc += '';
sFunc += '  try';
sFunc += '  {';
sFunc += '    f.i++;';
sFunc += '    print("i++ succeeded \t\ti = " + f.i);';
sFunc += '  }';
sFunc += '  catch(e)';
sFunc += '  {';
sFunc += '    print("i++ failed with " + e + "\ti = " + f.i);';
sFunc += '  }';
sFunc += '';
sFunc += '  try';
sFunc += '  {';
sFunc += '    --f.i;';
sFunc += '    print("--i succeeded \t\ti = " + f.i);';
sFunc += '  }';
sFunc += '  catch(e)';
sFunc += '  {';
sFunc += '    print("--i failed with " + e + "\ti = " + f.i);';
sFunc += '  }';
sFunc += '';
sFunc += '  try';
sFunc += '  {';
sFunc += '    f.i--;';
sFunc += '    print("i-- succeeded \t\ti = " + f.i);';
sFunc += '  }';
sFunc += '  catch(e)';
sFunc += '  {';
sFunc += '    print("i-- failed with " + e + "\ti = " + f.i);';
sFunc += '  }';
sFunc += '}';


/*
 * Use sFunc to define the function F. The test
 * then rests on comparing F.toString() to sFunc.
 */
eval(sFunc);


/*
 * There are trivial whitespace differences between F.toString()
 * and sFunc. So strip out whitespace before comparing them -
 */
sFunc = stripWhite(sFunc);
FtoString = stripWhite(F.toString());


/*
 * Break comparison into sections to make any failures
 * easier for the developer to track down -
 */
status = inSection(1);
actual = FtoString.substring(0,100);
expect = sFunc.substring(0,100);
addThis();

status = inSection(2);
actual = FtoString.substring(100,200);
expect = sFunc.substring(100,200);
addThis();

status = inSection(3);
actual = FtoString.substring(200,300);
expect = sFunc.substring(200,300);
addThis();

status = inSection(4);
actual = FtoString.substring(300,400);
expect = sFunc.substring(300,400);
addThis();

status = inSection(5);
actual = FtoString.substring(400,500);
expect = sFunc.substring(400,500);
addThis();

status = inSection(6);
actual = FtoString.substring(500,600);
expect = sFunc.substring(500,600);
addThis();

status = inSection(7);
actual = FtoString.substring(600,700);
expect = sFunc.substring(600,700);
addThis();

status = inSection(8);
actual = FtoString.substring(700,800);
expect = sFunc.substring(700,800);
addThis();

status = inSection(9);
actual = FtoString.substring(800,900);
expect = sFunc.substring(800,900);
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

/*
 * Remove any whitespace characters; also
 * any escaped tabs or escaped newlines.
 */
function stripWhite(str)
{
  var re = /\s|\\t|\\n/g;
  return str.replace(re, '');
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
