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
* Contributor(s): pschwartau@netscape.com, rogerl@netscape.com
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
* Date:    2002-07-07
* SUMMARY: Testing JS RegExp engine against Perl 5 RegExp engine.
* Adjust cnLBOUND, cnUBOUND below to restrict which sections are tested.
*
* This test was created by running various patterns and strings through the
* Perl 5 RegExp engine. We saved the results below to test the JS engine.
*
* Each of the examples below is a negative test; that is, each produces a
* null match in Perl. Thus we set |expectedmatch| = |null| in each section.
*
* NOTE: ECMA/JS and Perl do differ on certain points. We have either commented
* out such sections altogether, or modified them to fit what we expect from JS.
*
* EXAMPLES:
*
* - ECMA does support (?: (?= and (?! operators, but doesn't support (?<  etc.
*
* - ECMA doesn't support (?(condition)
*
*/
//-----------------------------------------------------------------------------
var i = 0;
var bug = 85721;
var summary = 'Testing regular expression edge cases';
var cnSingleSpace = ' ';
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
var cnLBOUND = 0;
var cnUBOUND = 1000;


status = inSection(1);
pattern = /abc/;
string = 'xbc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(2);
pattern = /abc/;
string = 'axc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(3);
pattern = /abc/;
string = 'abx';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(4);
pattern = /ab+bc/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(5);
pattern = /ab+bc/;
string = 'abq';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(6);
pattern = /ab{1,}bc/;
string = 'abq';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(7);
pattern = /ab{4,5}bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(8);
pattern = /ab?bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(9);
pattern = /^abc$/;
string = 'abcc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(10);
pattern = /^abc$/;
string = 'aabc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(11);
pattern = /abc$/;
string = 'aabcd';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(12);
pattern = /a.*c/;
string = 'axyzd';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(13);
pattern = /a[bc]d/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(14);
pattern = /a[b-d]e/;
string = 'abd';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(15);
pattern = /a[^bc]d/;
string = 'abd';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(16);
pattern = /a[^-b]c/;
string = 'a-c';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(17);
pattern = /a[^]b]c/;
string = 'a]c';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(18);
pattern = /\by\b/;
string = 'xy';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(19);
pattern = /\by\b/;
string = 'yz';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(20);
pattern = /\by\b/;
string = 'xyz';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(21);
pattern = /\Ba\B/;
string = 'a-';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(22);
pattern = /\Ba\B/;
string = '-a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(23);
pattern = /\Ba\B/;
string = '-a-';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(24);
pattern = /\w/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(25);
pattern = /\W/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(26);
pattern = /a\sb/;
string = 'a-b';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(27);
pattern = /\d/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(28);
pattern = /\D/;
string = '1';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(29);
pattern = /[\w]/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(30);
pattern = /[\W]/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(31);
pattern = /a[\s]b/;
string = 'a-b';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(32);
pattern = /[\d]/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(33);
pattern = /[\D]/;
string = '1';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(34);
pattern = /$b/;
string = 'b';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(35);
pattern = /^(ab|cd)e/;
string = 'abcde';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(36);
pattern = /a[bcd]+dcdcde/;
string = 'adcdcde';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(37);
pattern = /(bc+d$|ef*g.|h?i(j|k))/;
string = 'effg';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(38);
pattern = /(bc+d$|ef*g.|h?i(j|k))/;
string = 'bcdd';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(39);
pattern = /[k]/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

// MODIFIED - ECMA has different rules for paren contents.
status = inSection(40);
pattern = /(a)|\1/;
string = 'x';
actualmatch = string.match(pattern);
//expectedmatch = null;
expectedmatch = Array("", undefined);
addThis();

// MODIFIED - ECMA has different rules for paren contents.
status = inSection(41);
pattern = /((\3|b)\2(a)x)+/;
string = 'aaxabxbaxbbx';
actualmatch = string.match(pattern);
//expectedmatch = null;
expectedmatch = Array("ax", "ax", "", "a");
addThis();

status = inSection(42);
pattern = /abc/i;
string = 'XBC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(43);
pattern = /abc/i;
string = 'AXC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(44);
pattern = /abc/i;
string = 'ABX';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(45);
pattern = /ab+bc/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(46);
pattern = /ab+bc/i;
string = 'ABQ';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(47);
pattern = /ab{1,}bc/i;
string = 'ABQ';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(48);
pattern = /ab{4,5}?bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(49);
pattern = /ab??bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(50);
pattern = /^abc$/i;
string = 'ABCC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(51);
pattern = /^abc$/i;
string = 'AABC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(52);
pattern = /a.*c/i;
string = 'AXYZD';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(53);
pattern = /a[bc]d/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(54);
pattern = /a[b-d]e/i;
string = 'ABD';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(55);
pattern = /a[^bc]d/i;
string = 'ABD';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(56);
pattern = /a[^-b]c/i;
string = 'A-C';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(57);
pattern = /a[^]b]c/i;
string = 'A]C';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(58);
pattern = /$b/i;
string = 'B';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(59);
pattern = /^(ab|cd)e/i;
string = 'ABCDE';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(60);
pattern = /a[bcd]+dcdcde/i;
string = 'ADCDCDE';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(61);
pattern = /(bc+d$|ef*g.|h?i(j|k))/i;
string = 'EFFG';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(62);
pattern = /(bc+d$|ef*g.|h?i(j|k))/i;
string = 'BCDD';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(63);
pattern = /[k]/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(64);
pattern = /^(a\1?){4}$/;
string = 'aaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(65);
pattern = /^(a\1?){4}$/;
string = 'aaaaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

/* ECMA doesn't support (?(
status = inSection(66);
pattern = /^(a(?(1)\1)){4}$/;
string = 'aaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(67);
pattern = /^(a(?(1)\1)){4}$/;
string = 'aaaaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();
*/

/* ECMA doesn't support (?<
status = inSection(68);
pattern = /(?<=a)b/;
string = 'cb';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(69);
pattern = /(?<=a)b/;
string = 'b';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(70);
pattern = /(?<!c)b/;
string = 'cb';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();
*/

/* ECMA doesn't support (?(condition)
status = inSection(71);
pattern = /(?:(?i)a)b/;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(72);
pattern = /((?i)a)b/;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(73);
pattern = /(?i:a)b/;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(74);
pattern = /((?i:a))b/;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(75);
pattern = /(?:(?-i)a)b/i;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(76);
pattern = /((?-i)a)b/i;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(77);
pattern = /(?:(?-i)a)b/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(78);
pattern = /((?-i)a)b/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(79);
pattern = /(?-i:a)b/i;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(80);
pattern = /((?-i:a))b/i;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(81);
pattern = /(?-i:a)b/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(82);
pattern = /((?-i:a))b/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(83);
pattern = /((?-i:a.))b/i;
string = 'a\nB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(84);
pattern = /((?s-i:a.))b/i;
string = 'B\nB';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();
*/

/* ECMA doesn't support (?<
status = inSection(85);
pattern = /(?<![cd])b/;
string = 'dbcb';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(86);
pattern = /(?<!(c|d))b/;
string = 'dbcb';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();
*/

status = inSection(87);
pattern = /^(?:a?b?)*$/;
string = 'a--';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(88);
pattern = /^b/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(89);
pattern = /()^b/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

/* ECMA doesn't support (?(
status = inSection(90);
pattern = /(?(1)a|b)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(91);
pattern = /(x)?(?(1)a|b)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(92);
pattern = /()(?(1)b|a)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(93);
pattern = /^(\()?blah(?(1)(\)))$/;
string = 'blah)';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(94);
pattern = /^(\()?blah(?(1)(\)))$/;
string = '(blah';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(95);
pattern = /^(\(+)?blah(?(1)(\)))$/;
string = 'blah)';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(96);
pattern = /^(\(+)?blah(?(1)(\)))$/;
string = '(blah';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(97);
pattern = /(?(?{0})a|b)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(98);
pattern = /(?(?{1})b|a)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(99);
pattern = /(?(?!a)a|b)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(100);
pattern = /(?(?=a)b|a)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();
*/

status = inSection(101);
pattern = /^(?=(a+?))\1ab/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(102);
pattern = /^(?=(a+?))\1ab/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(103);
pattern = /([\w:]+::)?(\w+)$/;
string = 'abcd:';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(104);
pattern = /([\w:]+::)?(\w+)$/;
string = 'abcd:';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(105);
pattern = /(>a+)ab/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(106);
pattern = /a\Z/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(107);
pattern = /a\z/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(108);
pattern = /a$/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(109);
pattern = /a\z/;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(110);
pattern = /a\z/m;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(111);
pattern = /a\z/m;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(112);
pattern = /aa\Z/;
string = 'aa\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(113);
pattern = /aa\z/;
string = 'aa\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(114);
pattern = /aa$/;
string = 'aa\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(115);
pattern = /aa\z/;
string = 'b\naa\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(116);
pattern = /aa\z/m;
string = 'aa\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(117);
pattern = /aa\z/m;
string = 'b\naa\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(118);
pattern = /aa\Z/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(119);
pattern = /aa\z/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(120);
pattern = /aa$/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(121);
pattern = /aa\Z/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(122);
pattern = /aa\z/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(123);
pattern = /aa$/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(124);
pattern = /aa\Z/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(125);
pattern = /aa\z/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(126);
pattern = /aa$/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(127);
pattern = /aa\Z/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(128);
pattern = /aa\z/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(129);
pattern = /aa$/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(130);
pattern = /aa\Z/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(131);
pattern = /aa\z/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(132);
pattern = /aa$/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(133);
pattern = /aa\Z/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(134);
pattern = /aa\z/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(135);
pattern = /aa$/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(136);
pattern = /aa\Z/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(137);
pattern = /aa\z/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(138);
pattern = /aa$/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(139);
pattern = /aa\Z/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(140);
pattern = /aa\z/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(141);
pattern = /aa$/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(142);
pattern = /aa\Z/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(143);
pattern = /aa\z/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(144);
pattern = /aa$/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(145);
pattern = /aa\Z/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(146);
pattern = /aa\z/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(147);
pattern = /aa$/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(148);
pattern = /aa\Z/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(149);
pattern = /aa\z/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(150);
pattern = /aa$/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(151);
pattern = /aa\Z/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(152);
pattern = /aa\z/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(153);
pattern = /aa$/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(154);
pattern = /ab\Z/;
string = 'ab\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(155);
pattern = /ab\z/;
string = 'ab\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(156);
pattern = /ab$/;
string = 'ab\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(157);
pattern = /ab\z/;
string = 'b\nab\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(158);
pattern = /ab\z/m;
string = 'ab\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(159);
pattern = /ab\z/m;
string = 'b\nab\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(160);
pattern = /ab\Z/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(161);
pattern = /ab\z/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(162);
pattern = /ab$/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(163);
pattern = /ab\Z/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(164);
pattern = /ab\z/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(165);
pattern = /ab$/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(166);
pattern = /ab\Z/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(167);
pattern = /ab\z/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(168);
pattern = /ab$/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(169);
pattern = /ab\Z/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(170);
pattern = /ab\z/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(171);
pattern = /ab$/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(172);
pattern = /ab\Z/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(173);
pattern = /ab\z/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(174);
pattern = /ab$/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(175);
pattern = /ab\Z/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(176);
pattern = /ab\z/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(177);
pattern = /ab$/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(178);
pattern = /ab\Z/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(179);
pattern = /ab\z/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(180);
pattern = /ab$/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(181);
pattern = /ab\Z/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(182);
pattern = /ab\z/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(183);
pattern = /ab$/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(184);
pattern = /ab\Z/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(185);
pattern = /ab\z/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(186);
pattern = /ab$/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(187);
pattern = /ab\Z/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(188);
pattern = /ab\z/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(189);
pattern = /ab$/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(190);
pattern = /ab\Z/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(191);
pattern = /ab\z/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(192);
pattern = /ab$/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(193);
pattern = /ab\Z/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(194);
pattern = /ab\z/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(195);
pattern = /ab$/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(196);
pattern = /abb\Z/;
string = 'abb\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(197);
pattern = /abb\z/;
string = 'abb\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(198);
pattern = /abb$/;
string = 'abb\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(199);
pattern = /abb\z/;
string = 'b\nabb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(200);
pattern = /abb\z/m;
string = 'abb\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(201);
pattern = /abb\z/m;
string = 'b\nabb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(202);
pattern = /abb\Z/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(203);
pattern = /abb\z/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(204);
pattern = /abb$/;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(205);
pattern = /abb\Z/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(206);
pattern = /abb\z/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(207);
pattern = /abb$/;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(208);
pattern = /abb\Z/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(209);
pattern = /abb\z/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(210);
pattern = /abb$/;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(211);
pattern = /abb\Z/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(212);
pattern = /abb\z/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(213);
pattern = /abb$/m;
string = 'ac\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(214);
pattern = /abb\Z/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(215);
pattern = /abb\z/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(216);
pattern = /abb$/m;
string = 'b\nac\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(217);
pattern = /abb\Z/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(218);
pattern = /abb\z/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(219);
pattern = /abb$/m;
string = 'b\nac';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(220);
pattern = /abb\Z/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(221);
pattern = /abb\z/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(222);
pattern = /abb$/;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(223);
pattern = /abb\Z/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(224);
pattern = /abb\z/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(225);
pattern = /abb$/;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(226);
pattern = /abb\Z/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(227);
pattern = /abb\z/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(228);
pattern = /abb$/;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(229);
pattern = /abb\Z/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(230);
pattern = /abb\z/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(231);
pattern = /abb$/m;
string = 'ca\nb\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(232);
pattern = /abb\Z/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(233);
pattern = /abb\z/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(234);
pattern = /abb$/m;
string = 'b\nca\n';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(235);
pattern = /abb\Z/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(236);
pattern = /abb\z/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(237);
pattern = /abb$/m;
string = 'b\nca';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(238);
pattern = /a*abc?xyz+pqr{3}ab{2,}xy{4,5}pq{0,6}AB{0,}zz/;
string = 'x';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(239);
pattern = /\GX.*X/;
string = 'aaaXbX';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(240);
pattern = /\.c(pp|xx|c)?$/i;
string = 'Changes';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(241);
pattern = /^([a-z]:)/;
string = 'C:/';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

status = inSection(242);
pattern = /(\w)?(abc)\1b/;
string = 'abcab';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

/* ECMA doesn't support (?(
status = inSection(243);
pattern = /^(a)?(?(1)a|b)+$/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();
*/



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function addThis()
{
  if(omitCurrentSection())
    return;

  statusmessages[i] = status;
  patterns[i] = pattern;
  strings[i] = string;
  actualmatches[i] = actualmatch;
  expectedmatches[i] = expectedmatch;
  i++;
}


function omitCurrentSection()
{
  try
  {
    // current section number is in global status variable
    var n = status.match(/(\d+)/)[1];
    return ((n < cnLBOUND) || (n > cnUBOUND));
  }
  catch(e)
  {
    return false;
  }
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
