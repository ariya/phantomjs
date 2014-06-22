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
* NOTE: ECMA/JS and Perl do differ on certain points. We have either commented
* out such sections altogether, or modified them to fit what we expect from JS.
*
* EXAMPLES:
*
* - In JS, regexp captures (/(a) etc./) must hold |undefined| if not used.
*   See http://bugzilla.mozilla.org/show_bug.cgi?id=123437.
*   By contrast, in Perl, unmatched captures hold the empty string.
*   We have modified such sections accordingly. Example:

    pattern = /^([^a-z])|(\^)$/;
    string = '.';
    actualmatch = string.match(pattern);
  //expectedmatch = Array('.', '.', '');        <<<--- Perl
    expectedmatch = Array('.', '.', undefined); <<<--- JS
    addThis();


* - In JS, you can't refer to a capture before it's encountered & completed
*
* - Perl supports ] & ^] inside a [], ECMA does not
*
* - ECMA does support (?: (?= and (?! operators, but doesn't support (?<  etc.
*
* - ECMA doesn't support (?imsx or (?-imsx
*
* - ECMA doesn't support (?(condition)
*
* - Perl has \Z has end-of-line, ECMA doesn't
*
* - In ECMA, ^ matches only the empty string before the first character
*
* - In ECMA, $ matches only the empty string at end of input (unless multiline)
*
* - ECMA spec says that each atom in a range must be a single character
*
* - ECMA doesn't support \A
*
* - ECMA doesn't have rules for [:
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
var cnLBOUND = 1;
var cnUBOUND = 1000;


status = inSection(1);
pattern = /abc/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(2);
pattern = /abc/;
string = 'xabcy';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(3);
pattern = /abc/;
string = 'ababc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(4);
pattern = /ab*c/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(5);
pattern = /ab*bc/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(6);
pattern = /ab*bc/;
string = 'abbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbc');
addThis();

status = inSection(7);
pattern = /ab*bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbbbc');
addThis();

status = inSection(8);
pattern = /.{1}/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(9);
pattern = /.{3,4}/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbb');
addThis();

status = inSection(10);
pattern = /ab{0,}bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbbbc');
addThis();

status = inSection(11);
pattern = /ab+bc/;
string = 'abbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbc');
addThis();

status = inSection(12);
pattern = /ab+bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbbbc');
addThis();

status = inSection(13);
pattern = /ab{1,}bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbbbc');
addThis();

status = inSection(14);
pattern = /ab{1,3}bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbbbc');
addThis();

status = inSection(15);
pattern = /ab{3,4}bc/;
string = 'abbbbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbbbc');
addThis();

status = inSection(16);
pattern = /ab?bc/;
string = 'abbc';
actualmatch = string.match(pattern);
expectedmatch = Array('abbc');
addThis();

status = inSection(17);
pattern = /ab?bc/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(18);
pattern = /ab{0,1}bc/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(19);
pattern = /ab?c/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(20);
pattern = /ab{0,1}c/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(21);
pattern = /^abc$/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(22);
pattern = /^abc/;
string = 'abcc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(23);
pattern = /abc$/;
string = 'aabc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(24);
pattern = /^/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(25);
pattern = /$/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(26);
pattern = /a.c/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(27);
pattern = /a.c/;
string = 'axc';
actualmatch = string.match(pattern);
expectedmatch = Array('axc');
addThis();

status = inSection(28);
pattern = /a.*c/;
string = 'axyzc';
actualmatch = string.match(pattern);
expectedmatch = Array('axyzc');
addThis();

status = inSection(29);
pattern = /a[bc]d/;
string = 'abd';
actualmatch = string.match(pattern);
expectedmatch = Array('abd');
addThis();

status = inSection(30);
pattern = /a[b-d]e/;
string = 'ace';
actualmatch = string.match(pattern);
expectedmatch = Array('ace');
addThis();

status = inSection(31);
pattern = /a[b-d]/;
string = 'aac';
actualmatch = string.match(pattern);
expectedmatch = Array('ac');
addThis();

status = inSection(32);
pattern = /a[-b]/;
string = 'a-';
actualmatch = string.match(pattern);
expectedmatch = Array('a-');
addThis();

status = inSection(33);
pattern = /a[b-]/;
string = 'a-';
actualmatch = string.match(pattern);
expectedmatch = Array('a-');
addThis();

status = inSection(34);
pattern = /a]/;
string = 'a]';
actualmatch = string.match(pattern);
expectedmatch = Array('a]');
addThis();

/* Perl supports ] & ^] inside a [], ECMA does not
pattern = /a[]]b/;
status = inSection(35);
string = 'a]b';
actualmatch = string.match(pattern);
expectedmatch = Array('a]b');
addThis();
*/

status = inSection(36);
pattern = /a[^bc]d/;
string = 'aed';
actualmatch = string.match(pattern);
expectedmatch = Array('aed');
addThis();

status = inSection(37);
pattern = /a[^-b]c/;
string = 'adc';
actualmatch = string.match(pattern);
expectedmatch = Array('adc');
addThis();

/* Perl supports ] & ^] inside a [], ECMA does not
status = inSection(38);
pattern = /a[^]b]c/;
string = 'adc';
actualmatch = string.match(pattern);
expectedmatch = Array('adc');
addThis();
*/

status = inSection(39);
pattern = /\ba\b/;
string = 'a-';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(40);
pattern = /\ba\b/;
string = '-a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(41);
pattern = /\ba\b/;
string = '-a-';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(42);
pattern = /\By\b/;
string = 'xy';
actualmatch = string.match(pattern);
expectedmatch = Array('y');
addThis();

status = inSection(43);
pattern = /\by\B/;
string = 'yz';
actualmatch = string.match(pattern);
expectedmatch = Array('y');
addThis();

status = inSection(44);
pattern = /\By\B/;
string = 'xyz';
actualmatch = string.match(pattern);
expectedmatch = Array('y');
addThis();

status = inSection(45);
pattern = /\w/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(46);
pattern = /\W/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = Array('-');
addThis();

status = inSection(47);
pattern = /a\Sb/;
string = 'a-b';
actualmatch = string.match(pattern);
expectedmatch = Array('a-b');
addThis();

status = inSection(48);
pattern = /\d/;
string = '1';
actualmatch = string.match(pattern);
expectedmatch = Array('1');
addThis();

status = inSection(49);
pattern = /\D/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = Array('-');
addThis();

status = inSection(50);
pattern = /[\w]/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(51);
pattern = /[\W]/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = Array('-');
addThis();

status = inSection(52);
pattern = /a[\S]b/;
string = 'a-b';
actualmatch = string.match(pattern);
expectedmatch = Array('a-b');
addThis();

status = inSection(53);
pattern = /[\d]/;
string = '1';
actualmatch = string.match(pattern);
expectedmatch = Array('1');
addThis();

status = inSection(54);
pattern = /[\D]/;
string = '-';
actualmatch = string.match(pattern);
expectedmatch = Array('-');
addThis();

