#ifndef CSSGRAMMAR_H
#define CSSGRAMMAR_H

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOKEN_EOF = 0,
     LOWEST_PREC = 258,
     UNIMPORTANT_TOK = 259,
     WHITESPACE = 260,
     SGML_CD = 261,
     INCLUDES = 262,
     DASHMATCH = 263,
     BEGINSWITH = 264,
     ENDSWITH = 265,
     CONTAINS = 266,
     STRING = 267,
     IDENT = 268,
     NTH = 269,
     HEX = 270,
     IDSEL = 271,
     IMPORT_SYM = 272,
     PAGE_SYM = 273,
     MEDIA_SYM = 274,
     FONT_FACE_SYM = 275,
     CHARSET_SYM = 276,
     NAMESPACE_SYM = 277,
     WEBKIT_RULE_SYM = 278,
     WEBKIT_DECLS_SYM = 279,
     WEBKIT_KEYFRAME_RULE_SYM = 280,
     WEBKIT_KEYFRAMES_SYM = 281,
     WEBKIT_VALUE_SYM = 282,
     WEBKIT_MEDIAQUERY_SYM = 283,
     WEBKIT_SELECTOR_SYM = 284,
     TOPLEFTCORNER_SYM = 285,
     TOPLEFT_SYM = 286,
     TOPCENTER_SYM = 287,
     TOPRIGHT_SYM = 288,
     TOPRIGHTCORNER_SYM = 289,
     BOTTOMLEFTCORNER_SYM = 290,
     BOTTOMLEFT_SYM = 291,
     BOTTOMCENTER_SYM = 292,
     BOTTOMRIGHT_SYM = 293,
     BOTTOMRIGHTCORNER_SYM = 294,
     LEFTTOP_SYM = 295,
     LEFTMIDDLE_SYM = 296,
     LEFTBOTTOM_SYM = 297,
     RIGHTTOP_SYM = 298,
     RIGHTMIDDLE_SYM = 299,
     RIGHTBOTTOM_SYM = 300,
     ATKEYWORD = 301,
     IMPORTANT_SYM = 302,
     MEDIA_ONLY = 303,
     MEDIA_NOT = 304,
     MEDIA_AND = 305,
     REMS = 306,
     QEMS = 307,
     EMS = 308,
     EXS = 309,
     PXS = 310,
     CMS = 311,
     MMS = 312,
     INS = 313,
     PTS = 314,
     PCS = 315,
     DEGS = 316,
     RADS = 317,
     GRADS = 318,
     TURNS = 319,
     MSECS = 320,
     SECS = 321,
     HERTZ = 322,
     KHERTZ = 323,
     DIMEN = 324,
     INVALIDDIMEN = 325,
     PERCENTAGE = 326,
     FLOATTOKEN = 327,
     INTEGER = 328,
     URI = 329,
     FUNCTION = 330,
     ANYFUNCTION = 331,
     NOTFUNCTION = 332,
     CALCFUNCTION = 333,
     MINFUNCTION = 334,
     MAXFUNCTION = 335,
     UNICODERANGE = 336
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 63 "../Source/WebCore/css/CSSGrammar.y"

    bool boolean;
    char character;
    int integer;
    double number;
    CSSParserString string;

    CSSRule* rule;
    CSSRuleList* ruleList;
    CSSParserSelector* selector;
    Vector<OwnPtr<CSSParserSelector> >* selectorList;
    CSSSelector::MarginBoxType marginBox;
    CSSSelector::Relation relation;
    MediaList* mediaList;
    MediaQuery* mediaQuery;
    MediaQuery::Restrictor mediaQueryRestrictor;
    MediaQueryExp* mediaQueryExp;
    CSSParserValue value;
    CSSParserValueList* valueList;
    Vector<OwnPtr<MediaQueryExp> >* mediaQueryExpList;
    WebKitCSSKeyframeRule* keyframeRule;
    WebKitCSSKeyframesRule* keyframesRule;
    float val;



/* Line 1676 of yacc.c  */
#line 161 "/Source/WebCore/generated/CSSGrammar.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




#endif
