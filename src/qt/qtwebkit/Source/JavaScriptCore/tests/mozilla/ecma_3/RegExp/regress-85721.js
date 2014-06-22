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
* Date:    14 Feb 2002
* SUMMARY: Performance: Regexp performance degraded from 4.7
* See http://bugzilla.mozilla.org/show_bug.cgi?id=85721
*
* Adjust this testcase if necessary. The FAST constant defines
* an upper bound in milliseconds for any execution to take.
*
*/
//-----------------------------------------------------------------------------
var bug = 85721;
var summary = 'Performance: execution of regular expression';
var FAST = 100; // execution should be 100 ms or less to pass the test
var MSG_FAST = 'Execution took less than ' + FAST + ' ms';
var MSG_SLOW = 'Execution took ';
var MSG_MS = ' ms';
var str = '';
var re = '';
var status = '';
var actual = '';
var expect= '';

printBugNumber (bug);
printStatus (summary);


function elapsedTime(startTime)
{
  return new Date() - startTime;
}


function isThisFast(ms)
{
  if (ms <= FAST)
    return MSG_FAST;
  return MSG_SLOW + ms + MSG_MS;
}



/*
 * The first regexp. We'll test for performance (Section 1) and accuracy (Section 2).
 */ 
str='<sql:connection id="conn1"> <sql:url>www.m.com</sql:url> <sql:driver>drive.class</sql:driver>\n<sql:userId>foo</sql:userId> <sql:password>goo</sql:password> </sql:connection>';
re = /<sql:connection id="([^\r\n]*?)">\s*<sql:url>\s*([^\r\n]*?)\s*<\/sql:url>\s*<sql:driver>\s*([^\r\n]*?)\s*<\/sql:driver>\s*(\s*<sql:userId>\s*([^\r\n]*?)\s*<\/sql:userId>\s*)?\s*(\s*<sql:password>\s*([^\r\n]*?)\s*<\/sql:password>\s*)?\s*<\/sql:connection>/;
expect = Array("<sql:connection id=\"conn1\"> <sql:url>www.m.com</sql:url> <sql:driver>drive.class</sql:driver>\n<sql:userId>foo</sql:userId> <sql:password>goo</sql:password> </sql:connection>","conn1","www.m.com","drive.class","<sql:userId>foo</sql:userId> ","foo","<sql:password>goo</sql:password> ","goo");

/*
 *  Check performance -
 */
status = inSection(1);
var start = new Date();
var result = re.exec(str);
actual = elapsedTime(start);
reportCompare(isThisFast(FAST), isThisFast(actual), status);

/*
 *  Check accuracy -
 */
status = inSection(2);
testRegExp([status], [re], [str], [result], [expect]);



/*
 * The second regexp (HUGE!). We'll test for performance (Section 3) and accuracy (Section 4).
 * It comes from the O'Reilly book "Mastering Regular Expressions" by Jeffrey Friedl, Appendix B
 */ 

//# Some things for avoiding backslashitis later on.
$esc        = '\\\\';      
$Period      = '\.';
$space      = '\040';              $tab         = '\t';
$OpenBR     = '\\[';               $CloseBR     = '\\]';
$OpenParen  = '\\(';               $CloseParen  = '\\)';
$NonASCII   = '\x80-\xff';         $ctrl        = '\000-\037';
$CRlist     = '\n\015';  //# note: this should really be only \015.
// Items 19, 20, 21
$qtext = '[^' + $esc + $NonASCII + $CRlist + '\"]';						  // # for within "..."
$dtext = '[^' + $esc + $NonASCII + $CRlist + $OpenBR + $CloseBR + ']';    // # for within [...]
$quoted_pair = $esc + '[^' + $NonASCII + ']';							  // # an escaped character

//##############################################################################
//# Items 22 and 23, comment.
//# Impossible to do properly with a regex, I make do by allowing at most one level of nesting.
$ctext   =  '[^' + $esc + $NonASCII + $CRlist + '()]';

//# $Cnested matches one non-nested comment.
//# It is unrolled, with normal of $ctext, special of $quoted_pair.
$Cnested = 
   $OpenParen +                                 // #  (
      $ctext + '*' +                            // #     normal*
      '(?:' + $quoted_pair + $ctext + '*)*' +   // #     (special normal*)*
   $CloseParen;                                 // #                       )


//# $comment allows one level of nested parentheses
//# It is unrolled, with normal of $ctext, special of ($quoted_pair|$Cnested)
$comment = 
   $OpenParen +                                           // #  (
       $ctext + '*' +                                     // #     normal*
       '(?:' +                                            // #       (
          '(?:' + $quoted_pair + '|' + $Cnested + ')' +   // #         special
           $ctext + '*' +                                 // #         normal*
       ')*' +                                             // #            )*
   $CloseParen;                                           // #                )