status = inSection(55);
pattern = /ab|cd/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(56);
pattern = /ab|cd/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(57);
pattern = /()ef/;
string = 'def';
actualmatch = string.match(pattern);
expectedmatch = Array('ef', '');
addThis();

status = inSection(58);
pattern = /a\(b/;
string = 'a(b';
actualmatch = string.match(pattern);
expectedmatch = Array('a(b');
addThis();

status = inSection(59);
pattern = /a\(*b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(60);
pattern = /a\(*b/;
string = 'a((b';
actualmatch = string.match(pattern);
expectedmatch = Array('a((b');
addThis();

status = inSection(61);
pattern = /a\\b/;
string = 'a\\b';
actualmatch = string.match(pattern);
expectedmatch = Array('a\\b');
addThis();

status = inSection(62);
pattern = /((a))/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a', 'a');
addThis();

status = inSection(63);
pattern = /(a)b(c)/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc', 'a', 'c');
addThis();

status = inSection(64);
pattern = /a+b+c/;
string = 'aabbabc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(65);
pattern = /a{1,}b{1,}c/;
string = 'aabbabc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(66);
pattern = /a.+?c/;
string = 'abcabc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(67);
pattern = /(a+|b)*/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'b');
addThis();

status = inSection(68);
pattern = /(a+|b){0,}/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'b');
addThis();

status = inSection(69);
pattern = /(a+|b)+/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'b');
addThis();

status = inSection(70);
pattern = /(a+|b){1,}/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'b');
addThis();

status = inSection(71);
pattern = /(a+|b)?/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a');
addThis();

status = inSection(72);
pattern = /(a+|b){0,1}/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a');
addThis();

status = inSection(73);
pattern = /[^ab]*/;
string = 'cde';
actualmatch = string.match(pattern);
expectedmatch = Array('cde');
addThis();

status = inSection(74);
pattern = /([abc])*d/;
string = 'abbbcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abbbcd', 'c');
addThis();

status = inSection(75);
pattern = /([abc])*bcd/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abcd', 'a');
addThis();

status = inSection(76);
pattern = /a|b|c|d|e/;
string = 'e';
actualmatch = string.match(pattern);
expectedmatch = Array('e');
addThis();

status = inSection(77);
pattern = /(a|b|c|d|e)f/;
string = 'ef';
actualmatch = string.match(pattern);
expectedmatch = Array('ef', 'e');
addThis();

status = inSection(78);
pattern = /abcd*efg/;
string = 'abcdefg';
actualmatch = string.match(pattern);
expectedmatch = Array('abcdefg');
addThis();

status = inSection(79);
pattern = /ab*/;
string = 'xabyabbbz';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(80);
pattern = /ab*/;
string = 'xayabbbz';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(81);
pattern = /(ab|cd)e/;
string = 'abcde';
actualmatch = string.match(pattern);
expectedmatch = Array('cde', 'cd');
addThis();

status = inSection(82);
pattern = /[abhgefdc]ij/;
string = 'hij';
actualmatch = string.match(pattern);
expectedmatch = Array('hij');
addThis();

status = inSection(83);
pattern = /(abc|)ef/;
string = 'abcdef';
actualmatch = string.match(pattern);
expectedmatch = Array('ef', '');
addThis();

status = inSection(84);
pattern = /(a|b)c*d/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('bcd', 'b');
addThis();

status = inSection(85);
pattern = /(ab|ab*)bc/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc', 'a');
addThis();

status = inSection(86);
pattern = /a([bc]*)c*/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc', 'bc');
addThis();

status = inSection(87);
pattern = /a([bc]*)(c*d)/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abcd', 'bc', 'd');
addThis();

status = inSection(88);
pattern = /a([bc]+)(c*d)/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abcd', 'bc', 'd');
addThis();

status = inSection(89);
pattern = /a([bc]*)(c+d)/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abcd', 'b', 'cd');
addThis();

status = inSection(90);
pattern = /a[bcd]*dcdcde/;
string = 'adcdcde';
actualmatch = string.match(pattern);
expectedmatch = Array('adcdcde');
addThis();

status = inSection(91);
pattern = /(ab|a)b*c/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc', 'ab');
addThis();

status = inSection(92);
pattern = /((a)(b)c)(d)/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abcd', 'abc', 'a', 'b', 'd');
addThis();

status = inSection(93);
pattern = /[a-zA-Z_][a-zA-Z0-9_]*/;
string = 'alpha';
actualmatch = string.match(pattern);
expectedmatch = Array('alpha');
addThis();

status = inSection(94);
pattern = /^a(bc+|b[eh])g|.h$/;
string = 'abh';
actualmatch = string.match(pattern);
expectedmatch = Array('bh', undefined);
addThis();

status = inSection(95);
pattern = /(bc+d$|ef*g.|h?i(j|k))/;
string = 'effgz';
actualmatch = string.match(pattern);
expectedmatch = Array('effgz', 'effgz', undefined);
addThis();

status = inSection(96);
pattern = /(bc+d$|ef*g.|h?i(j|k))/;
string = 'ij';
actualmatch = string.match(pattern);
expectedmatch = Array('ij', 'ij', 'j');
addThis();

status = inSection(97);
pattern = /(bc+d$|ef*g.|h?i(j|k))/;
string = 'reffgz';
actualmatch = string.match(pattern);
expectedmatch = Array('effgz', 'effgz', undefined);
addThis();

status = inSection(98);
pattern = /((((((((((a))))))))))/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a');
addThis();

status = inSection(99);
pattern = /((((((((((a))))))))))\10/;
string = 'aa';
actualmatch = string.match(pattern);
expectedmatch = Array('aa', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a');
addThis();

status = inSection(100);
pattern = /((((((((((a))))))))))/;
string = 'a!';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a');
addThis();

status = inSection(101);
pattern = /(((((((((a)))))))))/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a');
addThis();

status = inSection(102);
pattern = /(.*)c(.*)/;
string = 'abcde';
actualmatch = string.match(pattern);
expectedmatch = Array('abcde', 'ab', 'de');
addThis();

status = inSection(103);
pattern = /abcd/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abcd');
addThis();

status = inSection(104);
pattern = /a(bc)d/;
string = 'abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('abcd', 'bc');
addThis();

status = inSection(105);
pattern = /a[-]?c/;
string = 'ac';
actualmatch = string.match(pattern);
expectedmatch = Array('ac');
addThis();

status = inSection(106);
pattern = /(abc)\1/;
string = 'abcabc';
actualmatch = string.match(pattern);
expectedmatch = Array('abcabc', 'abc');
addThis();

status = inSection(107);
pattern = /([a-c]*)\1/;
string = 'abcabc';
actualmatch = string.match(pattern);
expectedmatch = Array('abcabc', 'abc');
addThis();

status = inSection(108);
pattern = /(a)|\1/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a', 'a');
addThis();

status = inSection(109);
pattern = /(([a-c])b*?\2)*/;
string = 'ababbbcbc';
actualmatch = string.match(pattern);
expectedmatch = Array('ababb', 'bb', 'b');
addThis();

status = inSection(110);
pattern = /(([a-c])b*?\2){3}/;
string = 'ababbbcbc';
actualmatch = string.match(pattern);
expectedmatch = Array('ababbbcbc', 'cbc', 'c');
addThis();

/* Can't refer to a capture before it's encountered & completed
status = inSection(111);
pattern = /((\3|b)\2(a)x)+/;
string = 'aaaxabaxbaaxbbax';
actualmatch = string.match(pattern);
expectedmatch = Array('bbax', 'bbax', 'b', 'a');
addThis();

status = inSection(112);
pattern = /((\3|b)\2(a)){2,}/;
string = 'bbaababbabaaaaabbaaaabba';
actualmatch = string.match(pattern);
expectedmatch = Array('bbaaaabba', 'bba', 'b', 'a');
addThis();
*/

status = inSection(113);
pattern = /abc/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(114);
pattern = /abc/i;
string = 'XABCY';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(115);
pattern = /abc/i;
string = 'ABABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(116);
pattern = /ab*c/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(117);
pattern = /ab*bc/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(118);
pattern = /ab*bc/i;
string = 'ABBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBC');
addThis();

