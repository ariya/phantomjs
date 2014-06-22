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
* Contributor(s): rogerl@netscape.com, pschwartau@netscape.com
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
* Date:    15 July 2002
* SUMMARY: Testing identifiers with double-byte names
* See http://bugzilla.mozilla.org/show_bug.cgi?id=58274
*
* Here is a sample of the problem:
*
*    js> function f\u02B1 () {}
*
*    js> f\u02B1.toSource();
*    function f¦() {}
*
*    js> f\u02B1.toSource().toSource();
*    (new String("function f\xB1() {}"))
*
*
* See how the high-byte information (the 02) has been lost?
* The same thing was happening with the toString() method:
*
*    js> f\u02B1.toString();
*
*    function f¦() {
*    }
*
*    js> f\u02B1.toString().toSource();
*    (new String("\nfunction f\xB1() {\n}\n"))
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 58274;
var summary = 'Testing identifiers with double-byte names';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * Define a function that uses double-byte identifiers in
 * "every possible way"
 *
 * Then recover each double-byte identifier via f.toString().
 * To make this easier, put a 'Z' token before every one.
 *
 * Our eval string will be:
 *
 * sEval = "function Z\u02b1(Z\u02b2, b) {
 *          try { Z\u02b3 : var Z\u02b4 = Z\u02b1; }
 *          catch (Z\u02b5) { for (var Z\u02b6 in Z\u02b5)
 *          {for (1; 1<0; Z\u02b7++) {new Array()[Z\u02b6] = 1;} };} }";
 *
 * It will be helpful to build this string in stages:
 */
var s0 =  'function Z';
var s1 =  '\u02b1(Z';
var s2 =  '\u02b2, b) {try { Z';
var s3 =  '\u02b3 : var Z';
var s4 =  '\u02b4 = Z';
var s5 =  '\u02b1; } catch (Z'
var s6 =  '\u02b5) { for (var Z';
var s7 =  '\u02b6 in Z';
var s8 =  '\u02b5){for (1; 1<0; Z';
var s9 =  '\u02b7++) {new Array()[Z';
var s10 = '\u02b6] = 1;} };} }';


/*
 * Concatenate these and eval() to create the function Z\u02b1
 */
var sEval = s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10;
eval(sEval);


/*
 * Recover all the double-byte identifiers via Z\u02b1.toString().
 * We'll recover the 1st one as arrID[1], the 2nd one as arrID[2],
 * and so on ...
 */
var arrID = getIdentifiers(Z\u02b1);


/*
 * Now check that we got back what we put in -
 */
status = inSection(1);
actual = arrID[1];
expect = s1.charAt(0);
addThis();

status = inSection(2);
actual = arrID[2];
expect = s2.charAt(0);
addThis();

status = inSection(3);
actual = arrID[3];
expect = s3.charAt(0);
addThis();

status = inSection(4);
actual = arrID[4];
expect = s4.charAt(0);
addThis();

status = inSection(5);
actual = arrID[5];
expect = s5.charAt(0);
addThis();

status = inSection(6);
actual = arrID[6];
expect = s6.charAt(0);
addThis();

status = inSection(7);
actual = arrID[7];
expect = s7.charAt(0);
addThis();

status = inSection(8);
actual = arrID[8];
expect = s8.charAt(0);
addThis();

status = inSection(9);
actual = arrID[9];
expect = s9.charAt(0);
addThis();

status = inSection(10);
actual = arrID[10];
expect = s10.charAt(0);
addThis();




//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



/*
 * Goal: recover the double-byte identifiers from f.toString()
 * by getting the very next character after each 'Z' token.
 *
 * The return value will be an array |arr| indexed such that
 * |arr[1]| is the 1st identifier, |arr[2]| the 2nd, and so on.
 *
 * Note, however, f.toString() is implementation-independent.
 * For example, it may begin with '\nfunction' instead of 'function'. 
 *
 * Rhino uses a Unicode representation for f.toString(); whereas
 * SpiderMonkey uses an ASCII representation, putting escape sequences
 * for non-ASCII characters. For example, if a function is called f\u02B1,
 * then in Rhino the toString() method will present a 2-character Unicode
 * string for its name, whereas SpiderMonkey will present a 7-character
 * ASCII string for its name: the string literal 'f\u02B1'.
 *
 * So we force the lexer to condense the string before we use it.
 * This will give uniform results in Rhino and SpiderMonkey.
 */
function getIdentifiers(f)
{
  var str = condenseStr(f.toString());
  var arr = str.split('Z');

  /*
   * The identifiers are the 1st char of each split substring
   * EXCEPT the first one, which is just ('\n' +) 'function '.
   *
   * Thus note the 1st identifier will be stored in |arr[1]|,
   * the 2nd one in |arr[2]|, etc., making the indexing easy -
   */
  for (i in arr)
    arr[i] = arr[i].charAt(0);
  return arr;
}


/*
 * This function is the opposite of a functions like escape(), which take
 * Unicode characters and return escape sequences for them. Here, we force
 * the lexer to turn escape sequences back into single characters.
 *
 * Note we can't simply do |eval(str)|, since in practice |str| will be an
 * identifier somewhere in the program (e.g. a function name); thus |eval(str)|
 * would return the object that the identifier represents: not what we want.
 *
 * So we surround |str| lexicographically with quotes to force the lexer to
 * evaluate it as a string. Have to strip out any linefeeds first, however -
 */
function condenseStr(str)
{
  /*
   * You won't be able to do the next step if |str| has
   * any carriage returns or linefeeds in it. For example:
   *
   *  js> eval("'" + '\nHello' + "'");
   *  1: SyntaxError: unterminated string literal:
   *  1: '
   *  1: ^
   *
   * So replace them with the empty string -
   */
  str = str.replace(/[\r\n]/g, '') 
  return eval("'" + str + "'")
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