//##############################################################################
//# $X is optional whitespace/comments.
$X = 
   '[' + $space + $tab + ']*' +					       // # Nab whitespace.
   '(?:' + $comment + '[' + $space + $tab + ']*)*';    // # If comment found, allow more spaces.


//# Item 10: atom
$atom_char   = '[^(' + $space + '<>\@,;:\".' + $esc + $OpenBR + $CloseBR + $ctrl + $NonASCII + ']';
$atom = 
  $atom_char + '+' +            // # some number of atom characters...
  '(?!' + $atom_char + ')';     // # ..not followed by something that could be part of an atom

// # Item 11: doublequoted string, unrolled.
$quoted_str =
    '\"' +                                         // # "
       $qtext + '*' +                              // #   normal
       '(?:' + $quoted_pair + $qtext + '*)*' +     // #   ( special normal* )*
    '\"';                                          // # "

//# Item 7: word is an atom or quoted string
$word = 
    '(?:' +
       $atom +                // # Atom
       '|' +                  //     #  or
       $quoted_str +          // # Quoted string
     ')'

//# Item 12: domain-ref is just an atom
$domain_ref  = $atom;

//# Item 13: domain-literal is like a quoted string, but [...] instead of  "..."
$domain_lit  = 
    $OpenBR +								   	     // # [
    '(?:' + $dtext + '|' + $quoted_pair + ')*' +     // #    stuff
    $CloseBR;                                        // #           ]

// # Item 9: sub-domain is a domain-ref or domain-literal
$sub_domain  =
  '(?:' + 
    $domain_ref + 
    '|' +
    $domain_lit +
   ')' +
   $X;                 // # optional trailing comments

// # Item 6: domain is a list of subdomains separated by dots.
$domain = 
     $sub_domain +
     '(?:' +
        $Period + $X + $sub_domain +
     ')*';

//# Item 8: a route. A bunch of "@ $domain" separated by commas, followed by a colon.
$route = 
    '\@' + $X + $domain +
    '(?:,' + $X + '\@' + $X + $domain + ')*' +  // # additional domains
    ':' +
    $X;					// # optional trailing comments

//# Item 6: local-part is a bunch of $word separated by periods
$local_part = 
    $word + $X
    '(?:' + 
        $Period + $X + $word + $X +		// # additional words
    ')*';

// # Item 2: addr-spec is local@domain
$addr_spec  = 
  $local_part + '\@' + $X + $domain;

//# Item 4: route-addr is <route? addr-spec>
$route_addr =
    '<' + $X +                     // # <
       '(?:' + $route + ')?' +     // #       optional route
       $addr_spec +                // #       address spec
    '>';                           // #                 >

//# Item 3: phrase........
$phrase_ctrl = '\000-\010\012-\037'; // # like ctrl, but without tab

//# Like atom-char, but without listing space, and uses phrase_ctrl.
//# Since the class is negated, this matches the same as atom-char plus space and tab
$phrase_char =
   '[^()<>\@,;:\".' + $esc + $OpenBR + $CloseBR + $NonASCII + $phrase_ctrl + ']';

// # We've worked it so that $word, $comment, and $quoted_str to not consume trailing $X
// # because we take care of it manually.
$phrase = 
   $word +                                                  // # leading word
   $phrase_char + '*' +                                     // # "normal" atoms and/or spaces
   '(?:' +
      '(?:' + $comment + '|' + $quoted_str + ')' +          // # "special" comment or quoted string
      $phrase_char + '*' +                                  // #  more "normal"
   ')*';

// ## Item #1: mailbox is an addr_spec or a phrase/route_addr
$mailbox = 
    $X +                                // # optional leading comment
    '(?:' +
            $phrase + $route_addr +     // # name and address
            '|' +                       //     #  or
            $addr_spec +                // # address
     ')';


//###########################################################################


re = new RegExp($mailbox, "g");
str = 'Jeffy<"That Tall Guy"@ora.com (this address is no longer active)>';
expect = Array('Jeffy<"That Tall Guy"@ora.com (this address is no longer active)>');

/*
 *  Check performance -
 */
status = inSection(3);
var start = new Date();
var result = re.exec(str);
actual = elapsedTime(start);
reportCompare(isThisFast(FAST), isThisFast(actual), status);

/*
 *  Check accuracy -
 */
status = inSection(4);
testRegExp([status], [re], [str], [result], [expect]);