status = inSection(119);
pattern = /ab*?bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBBBC');
addThis();

status = inSection(120);
pattern = /ab{0,}?bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBBBC');
addThis();

status = inSection(121);
pattern = /ab+?bc/i;
string = 'ABBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBC');
addThis();

status = inSection(122);
pattern = /ab+bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBBBC');
addThis();

status = inSection(123);
pattern = /ab{1,}?bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBBBC');
addThis();

status = inSection(124);
pattern = /ab{1,3}?bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBBBC');
addThis();

status = inSection(125);
pattern = /ab{3,4}?bc/i;
string = 'ABBBBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBBBC');
addThis();

status = inSection(126);
pattern = /ab??bc/i;
string = 'ABBC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBC');
addThis();

status = inSection(127);
pattern = /ab??bc/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(128);
pattern = /ab{0,1}?bc/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(129);
pattern = /ab??c/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(130);
pattern = /ab{0,1}?c/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(131);
pattern = /^abc$/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(132);
pattern = /^abc/i;
string = 'ABCC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(133);
pattern = /abc$/i;
string = 'AABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(134);
pattern = /^/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(135);
pattern = /$/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(136);
pattern = /a.c/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(137);
pattern = /a.c/i;
string = 'AXC';
actualmatch = string.match(pattern);
expectedmatch = Array('AXC');
addThis();

status = inSection(138);
pattern = /a.*?c/i;
string = 'AXYZC';
actualmatch = string.match(pattern);
expectedmatch = Array('AXYZC');
addThis();

status = inSection(139);
pattern = /a[bc]d/i;
string = 'ABD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABD');
addThis();

status = inSection(140);
pattern = /a[b-d]e/i;
string = 'ACE';
actualmatch = string.match(pattern);
expectedmatch = Array('ACE');
addThis();

status = inSection(141);
pattern = /a[b-d]/i;
string = 'AAC';
actualmatch = string.match(pattern);
expectedmatch = Array('AC');
addThis();

status = inSection(142);
pattern = /a[-b]/i;
string = 'A-';
actualmatch = string.match(pattern);
expectedmatch = Array('A-');
addThis();

status = inSection(143);
pattern = /a[b-]/i;
string = 'A-';
actualmatch = string.match(pattern);
expectedmatch = Array('A-');
addThis();

status = inSection(144);
pattern = /a]/i;
string = 'A]';
actualmatch = string.match(pattern);
expectedmatch = Array('A]');
addThis();

/* Perl supports ] & ^] inside a [], ECMA does not
status = inSection(145);
pattern = /a[]]b/i;
string = 'A]B';
actualmatch = string.match(pattern);
expectedmatch = Array('A]B');
addThis();
*/

status = inSection(146);
pattern = /a[^bc]d/i;
string = 'AED';
actualmatch = string.match(pattern);
expectedmatch = Array('AED');
addThis();

status = inSection(147);
pattern = /a[^-b]c/i;
string = 'ADC';
actualmatch = string.match(pattern);
expectedmatch = Array('ADC');
addThis();

/* Perl supports ] & ^] inside a [], ECMA does not
status = inSection(148);
pattern = /a[^]b]c/i;
string = 'ADC';
actualmatch = string.match(pattern);
expectedmatch = Array('ADC');
addThis();
*/

status = inSection(149);
pattern = /ab|cd/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('AB');
addThis();

status = inSection(150);
pattern = /ab|cd/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('AB');
addThis();

status = inSection(151);
pattern = /()ef/i;
string = 'DEF';
actualmatch = string.match(pattern);
expectedmatch = Array('EF', '');
addThis();

status = inSection(152);
pattern = /a\(b/i;
string = 'A(B';
actualmatch = string.match(pattern);
expectedmatch = Array('A(B');
addThis();

status = inSection(153);
pattern = /a\(*b/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('AB');
addThis();

status = inSection(154);
pattern = /a\(*b/i;
string = 'A((B';
actualmatch = string.match(pattern);
expectedmatch = Array('A((B');
addThis();

status = inSection(155);
pattern = /a\\b/i;
string = 'A\\B';
actualmatch = string.match(pattern);
expectedmatch = Array('A\\B');
addThis();

status = inSection(156);
pattern = /((a))/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('A', 'A', 'A');
addThis();

status = inSection(157);
pattern = /(a)b(c)/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC', 'A', 'C');
addThis();

status = inSection(158);
pattern = /a+b+c/i;
string = 'AABBABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(159);
pattern = /a{1,}b{1,}c/i;
string = 'AABBABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(160);
pattern = /a.+?c/i;
string = 'ABCABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(161);
pattern = /a.*?c/i;
string = 'ABCABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(162);
pattern = /a.{0,5}?c/i;
string = 'ABCABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC');
addThis();

status = inSection(163);
pattern = /(a+|b)*/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('AB', 'B');
addThis();

status = inSection(164);
pattern = /(a+|b){0,}/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('AB', 'B');
addThis();

status = inSection(165);
pattern = /(a+|b)+/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('AB', 'B');
addThis();

status = inSection(166);
pattern = /(a+|b){1,}/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('AB', 'B');
addThis();

status = inSection(167);
pattern = /(a+|b)?/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('A', 'A');
addThis();

status = inSection(168);
pattern = /(a+|b){0,1}/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('A', 'A');
addThis();

status = inSection(169);
pattern = /(a+|b){0,1}?/i;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('', undefined);
addThis();

status = inSection(170);
pattern = /[^ab]*/i;
string = 'CDE';
actualmatch = string.match(pattern);
expectedmatch = Array('CDE');
addThis();

status = inSection(171);
pattern = /([abc])*d/i;
string = 'ABBBCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABBBCD', 'C');
addThis();

status = inSection(172);
pattern = /([abc])*bcd/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCD', 'A');
addThis();

status = inSection(173);
pattern = /a|b|c|d|e/i;
string = 'E';
actualmatch = string.match(pattern);
expectedmatch = Array('E');
addThis();

status = inSection(174);
pattern = /(a|b|c|d|e)f/i;
string = 'EF';
actualmatch = string.match(pattern);
expectedmatch = Array('EF', 'E');
addThis();

status = inSection(175);
pattern = /abcd*efg/i;
string = 'ABCDEFG';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCDEFG');
addThis();

status = inSection(176);
pattern = /ab*/i;
string = 'XABYABBBZ';
actualmatch = string.match(pattern);
expectedmatch = Array('AB');
addThis();

status = inSection(177);
pattern = /ab*/i;
string = 'XAYABBBZ';
actualmatch = string.match(pattern);
expectedmatch = Array('A');
addThis();

status = inSection(178);
pattern = /(ab|cd)e/i;
string = 'ABCDE';
actualmatch = string.match(pattern);
expectedmatch = Array('CDE', 'CD');
addThis();

status = inSection(179);
pattern = /[abhgefdc]ij/i;
string = 'HIJ';
actualmatch = string.match(pattern);
expectedmatch = Array('HIJ');
addThis();

status = inSection(180);
pattern = /(abc|)ef/i;
string = 'ABCDEF';
actualmatch = string.match(pattern);
expectedmatch = Array('EF', '');
addThis();

status = inSection(181);
pattern = /(a|b)c*d/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('BCD', 'B');
addThis();

status = inSection(182);
pattern = /(ab|ab*)bc/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC', 'A');
addThis();

status = inSection(183);
pattern = /a([bc]*)c*/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC', 'BC');
addThis();

status = inSection(184);
pattern = /a([bc]*)(c*d)/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCD', 'BC', 'D');
addThis();

status = inSection(185);
pattern = /a([bc]+)(c*d)/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCD', 'BC', 'D');
addThis();

status = inSection(186);
pattern = /a([bc]*)(c+d)/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCD', 'B', 'CD');
addThis();

status = inSection(187);
pattern = /a[bcd]*dcdcde/i;
string = 'ADCDCDE';
actualmatch = string.match(pattern);
expectedmatch = Array('ADCDCDE');
addThis();

status = inSection(188);
pattern = /(ab|a)b*c/i;
string = 'ABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABC', 'AB');
addThis();

status = inSection(189);
pattern = /((a)(b)c)(d)/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCD', 'ABC', 'A', 'B', 'D');
addThis();

status = inSection(190);
pattern = /[a-zA-Z_][a-zA-Z0-9_]*/i;
string = 'ALPHA';
actualmatch = string.match(pattern);
expectedmatch = Array('ALPHA');
addThis();

status = inSection(191);
pattern = /^a(bc+|b[eh])g|.h$/i;
string = 'ABH';
actualmatch = string.match(pattern);
expectedmatch = Array('BH', undefined);
addThis();

status = inSection(192);
pattern = /(bc+d$|ef*g.|h?i(j|k))/i;
string = 'EFFGZ';
actualmatch = string.match(pattern);
expectedmatch = Array('EFFGZ', 'EFFGZ', undefined);
addThis();

status = inSection(193);
pattern = /(bc+d$|ef*g.|h?i(j|k))/i;
string = 'IJ';
actualmatch = string.match(pattern);
expectedmatch = Array('IJ', 'IJ', 'J');
addThis();

status = inSection(194);
pattern = /(bc+d$|ef*g.|h?i(j|k))/i;
string = 'REFFGZ';
actualmatch = string.match(pattern);
expectedmatch = Array('EFFGZ', 'EFFGZ', undefined);
addThis();

status = inSection(195);
pattern = /((((((((((a))))))))))/i;
string = 'A';
actualmatch = string.match(pattern);
expectedmatch = Array('A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A');
addThis();

status = inSection(196);
pattern = /((((((((((a))))))))))\10/i;
string = 'AA';
actualmatch = string.match(pattern);
expectedmatch = Array('AA', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A');
addThis();

status = inSection(197);
pattern = /((((((((((a))))))))))/i;
string = 'A!';
actualmatch = string.match(pattern);
expectedmatch = Array('A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A');
addThis();

status = inSection(198);
pattern = /(((((((((a)))))))))/i;
string = 'A';
actualmatch = string.match(pattern);
expectedmatch = Array('A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A');
addThis();

status = inSection(199);
pattern = /(?:(?:(?:(?:(?:(?:(?:(?:(?:(a))))))))))/i;
string = 'A';
actualmatch = string.match(pattern);
expectedmatch = Array('A', 'A');
addThis();

status = inSection(200);
pattern = /(?:(?:(?:(?:(?:(?:(?:(?:(?:(a|b|c))))))))))/i;
string = 'C';
actualmatch = string.match(pattern);
expectedmatch = Array('C', 'C');
addThis();

status = inSection(201);
pattern = /(.*)c(.*)/i;
string = 'ABCDE';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCDE', 'AB', 'DE');
addThis();

status = inSection(202);
pattern = /abcd/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCD');
addThis();

status = inSection(203);
pattern = /a(bc)d/i;
string = 'ABCD';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCD', 'BC');
addThis();

status = inSection(204);
pattern = /a[-]?c/i;
string = 'AC';
actualmatch = string.match(pattern);
expectedmatch = Array('AC');
addThis();

status = inSection(205);
pattern = /(abc)\1/i;
string = 'ABCABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCABC', 'ABC');
addThis();

status = inSection(206);
pattern = /([a-c]*)\1/i;
string = 'ABCABC';
actualmatch = string.match(pattern);
expectedmatch = Array('ABCABC', 'ABC');
addThis();

status = inSection(207);
pattern = /a(?!b)./;
string = 'abad';
actualmatch = string.match(pattern);
expectedmatch = Array('ad');
addThis();

status = inSection(208);
pattern = /a(?=d)./;
string = 'abad';
actualmatch = string.match(pattern);
expectedmatch = Array('ad');
addThis();

status = inSection(209);
pattern = /a(?=c|d)./;
string = 'abad';
actualmatch = string.match(pattern);
expectedmatch = Array('ad');
addThis();

status = inSection(210);
pattern = /a(?:b|c|d)(.)/;
string = 'ace';
actualmatch = string.match(pattern);
expectedmatch = Array('ace', 'e');
addThis();

status = inSection(211);
pattern = /a(?:b|c|d)*(.)/;
string = 'ace';
actualmatch = string.match(pattern);
expectedmatch = Array('ace', 'e');
addThis();

status = inSection(212);
pattern = /a(?:b|c|d)+?(.)/;
string = 'ace';
actualmatch = string.match(pattern);
expectedmatch = Array('ace', 'e');
addThis();

status = inSection(213);
pattern = /a(?:b|c|d)+?(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acd', 'd');
addThis();

status = inSection(214);
pattern = /a(?:b|c|d)+(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdbe', 'e');
addThis();

status = inSection(215);
pattern = /a(?:b|c|d){2}(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdb', 'b');
addThis();

status = inSection(216);
pattern = /a(?:b|c|d){4,5}(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdb', 'b');
addThis();

status = inSection(217);
pattern = /a(?:b|c|d){4,5}?(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcd', 'd');
addThis();

// MODIFIED - ECMA has different rules for paren contents
status = inSection(218);
pattern = /((foo)|(bar))*/;
string = 'foobar';
actualmatch = string.match(pattern);
//expectedmatch = Array('foobar', 'bar', 'foo', 'bar');
expectedmatch = Array('foobar', 'bar', undefined, 'bar');
addThis();

status = inSection(219);
pattern = /a(?:b|c|d){6,7}(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdbe', 'e');
addThis();

status = inSection(220);
pattern = /a(?:b|c|d){6,7}?(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdbe', 'e');
addThis();

status = inSection(221);
pattern = /a(?:b|c|d){5,6}(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdbe', 'e');
addThis();

status = inSection(222);
pattern = /a(?:b|c|d){5,6}?(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdb', 'b');
addThis();

status = inSection(223);
pattern = /a(?:b|c|d){5,7}(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdbe', 'e');
addThis();

status = inSection(224);
pattern = /a(?:b|c|d){5,7}?(.)/;
string = 'acdbcdbe';
actualmatch = string.match(pattern);
expectedmatch = Array('acdbcdb', 'b');
addThis();

status = inSection(225);
pattern = /a(?:b|(c|e){1,2}?|d)+?(.)/;
string = 'ace';
actualmatch = string.match(pattern);
expectedmatch = Array('ace', 'c', 'e');
addThis();

status = inSection(226);
pattern = /^(.+)?B/;
string = 'AB';
actualmatch = string.match(pattern);
expectedmatch = Array('AB', 'A');
addThis();

/* MODIFIED - ECMA has different rules for paren contents */
status = inSection(227);
pattern = /^([^a-z])|(\^)$/;
string = '.';
actualmatch = string.match(pattern);
//expectedmatch = Array('.', '.', '');
expectedmatch = Array('.', '.', undefined);
addThis();

status = inSection(228);
pattern = /^[<>]&/;
string = '<&OUT';
actualmatch = string.match(pattern);
expectedmatch = Array('<&');
addThis();

/* Can't refer to a capture before it's encountered & completed
status = inSection(229);
pattern = /^(a\1?){4}$/;
string = 'aaaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaaaaaa', 'aaaa');
addThis();

status = inSection(230);
pattern = /^(a(?(1)\1)){4}$/;
string = 'aaaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaaaaaa', 'aaaa');
addThis();
*/

status = inSection(231);
pattern = /((a{4})+)/;
string = 'aaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaaaa', 'aaaaaaaa', 'aaaa');
addThis();

status = inSection(232);
pattern = /(((aa){2})+)/;
string = 'aaaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaaaa', 'aaaaaaaa', 'aaaa', 'aa');
addThis();

status = inSection(233);
pattern = /(((a{2}){2})+)/;
string = 'aaaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaaaa', 'aaaaaaaa', 'aaaa', 'aa');
addThis();

status = inSection(234);
pattern = /(?:(f)(o)(o)|(b)(a)(r))*/;
string = 'foobar';
actualmatch = string.match(pattern);
//expectedmatch = Array('foobar', 'f', 'o', 'o', 'b', 'a', 'r');
expectedmatch = Array('foobar', undefined, undefined, undefined, 'b', 'a', 'r');
addThis();

/* ECMA supports (?: (?= and (?! but doesn't support (?< etc.
status = inSection(235);
pattern = /(?<=a)b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('b');
addThis();

status = inSection(236);
pattern = /(?<!c)b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('b');
addThis();

status = inSection(237);
pattern = /(?<!c)b/;
string = 'b';
actualmatch = string.match(pattern);
expectedmatch = Array('b');
addThis();

status = inSection(238);
pattern = /(?<!c)b/;
string = 'b';
actualmatch = string.match(pattern);
expectedmatch = Array('b');
addThis();
*/

status = inSection(239);
pattern = /(?:..)*a/;
string = 'aba';
actualmatch = string.match(pattern);
expectedmatch = Array('aba');
addThis();

status = inSection(240);
pattern = /(?:..)*?a/;
string = 'aba';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

/*
 * MODIFIED - ECMA has different rules for paren contents. Note
 * this regexp has two non-capturing parens, and one capturing
 *
 * The issue: shouldn't the match be ['ab', undefined]? Because the
 * '\1' matches the undefined value of the second iteration of the '*'
 * (in which the 'b' part of the '|' matches). But Perl wants ['ab','b'].
 *
 * Answer: waldemar@netscape.com:
 *
 * The correct answer is ['ab', undefined].  Perl doesn't match
 * ECMAScript here, and I'd say that Perl is wrong in this case.
 */
status = inSection(241);
pattern = /^(?:b|a(?=(.)))*\1/;
string = 'abc';
actualmatch = string.match(pattern);
//expectedmatch = Array('ab', 'b');
expectedmatch = Array('ab', undefined);
addThis();

status = inSection(242);
pattern = /^(){3,5}/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('', '');
addThis();

status = inSection(243);
pattern = /^(a+)*ax/;
string = 'aax';
actualmatch = string.match(pattern);
expectedmatch = Array('aax', 'a');
addThis();

status = inSection(244);
pattern = /^((a|b)+)*ax/;
string = 'aax';
actualmatch = string.match(pattern);
expectedmatch = Array('aax', 'a', 'a');
addThis();

status = inSection(245);
pattern = /^((a|bc)+)*ax/;
string = 'aax';
actualmatch = string.match(pattern);
expectedmatch = Array('aax', 'a', 'a');
addThis();

/* MODIFIED - ECMA has different rules for paren contents */
status = inSection(246);
pattern = /(a|x)*ab/;
string = 'cab';
actualmatch = string.match(pattern);
//expectedmatch = Array('ab', '');
expectedmatch = Array('ab', undefined);
addThis();

status = inSection(247);
pattern = /(a)*ab/;
string = 'cab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', undefined);
addThis();

/* ECMA doesn't support (?imsx or (?-imsx
status = inSection(248);
pattern = /(?:(?i)a)b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(249);
pattern = /((?i)a)b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'a');
addThis();

status = inSection(250);
pattern = /(?:(?i)a)b/;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = Array('Ab');
addThis();

status = inSection(251);
pattern = /((?i)a)b/;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = Array('Ab', 'A');
addThis();

status = inSection(252);
pattern = /(?i:a)b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(253);
pattern = /((?i:a))b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'a');
addThis();

status = inSection(254);
pattern = /(?i:a)b/;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = Array('Ab');
addThis();

status = inSection(255);
pattern = /((?i:a))b/;
string = 'Ab';
actualmatch = string.match(pattern);
expectedmatch = Array('Ab', 'A');
addThis();

status = inSection(256);
pattern = /(?:(?-i)a)b/i;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(257);
pattern = /((?-i)a)b/i;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'a');
addThis();

status = inSection(258);
pattern = /(?:(?-i)a)b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB');
addThis();

status = inSection(259);
pattern = /((?-i)a)b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB', 'a');
addThis();

status = inSection(260);
pattern = /(?:(?-i)a)b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB');
addThis();

status = inSection(261);
pattern = /((?-i)a)b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB', 'a');
addThis();

status = inSection(262);
pattern = /(?-i:a)b/i;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(263);
pattern = /((?-i:a))b/i;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'a');
addThis();

status = inSection(264);
pattern = /(?-i:a)b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB');
addThis();

status = inSection(265);
pattern = /((?-i:a))b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB', 'a');
addThis();

status = inSection(266);
pattern = /(?-i:a)b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB');
addThis();

status = inSection(267);
pattern = /((?-i:a))b/i;
string = 'aB';
actualmatch = string.match(pattern);
expectedmatch = Array('aB', 'a');
addThis();

status = inSection(268);
pattern = /((?s-i:a.))b/i;
string = 'a\nB';
actualmatch = string.match(pattern);
expectedmatch = Array('a\nB', 'a\n');
addThis();
*/

status = inSection(269);
pattern = /(?:c|d)(?:)(?:a(?:)(?:b)(?:b(?:))(?:b(?:)(?:b)))/;
string = 'cabbbb';
actualmatch = string.match(pattern);
expectedmatch = Array('cabbbb');
addThis();

status = inSection(270);
pattern = /(?:c|d)(?:)(?:aaaaaaaa(?:)(?:bbbbbbbb)(?:bbbbbbbb(?:))(?:bbbbbbbb(?:)(?:bbbbbbbb)))/;
string = 'caaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
actualmatch = string.match(pattern);
expectedmatch = Array('caaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb');
addThis();

status = inSection(271);
pattern = /(ab)\d\1/i;
string = 'Ab4ab';
actualmatch = string.match(pattern);
expectedmatch = Array('Ab4ab', 'Ab');
addThis();

status = inSection(272);
pattern = /(ab)\d\1/i;
string = 'ab4Ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab4Ab', 'ab');
addThis();

status = inSection(273);
pattern = /foo\w*\d{4}baz/;
string = 'foobar1234baz';
actualmatch = string.match(pattern);
expectedmatch = Array('foobar1234baz');
addThis();

status = inSection(274);
pattern = /x(~~)*(?:(?:F)?)?/;
string = 'x~~';
actualmatch = string.match(pattern);
expectedmatch = Array('x~~', '~~');
addThis();

/* Perl supports (?# but JS doesn't
status = inSection(275);
pattern = /^a(?#xxx){3}c/;
string = 'aaac';
actualmatch = string.match(pattern);
expectedmatch = Array('aaac');
addThis();
*/

/* ECMA doesn't support (?< etc
status = inSection(276);
pattern = /(?<![cd])[ab]/;
string = 'dbaacb';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(277);
pattern = /(?<!(c|d))[ab]/;
string = 'dbaacb';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(278);
pattern = /(?<!cd)[ab]/;
string = 'cdaccb';
actualmatch = string.match(pattern);
expectedmatch = Array('b');
addThis();

status = inSection(279);
pattern = /((?s)^a(.))((?m)^b$)/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('a\nb', 'a\n', '\n', 'b');
addThis();

status = inSection(280);
pattern = /((?m)^b$)/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('b', 'b');
addThis();

status = inSection(281);
pattern = /(?m)^b/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('b');
addThis();

status = inSection(282);
pattern = /(?m)^(b)/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('b', 'b');
addThis();

status = inSection(283);
pattern = /((?m)^b)/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('b', 'b');
addThis();

status = inSection(284);
pattern = /\n((?m)^b)/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('\nb', 'b');
addThis();

status = inSection(285);
pattern = /((?s).)c(?!.)/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('\nc', '\n');
addThis();

status = inSection(286);
pattern = /((?s).)c(?!.)/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('\nc', '\n');
addThis();

status = inSection(287);
pattern = /((?s)b.)c(?!.)/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('b\nc', 'b\n');
addThis();

status = inSection(288);
pattern = /((?s)b.)c(?!.)/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('b\nc', 'b\n');
addThis();

status = inSection(289);
pattern = /((?m)^b)/;
string = 'a\nb\nc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('b', 'b');
addThis();
*/

/* ECMA doesn't support (?(condition)
status = inSection(290);
pattern = /(?(1)b|a)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(291);
pattern = /(x)?(?(1)b|a)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(292);
pattern = /()?(?(1)b|a)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(293);
pattern = /()?(?(1)a|b)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(294);
pattern = /^(\()?blah(?(1)(\)))$/;
string = '(blah)';
actualmatch = string.match(pattern);
expectedmatch = Array('(blah)', '(', ')');
addThis();

status = inSection(295);
pattern = /^(\()?blah(?(1)(\)))$/;
string = 'blah';
actualmatch = string.match(pattern);
expectedmatch = Array('blah');
addThis();

status = inSection(296);
pattern = /^(\(+)?blah(?(1)(\)))$/;
string = '(blah)';
actualmatch = string.match(pattern);
expectedmatch = Array('(blah)', '(', ')');
addThis();

status = inSection(297);
pattern = /^(\(+)?blah(?(1)(\)))$/;
string = 'blah';
actualmatch = string.match(pattern);
expectedmatch = Array('blah');
addThis();

status = inSection(298);
pattern = /(?(?!a)b|a)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(299);
pattern = /(?(?=a)a|b)/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();
*/

status = inSection(300);
pattern = /(?=(a+?))(\1ab)/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = Array('aab', 'a', 'aab');
addThis();

status = inSection(301);
pattern = /(\w+:)+/;
string = 'one:';
actualmatch = string.match(pattern);
expectedmatch = Array('one:', 'one:');
addThis();

/* ECMA doesn't support (?< etc
status = inSection(302);
pattern = /$(?<=^(a))/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('', 'a');
addThis();
*/

status = inSection(303);
pattern = /(?=(a+?))(\1ab)/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = Array('aab', 'a', 'aab');
addThis();

/* MODIFIED - ECMA has different rules for paren contents */
status = inSection(304);
pattern = /([\w:]+::)?(\w+)$/;
string = 'abcd';
actualmatch = string.match(pattern);
//expectedmatch = Array('abcd', '', 'abcd');
expectedmatch = Array('abcd', undefined, 'abcd');
addThis();

status = inSection(305);
pattern = /([\w:]+::)?(\w+)$/;
string = 'xy:z:::abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('xy:z:::abcd', 'xy:z:::', 'abcd');
addThis();

status = inSection(306);
pattern = /^[^bcd]*(c+)/;
string = 'aexycd';
actualmatch = string.match(pattern);
expectedmatch = Array('aexyc', 'c');
addThis();

status = inSection(307);
pattern = /(a*)b+/;
string = 'caab';
actualmatch = string.match(pattern);
expectedmatch = Array('aab', 'aa');
addThis();

/* MODIFIED - ECMA has different rules for paren contents */
status = inSection(308);
pattern = /([\w:]+::)?(\w+)$/;
string = 'abcd';
actualmatch = string.match(pattern);
//expectedmatch = Array('abcd', '', 'abcd');
expectedmatch = Array('abcd', undefined, 'abcd');
addThis();

status = inSection(309);
pattern = /([\w:]+::)?(\w+)$/;
string = 'xy:z:::abcd';
actualmatch = string.match(pattern);
expectedmatch = Array('xy:z:::abcd', 'xy:z:::', 'abcd');
addThis();

status = inSection(310);
pattern = /^[^bcd]*(c+)/;
string = 'aexycd';
actualmatch = string.match(pattern);
expectedmatch = Array('aexyc', 'c');
addThis();

/* ECMA doesn't support (?>
status = inSection(311);
pattern = /(?>a+)b/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = Array('aaab');
addThis();
*/

status = inSection(312);
pattern = /([[:]+)/;
string = 'a:[b]:';
actualmatch = string.match(pattern);
expectedmatch = Array(':[', ':[');
addThis();

status = inSection(313);
pattern = /([[=]+)/;
string = 'a=[b]=';
actualmatch = string.match(pattern);
expectedmatch = Array('=[', '=[');
addThis();

status = inSection(314);
pattern = /([[.]+)/;
string = 'a.[b].';
actualmatch = string.match(pattern);
expectedmatch = Array('.[', '.[');
addThis();

/* ECMA doesn't have rules for [:
status = inSection(315);
pattern = /[a[:]b[:c]/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();
*/

/* ECMA doesn't support (?>
status = inSection(316);
pattern = /((?>a+)b)/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = Array('aaab', 'aaab');
addThis();

status = inSection(317);
pattern = /(?>(a+))b/;
string = 'aaab';
actualmatch = string.match(pattern);
expectedmatch = Array('aaab', 'aaa');
addThis();

status = inSection(318);
pattern = /((?>[^()]+)|\([^()]*\))+/;
string = '((abc(ade)ufh()()x';
actualmatch = string.match(pattern);
expectedmatch = Array('abc(ade)ufh()()x', 'x');
addThis();
*/

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(319);
pattern = /\Z/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(320);
pattern = /\z/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();
*/

status = inSection(321);
pattern = /$/;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(322);
pattern = /\Z/;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(323);
pattern = /\z/;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();
*/

status = inSection(324);
pattern = /$/;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(325);
pattern = /\Z/;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(326);
pattern = /\z/;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();
*/

status = inSection(327);
pattern = /$/;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(328);
pattern = /\Z/m;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(329);
pattern = /\z/m;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();
*/

status = inSection(330);
pattern = /$/m;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(331);
pattern = /\Z/m;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(332);
pattern = /\z/m;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();
*/

status = inSection(333);
pattern = /$/m;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(334);
pattern = /\Z/m;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

status = inSection(335);
pattern = /\z/m;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();
*/

status = inSection(336);
pattern = /$/m;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(337);
pattern = /a\Z/;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();
*/

/* $ only matches end of input unless multiline
status = inSection(338);
pattern = /a$/;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();
*/

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(339);
pattern = /a\Z/;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(340);
pattern = /a\z/;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();
*/

status = inSection(341);
pattern = /a$/;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(342);
pattern = /a$/m;
string = 'a\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(343);
pattern = /a\Z/m;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();
*/

status = inSection(344);
pattern = /a$/m;
string = 'b\na\n';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(345);
pattern = /a\Z/m;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(346);
pattern = /a\z/m;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();
*/

status = inSection(347);
pattern = /a$/m;
string = 'b\na';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(348);
pattern = /aa\Z/;
string = 'b\naa\n';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();
*/

/* $ only matches end of input unless multiline
status = inSection(349);
pattern = /aa$/;
string = 'b\naa\n';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();
*/

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(350);
pattern = /aa\Z/;
string = 'b\naa';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();

status = inSection(351);
pattern = /aa\z/;
string = 'b\naa';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();
*/

status = inSection(352);
pattern = /aa$/;
string = 'b\naa';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();

status = inSection(353);
pattern = /aa$/m;
string = 'aa\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(354);
pattern = /aa\Z/m;
string = 'b\naa\n';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();
*/

status = inSection(355);
pattern = /aa$/m;
string = 'b\naa\n';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(356);
pattern = /aa\Z/m;
string = 'b\naa';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();

status = inSection(357);
pattern = /aa\z/m;
string = 'b\naa';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();
*/

status = inSection(358);
pattern = /aa$/m;
string = 'b\naa';
actualmatch = string.match(pattern);
expectedmatch = Array('aa');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(359);
pattern = /ab\Z/;
string = 'b\nab\n';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();
*/

/* $ only matches end of input unless multiline
status = inSection(360);
pattern = /ab$/;
string = 'b\nab\n';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();
*/

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(361);
pattern = /ab\Z/;
string = 'b\nab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(362);
pattern = /ab\z/;
string = 'b\nab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();
*/

status = inSection(363);
pattern = /ab$/;
string = 'b\nab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(364);
pattern = /ab$/m;
string = 'ab\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(365);
pattern = /ab\Z/m;
string = 'b\nab\n';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();
*/

status = inSection(366);
pattern = /ab$/m;
string = 'b\nab\n';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(367);
pattern = /ab\Z/m;
string = 'b\nab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

status = inSection(368);
pattern = /ab\z/m;
string = 'b\nab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();
*/

status = inSection(369);
pattern = /ab$/m;
string = 'b\nab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(370);
pattern = /abb\Z/;
string = 'b\nabb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();
*/

/* $ only matches end of input unless multiline
status = inSection(371);
pattern = /abb$/;
string = 'b\nabb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();
*/

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(372);
pattern = /abb\Z/;
string = 'b\nabb';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();

status = inSection(373);
pattern = /abb\z/;
string = 'b\nabb';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();
*/

status = inSection(374);
pattern = /abb$/;
string = 'b\nabb';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();

status = inSection(375);
pattern = /abb$/m;
string = 'abb\nb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(376);
pattern = /abb\Z/m;
string = 'b\nabb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();
*/

status = inSection(377);
pattern = /abb$/m;
string = 'b\nabb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();

/* Perl has \Z has end-of-line, ECMA doesn't
status = inSection(378);
pattern = /abb\Z/m;
string = 'b\nabb';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();

status = inSection(379);
pattern = /abb\z/m;
string = 'b\nabb';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();
*/

status = inSection(380);
pattern = /abb$/m;
string = 'b\nabb';
actualmatch = string.match(pattern);
expectedmatch = Array('abb');
addThis();

status = inSection(381);
pattern = /(^|x)(c)/;
string = 'ca';
actualmatch = string.match(pattern);
expectedmatch = Array('c', '', 'c');
addThis();

status = inSection(382);
pattern = /foo.bart/;
string = 'foo.bart';
actualmatch = string.match(pattern);
expectedmatch = Array('foo.bart');
addThis();

status = inSection(383);
pattern = /^d[x][x][x]/m;
string = 'abcd\ndxxx';
actualmatch = string.match(pattern);
expectedmatch = Array('dxxx');
addThis();

status = inSection(384);
pattern = /tt+$/;
string = 'xxxtt';
actualmatch = string.match(pattern);
expectedmatch = Array('tt');
addThis();

/* ECMA spec says that each atom in a range must be a single character
status = inSection(385);
pattern = /([a-\d]+)/;
string = 'za-9z';
actualmatch = string.match(pattern);
expectedmatch = Array('9', '9');
addThis();

status = inSection(386);
pattern = /([\d-z]+)/;
string = 'a0-za';
actualmatch = string.match(pattern);
expectedmatch = Array('0-z', '0-z');
addThis();
*/

/* ECMA doesn't support [:
status = inSection(387);
pattern = /([a-[:digit:]]+)/;
string = 'za-9z';
actualmatch = string.match(pattern);
expectedmatch = Array('a-9', 'a-9');
addThis();

status = inSection(388);
pattern = /([[:digit:]-z]+)/;
string = '=0-z=';
actualmatch = string.match(pattern);
expectedmatch = Array('0-z', '0-z');
addThis();

status = inSection(389);
pattern = /([[:digit:]-[:alpha:]]+)/;
string = '=0-z=';
actualmatch = string.match(pattern);
expectedmatch = Array('0-z', '0-z');
addThis();
*/

status = inSection(390);
pattern = /(\d+\.\d+)/;
string = '3.1415926';
actualmatch = string.match(pattern);
expectedmatch = Array('3.1415926', '3.1415926');
addThis();

status = inSection(391);
pattern = /\.c(pp|xx|c)?$/i;
string = 'IO.c';
actualmatch = string.match(pattern);
expectedmatch = Array('.c', undefined);
addThis();

status = inSection(392);
pattern = /(\.c(pp|xx|c)?$)/i;
string = 'IO.c';
actualmatch = string.match(pattern);
expectedmatch = Array('.c', '.c', undefined);
addThis();

status = inSection(393);
pattern = /(^|a)b/;
string = 'ab';
actualmatch = string.match(pattern);
expectedmatch = Array('ab', 'a');
addThis();

status = inSection(394);
pattern = /^([ab]*?)(b)?(c)$/;
string = 'abac';
actualmatch = string.match(pattern);
expectedmatch = Array('abac', 'aba', undefined, 'c');
addThis();

status = inSection(395);
pattern = /^(?:.,){2}c/i;
string = 'a,b,c';
actualmatch = string.match(pattern);
expectedmatch = Array('a,b,c');
addThis();

status = inSection(396);
pattern = /^(.,){2}c/i;
string = 'a,b,c';
actualmatch = string.match(pattern);
expectedmatch =  Array('a,b,c', 'b,');
addThis();

status = inSection(397);
pattern = /^(?:[^,]*,){2}c/;
string = 'a,b,c';
actualmatch = string.match(pattern);
expectedmatch = Array('a,b,c');
addThis();

status = inSection(398);
pattern = /^([^,]*,){2}c/;
string = 'a,b,c';
actualmatch = string.match(pattern);
expectedmatch = Array('a,b,c', 'b,');
addThis();

status = inSection(399);
pattern = /^([^,]*,){3}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(400);
pattern = /^([^,]*,){3,}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(401);
pattern = /^([^,]*,){0,3}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(402);
pattern = /^([^,]{1,3},){3}d/i;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(403);
pattern = /^([^,]{1,3},){3,}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(404);
pattern = /^([^,]{1,3},){0,3}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(405);
pattern = /^([^,]{1,},){3}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(406);
pattern = /^([^,]{1,},){3,}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(407);
pattern = /^([^,]{1,},){0,3}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(408);
pattern = /^([^,]{0,3},){3}d/i;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(409);
pattern = /^([^,]{0,3},){3,}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

status = inSection(410);
pattern = /^([^,]{0,3},){0,3}d/;
string = 'aaa,b,c,d';
actualmatch = string.match(pattern);
expectedmatch = Array('aaa,b,c,d', 'c,');
addThis();

/* ECMA doesn't support \A
status = inSection(411);
pattern = /(?!\A)x/m;
string = 'a\nxb\n';
actualmatch = string.match(pattern);
expectedmatch = Array('\n');
addThis();
*/

status = inSection(412);
pattern = /^(a(b)?)+$/;
string = 'aba';
actualmatch = string.match(pattern);
expectedmatch = Array('aba', 'a', undefined);
addThis();

status = inSection(413);
pattern = /^(aa(bb)?)+$/;
string = 'aabbaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aabbaa', 'aa', undefined);
addThis();

status = inSection(414);
pattern = /^.{9}abc.*\n/m;
string = '123\nabcabcabcabc\n';
actualmatch = string.match(pattern);
expectedmatch = Array('abcabcabcabc\n');
addThis();

status = inSection(415);
pattern = /^(a)?a$/;
string = 'a';
actualmatch = string.match(pattern);
expectedmatch = Array('a', undefined);
addThis();

status = inSection(416);
pattern = /^(a\1?)(a\1?)(a\2?)(a\3?)$/;
string = 'aaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaa', 'a', 'aa', 'a', 'aa');
addThis();

/* Can't refer to a capture before it's encountered & completed
status = inSection(417);
pattern = /^(a\1?){4}$/;
string = 'aaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaa', 'aaa');
addThis();
*/

status = inSection(418);
pattern = /^(0+)?(?:x(1))?/;
string = 'x1';
actualmatch = string.match(pattern);
expectedmatch = Array('x1', undefined, '1');
addThis();

status = inSection(419);
pattern = /^([0-9a-fA-F]+)(?:x([0-9a-fA-F]+)?)(?:x([0-9a-fA-F]+))?/;
string = '012cxx0190';
actualmatch = string.match(pattern);
expectedmatch = Array('012cxx0190', '012c', undefined, '0190');
addThis();

status = inSection(420);
pattern = /^(b+?|a){1,2}c/;
string = 'bbbac';
actualmatch = string.match(pattern);
expectedmatch = Array('bbbac', 'a');
addThis();

status = inSection(421);
pattern = /^(b+?|a){1,2}c/;
string = 'bbbbac';
actualmatch = string.match(pattern);
expectedmatch = Array('bbbbac', 'a');
addThis();

status = inSection(422);
pattern = /((?:aaaa|bbbb)cccc)?/;
string = 'aaaacccc';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaacccc', 'aaaacccc');
addThis();

status = inSection(423);
pattern = /((?:aaaa|bbbb)cccc)?/;
string = 'bbbbcccc';
actualmatch = string.match(pattern);
expectedmatch = Array('bbbbcccc', 'bbbbcccc');
addThis();




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
