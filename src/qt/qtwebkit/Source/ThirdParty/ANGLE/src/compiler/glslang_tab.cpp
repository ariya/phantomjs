/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INVARIANT = 258,
     HIGH_PRECISION = 259,
     MEDIUM_PRECISION = 260,
     LOW_PRECISION = 261,
     PRECISION = 262,
     ATTRIBUTE = 263,
     CONST_QUAL = 264,
     BOOL_TYPE = 265,
     FLOAT_TYPE = 266,
     INT_TYPE = 267,
     BREAK = 268,
     CONTINUE = 269,
     DO = 270,
     ELSE = 271,
     FOR = 272,
     IF = 273,
     DISCARD = 274,
     RETURN = 275,
     BVEC2 = 276,
     BVEC3 = 277,
     BVEC4 = 278,
     IVEC2 = 279,
     IVEC3 = 280,
     IVEC4 = 281,
     VEC2 = 282,
     VEC3 = 283,
     VEC4 = 284,
     MATRIX2 = 285,
     MATRIX3 = 286,
     MATRIX4 = 287,
     IN_QUAL = 288,
     OUT_QUAL = 289,
     INOUT_QUAL = 290,
     UNIFORM = 291,
     VARYING = 292,
     STRUCT = 293,
     VOID_TYPE = 294,
     WHILE = 295,
     SAMPLER2D = 296,
     SAMPLERCUBE = 297,
     SAMPLER_EXTERNAL_OES = 298,
     SAMPLER2DRECT = 299,
     IDENTIFIER = 300,
     TYPE_NAME = 301,
     FLOATCONSTANT = 302,
     INTCONSTANT = 303,
     BOOLCONSTANT = 304,
     LEFT_OP = 305,
     RIGHT_OP = 306,
     INC_OP = 307,
     DEC_OP = 308,
     LE_OP = 309,
     GE_OP = 310,
     EQ_OP = 311,
     NE_OP = 312,
     AND_OP = 313,
     OR_OP = 314,
     XOR_OP = 315,
     MUL_ASSIGN = 316,
     DIV_ASSIGN = 317,
     ADD_ASSIGN = 318,
     MOD_ASSIGN = 319,
     LEFT_ASSIGN = 320,
     RIGHT_ASSIGN = 321,
     AND_ASSIGN = 322,
     XOR_ASSIGN = 323,
     OR_ASSIGN = 324,
     SUB_ASSIGN = 325,
     LEFT_PAREN = 326,
     RIGHT_PAREN = 327,
     LEFT_BRACKET = 328,
     RIGHT_BRACKET = 329,
     LEFT_BRACE = 330,
     RIGHT_BRACE = 331,
     DOT = 332,
     COMMA = 333,
     COLON = 334,
     EQUAL = 335,
     SEMICOLON = 336,
     BANG = 337,
     DASH = 338,
     TILDE = 339,
     PLUS = 340,
     STAR = 341,
     SLASH = 342,
     PERCENT = 343,
     LEFT_ANGLE = 344,
     RIGHT_ANGLE = 345,
     VERTICAL_BAR = 346,
     CARET = 347,
     AMPERSAND = 348,
     QUESTION = 349
   };
#endif
/* Tokens.  */
#define INVARIANT 258
#define HIGH_PRECISION 259
#define MEDIUM_PRECISION 260
#define LOW_PRECISION 261
#define PRECISION 262
#define ATTRIBUTE 263
#define CONST_QUAL 264
#define BOOL_TYPE 265
#define FLOAT_TYPE 266
#define INT_TYPE 267
#define BREAK 268
#define CONTINUE 269
#define DO 270
#define ELSE 271
#define FOR 272
#define IF 273
#define DISCARD 274
#define RETURN 275
#define BVEC2 276
#define BVEC3 277
#define BVEC4 278
#define IVEC2 279
#define IVEC3 280
#define IVEC4 281
#define VEC2 282
#define VEC3 283
#define VEC4 284
#define MATRIX2 285
#define MATRIX3 286
#define MATRIX4 287
#define IN_QUAL 288
#define OUT_QUAL 289
#define INOUT_QUAL 290
#define UNIFORM 291
#define VARYING 292
#define STRUCT 293
#define VOID_TYPE 294
#define WHILE 295
#define SAMPLER2D 296
#define SAMPLERCUBE 297
#define SAMPLER_EXTERNAL_OES 298
#define SAMPLER2DRECT 299
#define IDENTIFIER 300
#define TYPE_NAME 301
#define FLOATCONSTANT 302
#define INTCONSTANT 303
#define BOOLCONSTANT 304
#define LEFT_OP 305
#define RIGHT_OP 306
#define INC_OP 307
#define DEC_OP 308
#define LE_OP 309
#define GE_OP 310
#define EQ_OP 311
#define NE_OP 312
#define AND_OP 313
#define OR_OP 314
#define XOR_OP 315
#define MUL_ASSIGN 316
#define DIV_ASSIGN 317
#define ADD_ASSIGN 318
#define MOD_ASSIGN 319
#define LEFT_ASSIGN 320
#define RIGHT_ASSIGN 321
#define AND_ASSIGN 322
#define XOR_ASSIGN 323
#define OR_ASSIGN 324
#define SUB_ASSIGN 325
#define LEFT_PAREN 326
#define RIGHT_PAREN 327
#define LEFT_BRACKET 328
#define RIGHT_BRACKET 329
#define LEFT_BRACE 330
#define RIGHT_BRACE 331
#define DOT 332
#define COMMA 333
#define COLON 334
#define EQUAL 335
#define SEMICOLON 336
#define BANG 337
#define DASH 338
#define TILDE 339
#define PLUS 340
#define STAR 341
#define SLASH 342
#define PERCENT 343
#define LEFT_ANGLE 344
#define RIGHT_ANGLE 345
#define VERTICAL_BAR 346
#define CARET 347
#define AMPERSAND 348
#define QUESTION 349




/* Copy the first part of user declarations.  */


//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// This file is auto-generated by generate_parser.sh. DO NOT EDIT!

// Ignore errors in auto-generated code.
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#elif defined(_MSC_VER)
#pragma warning(disable: 4065)
#pragma warning(disable: 4189)
#pragma warning(disable: 4505)
#pragma warning(disable: 4701)
#endif

#include "compiler/SymbolTable.h"
#include "compiler/ParseHelper.h"
#include "GLSLANG/ShaderLang.h"

#define YYENABLE_NLS 0

#define YYLEX_PARAM context->scanner


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE

{
#define YYLTYPE TSourceLoc
#define YYLTYPE_IS_DECLARED 1
    struct {
        union {
            TString *string;
            float f;
            int i;
            bool b;
        };
        TSymbol* symbol;
    } lex;
    struct {
        TOperator op;
        union {
            TIntermNode* intermNode;
            TIntermNodePair nodePair;
            TIntermTyped* intermTypedNode;
            TIntermAggregate* intermAggregate;
        };
        union {
            TPublicType type;
            TPrecision precision;
            TQualifier qualifier;
            TFunction* function;
            TParameter param;
            TField* field;
            TFieldList* fieldList;
        };
    } interm;
}
/* Line 193 of yacc.c.  */

	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


extern int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, void* yyscanner);
static void yyerror(YYLTYPE* yylloc, TParseContext* context, const char* reason);

#define YYLLOC_DEFAULT(Current, Rhs, N)                      \
  do {                                                       \
      if (YYID(N)) {                                         \
        (Current).first_file = YYRHSLOC(Rhs, 1).first_file;  \
        (Current).first_line = YYRHSLOC(Rhs, 1).first_line;  \
        (Current).last_file = YYRHSLOC(Rhs, N).last_file;    \
        (Current).last_line = YYRHSLOC(Rhs, N).last_line;    \
      }                                                      \
      else {                                                 \
        (Current).first_file = YYRHSLOC(Rhs, 0).last_file;   \
        (Current).first_line = YYRHSLOC(Rhs, 0).last_line;   \
        (Current).last_file = YYRHSLOC(Rhs, 0).last_file;    \
        (Current).last_line = YYRHSLOC(Rhs, 0).last_line;    \
      }                                                      \
  } while (0)

#define VERTEX_ONLY(S, L) {  \
    if (context->shaderType != SH_VERTEX_SHADER) {  \
        context->error(L, " supported in vertex shaders only ", S);  \
        context->recover();  \
    }  \
}

#define FRAG_ONLY(S, L) {  \
    if (context->shaderType != SH_FRAGMENT_SHADER) {  \
        context->error(L, " supported in fragment shaders only ", S);  \
        context->recover();  \
    }  \
}


/* Line 216 of yacc.c.  */


#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  74
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1490

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  95
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  84
/* YYNRULES -- Number of rules.  */
#define YYNRULES  202
/* YYNRULES -- Number of states.  */
#define YYNSTATES  307

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   349

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      21,    23,    28,    30,    34,    37,    40,    42,    44,    46,
      50,    53,    56,    59,    61,    64,    68,    71,    73,    75,
      77,    80,    83,    86,    88,    90,    92,    94,    98,   102,
     104,   108,   112,   114,   116,   120,   124,   128,   132,   134,
     138,   142,   144,   146,   148,   150,   154,   156,   160,   162,
     166,   168,   174,   176,   180,   182,   184,   186,   188,   190,
     192,   196,   198,   201,   204,   209,   212,   214,   216,   219,
     223,   227,   230,   236,   240,   243,   247,   250,   251,   253,
     255,   257,   259,   261,   265,   271,   278,   284,   286,   289,
     294,   300,   305,   308,   310,   313,   315,   317,   319,   322,
     324,   326,   329,   331,   333,   335,   337,   342,   344,   346,
     348,   350,   352,   354,   356,   358,   360,   362,   364,   366,
     368,   370,   372,   374,   376,   378,   380,   382,   384,   386,
     387,   394,   395,   401,   403,   406,   410,   412,   416,   418,
     423,   425,   427,   429,   431,   433,   435,   437,   439,   441,
     444,   445,   446,   452,   454,   456,   457,   460,   461,   464,
     467,   471,   473,   476,   478,   481,   487,   491,   493,   495,
     500,   501,   508,   509,   518,   519,   527,   529,   531,   533,
     534,   537,   541,   544,   547,   550,   554,   557,   559,   562,
     564,   566,   567
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     175,     0,    -1,    45,    -1,    46,    -1,    45,    -1,    97,
      -1,    48,    -1,    47,    -1,    49,    -1,    71,   124,    72,
      -1,    98,    -1,    99,    73,   100,    74,    -1,   101,    -1,
      99,    77,    96,    -1,    99,    52,    -1,    99,    53,    -1,
     124,    -1,   102,    -1,   103,    -1,    99,    77,   103,    -1,
     105,    72,    -1,   104,    72,    -1,   106,    39,    -1,   106,
      -1,   106,   122,    -1,   105,    78,   122,    -1,   107,    71,
      -1,   142,    -1,    45,    -1,    99,    -1,    52,   108,    -1,
      53,   108,    -1,   109,   108,    -1,    85,    -1,    83,    -1,
      82,    -1,   108,    -1,   110,    86,   108,    -1,   110,    87,
     108,    -1,   110,    -1,   111,    85,   110,    -1,   111,    83,
     110,    -1,   111,    -1,   112,    -1,   113,    89,   112,    -1,
     113,    90,   112,    -1,   113,    54,   112,    -1,   113,    55,
     112,    -1,   113,    -1,   114,    56,   113,    -1,   114,    57,
     113,    -1,   114,    -1,   115,    -1,   116,    -1,   117,    -1,
     118,    58,   117,    -1,   118,    -1,   119,    60,   118,    -1,
     119,    -1,   120,    59,   119,    -1,   120,    -1,   120,    94,
     124,    79,   122,    -1,   121,    -1,   108,   123,   122,    -1,
      80,    -1,    61,    -1,    62,    -1,    63,    -1,    70,    -1,
     122,    -1,   124,    78,   122,    -1,   121,    -1,   127,    81,
      -1,   135,    81,    -1,     7,   140,   141,    81,    -1,   128,
      72,    -1,   130,    -1,   129,    -1,   130,   132,    -1,   129,
      78,   132,    -1,   137,    45,    71,    -1,   139,    96,    -1,
     139,    96,    73,   125,    74,    -1,   138,   133,   131,    -1,
     133,   131,    -1,   138,   133,   134,    -1,   133,   134,    -1,
      -1,    33,    -1,    34,    -1,    35,    -1,   139,    -1,   136,
      -1,   135,    78,    96,    -1,   135,    78,    96,    73,    74,
      -1,   135,    78,    96,    73,   125,    74,    -1,   135,    78,
      96,    80,   150,    -1,   137,    -1,   137,    96,    -1,   137,
      96,    73,    74,    -1,   137,    96,    73,   125,    74,    -1,
     137,    96,    80,   150,    -1,     3,    45,    -1,   139,    -1,
     138,   139,    -1,     9,    -1,     8,    -1,    37,    -1,     3,
      37,    -1,    36,    -1,   141,    -1,   140,   141,    -1,     4,
      -1,     5,    -1,     6,    -1,   142,    -1,   142,    73,   125,
      74,    -1,    39,    -1,    11,    -1,    12,    -1,    10,    -1,
      27,    -1,    28,    -1,    29,    -1,    21,    -1,    22,    -1,
      23,    -1,    24,    -1,    25,    -1,    26,    -1,    30,    -1,
      31,    -1,    32,    -1,    41,    -1,    42,    -1,    43,    -1,
      44,    -1,   143,    -1,    46,    -1,    -1,    38,    96,    75,
     144,   146,    76,    -1,    -1,    38,    75,   145,   146,    76,
      -1,   147,    -1,   146,   147,    -1,   139,   148,    81,    -1,
     149,    -1,   148,    78,   149,    -1,    96,    -1,    96,    73,
     125,    74,    -1,   122,    -1,   126,    -1,   154,    -1,   153,
      -1,   151,    -1,   163,    -1,   164,    -1,   167,    -1,   174,
      -1,    75,    76,    -1,    -1,    -1,    75,   155,   162,   156,
      76,    -1,   161,    -1,   153,    -1,    -1,   159,   161,    -1,
      -1,   160,   153,    -1,    75,    76,    -1,    75,   162,    76,
      -1,   152,    -1,   162,   152,    -1,    81,    -1,   124,    81,
      -1,    18,    71,   124,    72,   165,    -1,   158,    16,   158,
      -1,   158,    -1,   124,    -1,   137,    96,    80,   150,    -1,
      -1,    40,    71,   168,   166,    72,   157,    -1,    -1,    15,
     169,   158,    40,    71,   124,    72,    81,    -1,    -1,    17,
      71,   170,   171,   173,    72,   157,    -1,   163,    -1,   151,
      -1,   166,    -1,    -1,   172,    81,    -1,   172,    81,   124,
      -1,    14,    81,    -1,    13,    81,    -1,    20,    81,    -1,
      20,   124,    81,    -1,    19,    81,    -1,   176,    -1,   175,
     176,    -1,   177,    -1,   126,    -1,    -1,   127,   178,   161,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   176,   176,   177,   180,   223,   226,   239,   244,   249,
     255,   258,   261,   264,   359,   369,   382,   390,   490,   493,
     501,   504,   510,   514,   521,   527,   536,   544,   599,   609,
     612,   622,   632,   653,   654,   655,   660,   661,   669,   680,
     681,   689,   700,   704,   705,   715,   725,   735,   748,   749,
     759,   772,   776,   780,   784,   785,   798,   799,   812,   813,
     826,   827,   844,   845,   858,   859,   860,   861,   862,   866,
     869,   880,   888,   915,   920,   934,   989,   992,   999,  1007,
    1028,  1049,  1059,  1087,  1092,  1102,  1107,  1117,  1120,  1123,
    1126,  1132,  1139,  1142,  1164,  1182,  1206,  1229,  1233,  1251,
    1259,  1291,  1311,  1332,  1341,  1364,  1367,  1373,  1381,  1389,
    1397,  1407,  1414,  1417,  1420,  1426,  1429,  1444,  1448,  1452,
    1456,  1460,  1465,  1470,  1475,  1480,  1485,  1490,  1495,  1500,
    1505,  1510,  1515,  1520,  1524,  1528,  1536,  1544,  1548,  1561,
    1561,  1575,  1575,  1584,  1587,  1603,  1636,  1640,  1646,  1653,
    1668,  1672,  1676,  1677,  1683,  1684,  1685,  1686,  1687,  1691,
    1692,  1692,  1692,  1702,  1703,  1707,  1707,  1708,  1708,  1713,
    1716,  1726,  1729,  1735,  1736,  1740,  1748,  1752,  1762,  1767,
    1784,  1784,  1789,  1789,  1796,  1796,  1804,  1807,  1813,  1816,
    1822,  1826,  1833,  1840,  1847,  1854,  1865,  1874,  1878,  1885,
    1888,  1894,  1894
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INVARIANT", "HIGH_PRECISION",
  "MEDIUM_PRECISION", "LOW_PRECISION", "PRECISION", "ATTRIBUTE",
  "CONST_QUAL", "BOOL_TYPE", "FLOAT_TYPE", "INT_TYPE", "BREAK", "CONTINUE",
  "DO", "ELSE", "FOR", "IF", "DISCARD", "RETURN", "BVEC2", "BVEC3",
  "BVEC4", "IVEC2", "IVEC3", "IVEC4", "VEC2", "VEC3", "VEC4", "MATRIX2",
  "MATRIX3", "MATRIX4", "IN_QUAL", "OUT_QUAL", "INOUT_QUAL", "UNIFORM",
  "VARYING", "STRUCT", "VOID_TYPE", "WHILE", "SAMPLER2D", "SAMPLERCUBE",
  "SAMPLER_EXTERNAL_OES", "SAMPLER2DRECT", "IDENTIFIER", "TYPE_NAME",
  "FLOATCONSTANT", "INTCONSTANT", "BOOLCONSTANT", "LEFT_OP", "RIGHT_OP",
  "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP", "AND_OP",
  "OR_OP", "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN", "ADD_ASSIGN",
  "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN",
  "OR_ASSIGN", "SUB_ASSIGN", "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACKET",
  "RIGHT_BRACKET", "LEFT_BRACE", "RIGHT_BRACE", "DOT", "COMMA", "COLON",
  "EQUAL", "SEMICOLON", "BANG", "DASH", "TILDE", "PLUS", "STAR", "SLASH",
  "PERCENT", "LEFT_ANGLE", "RIGHT_ANGLE", "VERTICAL_BAR", "CARET",
  "AMPERSAND", "QUESTION", "$accept", "identifier", "variable_identifier",
  "primary_expression", "postfix_expression", "integer_expression",
  "function_call", "function_call_or_method", "function_call_generic",
  "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "unary_expression", "unary_operator",
  "multiplicative_expression", "additive_expression", "shift_expression",
  "relational_expression", "equality_expression", "and_expression",
  "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "declaration", "function_prototype",
  "function_declarator", "function_header_with_parameters",
  "function_header", "parameter_declarator", "parameter_declaration",
  "parameter_qualifier", "parameter_type_specifier",
  "init_declarator_list", "single_declaration", "fully_specified_type",
  "type_qualifier", "type_specifier", "precision_qualifier",
  "type_specifier_no_prec", "type_specifier_nonarray", "struct_specifier",
  "@1", "@2", "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "simple_statement",
  "compound_statement", "@3", "@4", "statement_no_new_scope",
  "statement_with_scope", "@5", "@6", "compound_statement_no_new_scope",
  "statement_list", "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "iteration_statement", "@7",
  "@8", "@9", "for_init_statement", "conditionopt", "for_rest_statement",
  "jump_statement", "translation_unit", "external_declaration",
  "function_definition", "@10", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    95,    96,    96,    97,    98,    98,    98,    98,    98,
      99,    99,    99,    99,    99,    99,   100,   101,   102,   102,
     103,   103,   104,   104,   105,   105,   106,   107,   107,   108,
     108,   108,   108,   109,   109,   109,   110,   110,   110,   111,
     111,   111,   112,   113,   113,   113,   113,   113,   114,   114,
     114,   115,   116,   117,   118,   118,   119,   119,   120,   120,
     121,   121,   122,   122,   123,   123,   123,   123,   123,   124,
     124,   125,   126,   126,   126,   127,   128,   128,   129,   129,
     130,   131,   131,   132,   132,   132,   132,   133,   133,   133,
     133,   134,   135,   135,   135,   135,   135,   136,   136,   136,
     136,   136,   136,   137,   137,   138,   138,   138,   138,   138,
     139,   139,   140,   140,   140,   141,   141,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   144,
     143,   145,   143,   146,   146,   147,   148,   148,   149,   149,
     150,   151,   152,   152,   153,   153,   153,   153,   153,   154,
     155,   156,   154,   157,   157,   159,   158,   160,   158,   161,
     161,   162,   162,   163,   163,   164,   165,   165,   166,   166,
     168,   167,   169,   167,   170,   167,   171,   171,   172,   172,
     173,   173,   174,   174,   174,   174,   174,   175,   175,   176,
     176,   178,   177
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     4,     1,     3,     2,     2,     1,     1,     1,     3,
       2,     2,     2,     1,     2,     3,     2,     1,     1,     1,
       2,     2,     2,     1,     1,     1,     1,     3,     3,     1,
       3,     3,     1,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     1,     1,     1,     3,     1,     3,     1,     3,
       1,     5,     1,     3,     1,     1,     1,     1,     1,     1,
       3,     1,     2,     2,     4,     2,     1,     1,     2,     3,
       3,     2,     5,     3,     2,     3,     2,     0,     1,     1,
       1,     1,     1,     3,     5,     6,     5,     1,     2,     4,
       5,     4,     2,     1,     2,     1,     1,     1,     2,     1,
       1,     2,     1,     1,     1,     1,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       6,     0,     5,     1,     2,     3,     1,     3,     1,     4,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       0,     0,     5,     1,     1,     0,     2,     0,     2,     2,
       3,     1,     2,     1,     2,     5,     3,     1,     1,     4,
       0,     6,     0,     8,     0,     7,     1,     1,     1,     0,
       2,     3,     2,     2,     2,     3,     2,     1,     2,     1,
       1,     0,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,   112,   113,   114,     0,   106,   105,   120,   118,
     119,   124,   125,   126,   127,   128,   129,   121,   122,   123,
     130,   131,   132,   109,   107,     0,   117,   133,   134,   135,
     136,   138,   200,   201,     0,    77,    87,     0,    92,    97,
       0,   103,     0,   110,   115,   137,     0,   197,   199,   108,
     102,     0,     2,     3,   141,     0,    72,     0,    75,    87,
       0,    88,    89,    90,    78,     0,    87,     0,    73,     2,
      98,   104,   111,     0,     1,   198,     0,     0,   139,     0,
     202,    79,    84,    86,    91,     0,    93,    80,     0,     0,
       4,     7,     6,     8,     0,     0,     0,    35,    34,    33,
       5,    10,    29,    12,    17,    18,     0,     0,    23,     0,
      36,     0,    39,    42,    43,    48,    51,    52,    53,    54,
      56,    58,    60,    71,     0,    27,    74,     0,     0,   143,
       0,     0,     0,   182,     0,     0,     0,     0,     0,   160,
     169,   173,    36,    62,    69,     0,   151,     0,   115,   154,
     171,   153,   152,     0,   155,   156,   157,   158,    81,    83,
      85,     0,     0,    99,     0,   150,   101,    30,    31,     0,
      14,    15,     0,     0,    21,    20,     0,    22,    24,    26,
      32,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   116,   148,     0,   146,   142,
     144,     0,   193,   192,   167,   184,     0,   196,   194,     0,
     180,   159,     0,    65,    66,    67,    68,    64,     0,     0,
     174,   170,   172,     0,    94,     0,    96,   100,     9,     0,
      16,     2,     3,    13,    19,    25,    37,    38,    41,    40,
      46,    47,    44,    45,    49,    50,    55,    57,    59,     0,
       0,     0,   145,   140,     0,     0,     0,     0,     0,   195,
       0,   161,    63,    70,     0,    95,    11,     0,     0,   147,
       0,   166,   168,   187,   186,   189,   167,   178,     0,     0,
       0,    82,    61,   149,     0,   188,     0,     0,   177,   175,
       0,     0,   162,     0,   190,     0,   167,     0,   164,   181,
     163,     0,   191,   185,   176,   179,   183
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   196,   100,   101,   102,   229,   103,   104,   105,   106,
     107,   108,   109,   142,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   143,   144,   218,   145,
     124,   146,   147,    34,    35,    36,    82,    64,    65,    83,
      37,    38,    39,    40,    41,    42,    43,   125,    45,   130,
      77,   128,   129,   197,   198,   166,   149,   150,   151,   152,
     212,   280,   299,   254,   255,   256,   300,   153,   154,   155,
     289,   279,   156,   260,   204,   257,   275,   286,   287,   157,
      46,    47,    48,    57
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -261
static const yytype_int16 yypact[] =
{
    1327,   -20,  -261,  -261,  -261,   113,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,  -261,  -261,   -19,  -261,  -261,  -261,  -261,
    -261,  -261,  -261,   -61,   -40,   -28,    75,    -7,  -261,    24,
    1370,  -261,  1444,  -261,   -11,  -261,  1283,  -261,  -261,  -261,
    -261,  1444,  -261,  -261,  -261,     6,  -261,    54,  -261,    88,
      62,  -261,  -261,  -261,  -261,  1370,    59,    91,  -261,    36,
     -50,  -261,  -261,  1051,  -261,  -261,    63,  1370,  -261,   293,
    -261,  -261,  -261,  -261,    91,  1370,   -12,  -261,   856,  1051,
      77,  -261,  -261,  -261,  1051,  1051,  1051,  -261,  -261,  -261,
    -261,  -261,   -14,  -261,  -261,  -261,    84,   -44,  1116,    95,
    -261,  1051,    53,     3,  -261,   -36,    89,  -261,  -261,  -261,
     104,   107,   -45,  -261,    96,  -261,  -261,    91,  1184,  -261,
    1370,    92,    93,  -261,    98,   101,    94,   921,   105,   102,
    -261,  -261,    72,  -261,  -261,     9,  -261,   -61,    42,  -261,
    -261,  -261,  -261,   376,  -261,  -261,  -261,  -261,   106,  -261,
    -261,   986,  1051,  -261,   103,  -261,  -261,  -261,  -261,   -41,
    -261,  -261,  1051,  1407,  -261,  -261,  1051,   110,  -261,  -261,
    -261,  1051,  1051,  1051,  1051,  1051,  1051,  1051,  1051,  1051,
    1051,  1051,  1051,  1051,  1051,  -261,   109,    23,  -261,  -261,
    -261,  1227,  -261,  -261,   111,  -261,  1051,  -261,  -261,    25,
    -261,  -261,   459,  -261,  -261,  -261,  -261,  -261,  1051,  1051,
    -261,  -261,  -261,  1051,  -261,   114,  -261,  -261,  -261,   115,
     112,    77,   116,  -261,  -261,  -261,  -261,  -261,    53,    53,
    -261,  -261,  -261,  -261,   -36,   -36,  -261,   104,   107,    76,
    1051,    91,  -261,  -261,   145,    54,   625,   708,    -6,  -261,
     791,   459,  -261,  -261,   117,  -261,  -261,  1051,   120,  -261,
     124,  -261,  -261,  -261,  -261,   791,   111,   112,    91,   125,
     122,  -261,  -261,  -261,  1051,  -261,   118,   128,   180,  -261,
     126,   542,  -261,    -5,  1051,   542,   111,  1051,  -261,  -261,
    -261,   123,   112,  -261,  -261,  -261,  -261
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -261,   -24,  -261,  -261,  -261,  -261,  -261,  -261,    34,  -261,
    -261,  -261,  -261,    32,  -261,   -33,  -261,   -27,   -26,  -261,
    -261,  -261,    14,    16,    18,  -261,   -66,   -87,  -261,   -92,
     -85,    11,    12,  -261,  -261,  -261,   141,   150,   161,   143,
    -261,  -261,  -231,     5,   -30,   224,   -18,     0,  -261,  -261,
    -261,   100,  -119,  -261,   -17,  -156,   -25,  -145,  -243,  -261,
    -261,  -261,   -64,  -260,  -261,  -261,   -52,    21,   -22,  -261,
    -261,   -39,  -261,  -261,  -261,  -261,  -261,  -261,  -261,  -261,
    -261,   191,  -261,  -261
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -166
static const yytype_int16 yytable[] =
{
      44,    55,   165,   164,   169,    80,   226,   123,   222,   200,
      71,    32,    33,   272,   193,    70,   288,    49,   185,   186,
      56,   178,   123,    88,    72,    50,    52,    53,   175,   278,
      89,   228,    58,    76,   176,    84,   304,   219,   170,   171,
      44,    66,    44,    86,   278,   209,    44,   127,   298,   194,
      59,    44,   298,   187,   188,    84,    54,    32,    33,   172,
     158,   161,    73,   173,    66,    44,   276,   301,   162,    69,
      53,    67,   219,   219,    68,   165,   225,    44,    60,   148,
     230,    78,   200,     6,     7,    44,   183,   219,   184,   235,
     220,    60,    61,    62,    63,   123,     6,     7,   127,    49,
     127,   251,   249,   219,   252,   110,   259,    87,    61,    62,
      63,    23,    24,   -27,   258,    73,   222,     2,     3,     4,
     110,    61,    62,    63,    23,    24,   167,   168,    44,    79,
      44,   262,   263,   213,   214,   215,    52,    53,   264,   181,
     182,   305,   216,   180,   126,   189,   190,   -76,   -28,   233,
     238,   239,   217,   148,   219,   267,   174,   123,   240,   241,
     242,   243,   191,   244,   245,   268,   179,   192,   277,   205,
     195,   127,   206,   202,   203,   207,   210,   227,   211,   223,
     282,  -117,   250,   277,   123,   270,  -165,  -138,   265,   266,
     219,   281,   293,   110,   283,   284,   296,   291,   292,   294,
     295,    44,   302,   271,   306,   246,   297,   234,   247,    81,
     165,   248,   148,   236,   237,   110,   110,   110,   110,   110,
     110,   110,   110,   110,   110,   110,   159,    85,   160,    51,
     201,   303,   273,   261,   269,   274,   285,    75,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   290,   110,   148,   148,     0,     0,
     148,   148,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   148,     0,     0,     0,     0,
       0,     0,   110,     0,     0,     0,     0,     0,     0,     0,
       0,   148,     0,     0,     0,   148,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,   131,   132,   133,     0,
     134,   135,   136,   137,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,     0,     0,     0,    23,
      24,    25,    26,   138,    27,    28,    29,    30,    90,    31,
      91,    92,    93,     0,     0,    94,    95,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    96,     0,     0,     0,   139,   140,
       0,     0,     0,     0,   141,    97,    98,     0,    99,     1,
       2,     3,     4,     5,     6,     7,     8,     9,    10,   131,
     132,   133,     0,   134,   135,   136,   137,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,    23,    24,    25,    26,   138,    27,    28,    29,
      30,    90,    31,    91,    92,    93,     0,     0,    94,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    96,     0,     0,
       0,   139,   221,     0,     0,     0,     0,   141,    97,    98,
       0,    99,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,   131,   132,   133,     0,   134,   135,   136,   137,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,     0,     0,     0,    23,    24,    25,    26,   138,
      27,    28,    29,    30,    90,    31,    91,    92,    93,     0,
       0,    94,    95,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      96,     0,     0,     0,   139,     0,     0,     0,     0,     0,
     141,    97,    98,     0,    99,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,   131,   132,   133,     0,   134,
     135,   136,   137,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,     0,     0,     0,    23,    24,
      25,    26,   138,    27,    28,    29,    30,    90,    31,    91,
      92,    93,     0,     0,    94,    95,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    96,     0,     0,     0,    79,     0,     0,
       0,     0,     0,   141,    97,    98,     0,    99,     1,     2,
       3,     4,     5,     6,     7,     8,     9,    10,   131,   132,
     133,     0,   134,   135,   136,   137,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,     0,     0,
       0,    23,    24,    25,    26,   138,    27,    28,    29,    30,
      90,    31,    91,    92,    93,     0,     0,    94,    95,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    96,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   141,    97,    98,     0,
      99,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,     0,     0,     0,     0,     0,     0,     0,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,     0,     0,     0,    23,    24,    25,    26,     0,    27,
      28,    29,    30,    90,    31,    91,    92,    93,     0,     0,
      94,    95,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    96,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   141,
      97,    98,     0,    99,    60,     2,     3,     4,     0,     6,
       7,     8,     9,    10,     0,     0,     0,     0,     0,     0,
       0,     0,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,    23,    24,    25,
      26,     0,    27,    28,    29,    30,    90,    31,    91,    92,
      93,     0,     0,    94,    95,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    96,     0,     0,     0,     8,     9,    10,     0,
       0,     0,     0,    97,    98,     0,    99,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,    25,    26,     0,    27,    28,    29,
      30,    90,    31,    91,    92,    93,     0,     0,    94,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    96,     0,     0,
     163,     8,     9,    10,     0,     0,     0,     0,    97,    98,
       0,    99,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     0,    25,
      26,     0,    27,    28,    29,    30,    90,    31,    91,    92,
      93,     0,     0,    94,    95,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    96,     0,     0,     0,     8,     9,    10,     0,
       0,     0,   208,    97,    98,     0,    99,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,    25,    26,     0,    27,    28,    29,
      30,    90,    31,    91,    92,    93,     0,     0,    94,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    96,     0,     0,
     224,     8,     9,    10,     0,     0,     0,     0,    97,    98,
       0,    99,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     0,    25,
      26,     0,    27,    28,    29,    30,    90,    31,    91,    92,
      93,     0,     0,    94,    95,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    96,     0,     0,     0,     8,     9,    10,     0,
       0,     0,     0,    97,    98,     0,    99,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,    25,   177,     0,    27,    28,    29,
      30,    90,    31,    91,    92,    93,     0,     0,    94,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    96,     2,     3,
       4,     0,     0,     0,     8,     9,    10,     0,    97,    98,
       0,    99,     0,     0,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,     0,    25,    26,     0,    27,    28,    29,    30,     0,
      31,     2,     3,     4,     0,     0,     0,     8,     9,    10,
       0,     0,     0,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
     199,     0,     0,     0,     0,    25,    26,     0,    27,    28,
      29,    30,     0,    31,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    74,     0,     0,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,     0,     0,     0,     0,
       0,     0,     0,   253,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,     0,     0,     0,    23,
      24,    25,    26,     0,    27,    28,    29,    30,     0,    31,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
       0,     0,     0,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
       0,     0,     0,    23,    24,    25,    26,     0,    27,    28,
      29,    30,     0,    31,     2,     3,     4,     0,     0,     0,
       8,     9,    10,     0,     0,     0,     0,     0,     0,     0,
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     0,     0,     0,     0,    25,    26,
       0,    27,    28,    29,    30,     0,    31,     8,     9,    10,
       0,     0,     0,     0,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
       0,     0,     0,     0,     0,    25,    26,     0,    27,    28,
      29,    30,   231,   232,     8,     9,    10,     0,     0,     0,
       0,     0,     0,     0,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,     0,    25,    26,     0,    27,    28,    29,    30,     0,
      31
};

static const yytype_int16 yycheck[] =
{
       0,    25,    89,    88,    96,    57,   162,    73,   153,   128,
      40,     0,     0,   256,    59,    39,   276,    37,    54,    55,
      81,   108,    88,    73,    42,    45,    45,    46,    72,   260,
      80,    72,    72,    51,    78,    65,   296,    78,    52,    53,
      40,    36,    42,    67,   275,   137,    46,    77,   291,    94,
      78,    51,   295,    89,    90,    85,    75,    46,    46,    73,
      84,    73,    73,    77,    59,    65,    72,    72,    80,    45,
      46,    78,    78,    78,    81,   162,   161,    77,     3,    79,
     172,    75,   201,     8,     9,    85,    83,    78,    85,   176,
      81,     3,    33,    34,    35,   161,     8,     9,   128,    37,
     130,    78,   194,    78,    81,    73,    81,    71,    33,    34,
      35,    36,    37,    71,   206,    73,   261,     4,     5,     6,
      88,    33,    34,    35,    36,    37,    94,    95,   128,    75,
     130,   218,   219,    61,    62,    63,    45,    46,   223,    86,
      87,   297,    70,   111,    81,    56,    57,    72,    71,   173,
     183,   184,    80,   153,    78,    79,    72,   223,   185,   186,
     187,   188,    58,   189,   190,   250,    71,    60,   260,    71,
      74,   201,    71,    81,    81,    81,    71,    74,    76,    73,
     267,    71,    73,   275,   250,    40,    75,    71,    74,    74,
      78,    74,   284,   161,    74,    71,    16,    72,    76,    81,
      72,   201,   294,   255,    81,   191,    80,   173,   192,    59,
     297,   193,   212,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,    85,    66,    85,     5,
     130,   295,   257,   212,   251,   257,   275,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   278,   223,   256,   257,    -1,    -1,
     260,   261,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   275,    -1,    -1,    -1,    -1,
      -1,    -1,   250,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   291,    -1,    -1,    -1,   295,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    -1,    -1,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    -1,    -1,    -1,    75,    76,
      -1,    -1,    -1,    -1,    81,    82,    83,    -1,    85,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    -1,    -1,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,
      -1,    75,    76,    -1,    -1,    -1,    -1,    81,    82,    83,
      -1,    85,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    -1,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    -1,
      -1,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      71,    -1,    -1,    -1,    75,    -1,    -1,    -1,    -1,    -1,
      81,    82,    83,    -1,    85,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    -1,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    -1,    -1,    52,    53,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    -1,    -1,    -1,    75,    -1,    -1,
      -1,    -1,    -1,    81,    82,    83,    -1,    85,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    -1,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    -1,    -1,    52,    53,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    81,    82,    83,    -1,
      85,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    -1,    -1,    -1,    36,    37,    38,    39,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    -1,    -1,
      52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,
      82,    83,    -1,    85,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    -1,    36,    37,    38,
      39,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    -1,    -1,    52,    53,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    -1,    -1,    -1,    10,    11,    12,    -1,
      -1,    -1,    -1,    82,    83,    -1,    85,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,    39,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    -1,    -1,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,
      74,    10,    11,    12,    -1,    -1,    -1,    -1,    82,    83,
      -1,    85,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    -1,    -1,    -1,    38,
      39,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    -1,    -1,    52,    53,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    -1,    -1,    -1,    10,    11,    12,    -1,
      -1,    -1,    81,    82,    83,    -1,    85,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,    39,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    -1,    -1,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,
      74,    10,    11,    12,    -1,    -1,    -1,    -1,    82,    83,
      -1,    85,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    -1,    -1,    -1,    38,
      39,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    -1,    -1,    52,    53,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    -1,    -1,    -1,    10,    11,    12,    -1,
      -1,    -1,    -1,    82,    83,    -1,    85,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    -1,    -1,    -1,    38,    39,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    -1,    -1,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,     4,     5,
       6,    -1,    -1,    -1,    10,    11,    12,    -1,    82,    83,
      -1,    85,    -1,    -1,    -1,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    39,    -1,    41,    42,    43,    44,    -1,
      46,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      76,    -1,    -1,    -1,    -1,    38,    39,    -1,    41,    42,
      43,    44,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     0,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    76,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    -1,    -1,    36,
      37,    38,    39,    -1,    41,    42,    43,    44,    -1,    46,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    -1,    36,    37,    38,    39,    -1,    41,    42,
      43,    44,    -1,    46,     4,     5,     6,    -1,    -1,    -1,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    -1,    -1,    -1,    -1,    -1,    38,    39,
      -1,    41,    42,    43,    44,    -1,    46,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    -1,    -1,    -1,    38,    39,    -1,    41,    42,
      43,    44,    45,    46,    10,    11,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    -1,
      -1,    -1,    38,    39,    -1,    41,    42,    43,    44,    -1,
      46
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    36,    37,    38,    39,    41,    42,    43,
      44,    46,   126,   127,   128,   129,   130,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   175,   176,   177,    37,
      45,   140,    45,    46,    75,    96,    81,   178,    72,    78,
       3,    33,    34,    35,   132,   133,   138,    78,    81,    45,
      96,   139,   141,    73,     0,   176,   141,   145,    75,    75,
     161,   132,   131,   134,   139,   133,    96,    71,    73,    80,
      45,    47,    48,    49,    52,    53,    71,    82,    83,    85,
      97,    98,    99,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   125,   142,    81,   139,   146,   147,
     144,    13,    14,    15,    17,    18,    19,    20,    40,    75,
      76,    81,   108,   121,   122,   124,   126,   127,   142,   151,
     152,   153,   154,   162,   163,   164,   167,   174,    96,   131,
     134,    73,    80,    74,   125,   122,   150,   108,   108,   124,
      52,    53,    73,    77,    72,    72,    78,    39,   122,    71,
     108,    86,    87,    83,    85,    54,    55,    89,    90,    56,
      57,    58,    60,    59,    94,    74,    96,   148,   149,    76,
     147,   146,    81,    81,   169,    71,    71,    81,    81,   124,
      71,    76,   155,    61,    62,    63,    70,    80,   123,    78,
      81,    76,   152,    73,    74,   125,   150,    74,    72,   100,
     124,    45,    46,    96,   103,   122,   108,   108,   110,   110,
     112,   112,   112,   112,   113,   113,   117,   118,   119,   124,
      73,    78,    81,    76,   158,   159,   160,   170,   124,    81,
     168,   162,   122,   122,   125,    74,    74,    79,   125,   149,
      40,   161,   153,   151,   163,   171,    72,   124,   137,   166,
     156,    74,   122,    74,    71,   166,   172,   173,   158,   165,
      96,    72,    76,   124,    81,    72,    16,    80,   153,   157,
     161,    72,   124,   157,   158,   150,    81
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, context, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, context); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, TParseContext* context)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    TParseContext* context;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (context);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, TParseContext* context)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    TParseContext* context;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, context);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, TParseContext* context)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, context)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    TParseContext* context;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , context);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, context); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, TParseContext* context)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, context)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    TParseContext* context;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (context);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (TParseContext* context);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (TParseContext* context)
#else
int
yyparse (context)
    TParseContext* context;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:

    {
        // The symbol table search was done in the lexical phase
        const TSymbol* symbol = (yyvsp[(1) - (1)].lex).symbol;
        const TVariable* variable;
        if (symbol == 0) {
            context->error((yylsp[(1) - (1)]), "undeclared identifier", (yyvsp[(1) - (1)].lex).string->c_str());
            context->recover();
            TType type(EbtFloat, EbpUndefined);
            TVariable* fakeVariable = new TVariable((yyvsp[(1) - (1)].lex).string, type);
            context->symbolTable.insert(*fakeVariable);
            variable = fakeVariable;
        } else {
            // This identifier can only be a variable type symbol
            if (! symbol->isVariable()) {
                context->error((yylsp[(1) - (1)]), "variable expected", (yyvsp[(1) - (1)].lex).string->c_str());
                context->recover();
            }
            
            variable = static_cast<const TVariable*>(symbol);

            if (context->symbolTable.findBuiltIn(variable->getName()) &&
                !variable->getExtension().empty() &&
                context->extensionErrorCheck((yylsp[(1) - (1)]), variable->getExtension())) {
                context->recover();
            }
        }

        // don't delete $1.string, it's used by error recovery, and the pool
        // pop will reclaim the memory

        if (variable->getType().getQualifier() == EvqConst ) {
            ConstantUnion* constArray = variable->getConstPointer();
            TType t(variable->getType());
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(constArray, t, (yylsp[(1) - (1)]));
        } else
            (yyval.interm.intermTypedNode) = context->intermediate.addSymbol(variable->getUniqueId(),
                                                 variable->getName(),
                                                 variable->getType(),
                                                 (yylsp[(1) - (1)]));
    ;}
    break;

  case 5:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 6:

    {
        //
        // INT_TYPE is only 16-bit plus sign bit for vertex/fragment shaders,
        // check for overflow for constants
        //
        if (abs((yyvsp[(1) - (1)].lex).i) >= (1 << 16)) {
            context->error((yylsp[(1) - (1)]), " integer constant overflow", "");
            context->recover();
        }
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setIConst((yyvsp[(1) - (1)].lex).i);
        (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
    ;}
    break;

  case 7:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setFConst((yyvsp[(1) - (1)].lex).f);
        (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
    ;}
    break;

  case 8:

    {
        ConstantUnion *unionArray = new ConstantUnion[1];
        unionArray->setBConst((yyvsp[(1) - (1)].lex).b);
        (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
    ;}
    break;

  case 9:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(2) - (3)].interm.intermTypedNode);
    ;}
    break;

  case 10:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 11:

    {
        (yyval.interm.intermTypedNode) = context->addIndexExpression((yyvsp[(1) - (4)].interm.intermTypedNode), (yylsp[(2) - (4)]), (yyvsp[(3) - (4)].interm.intermTypedNode));
    ;}
    break;

  case 12:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 13:

    {
        if ((yyvsp[(1) - (3)].interm.intermTypedNode)->isArray()) {
            context->error((yylsp[(3) - (3)]), "cannot apply dot operator to an array", ".");
            context->recover();
        }

        if ((yyvsp[(1) - (3)].interm.intermTypedNode)->isVector()) {
            TVectorFields fields;
            if (! context->parseVectorFields(*(yyvsp[(3) - (3)].lex).string, (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize(), fields, (yylsp[(3) - (3)]))) {
                fields.num = 1;
                fields.offsets[0] = 0;
                context->recover();
            }

            if ((yyvsp[(1) - (3)].interm.intermTypedNode)->getType().getQualifier() == EvqConst) { // constant folding for vector fields
                (yyval.interm.intermTypedNode) = context->addConstVectorNode(fields, (yyvsp[(1) - (3)].interm.intermTypedNode), (yylsp[(3) - (3)]));
                if ((yyval.interm.intermTypedNode) == 0) {
                    context->recover();
                    (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
                }
                else
                    (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision(), EvqConst, (int) (*(yyvsp[(3) - (3)].lex).string).size()));
            } else {
                TString vectorString = *(yyvsp[(3) - (3)].lex).string;
                TIntermTyped* index = context->intermediate.addSwizzle(fields, (yylsp[(3) - (3)]));
                (yyval.interm.intermTypedNode) = context->intermediate.addIndex(EOpVectorSwizzle, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yylsp[(2) - (3)]));
                (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision(), EvqTemporary, (int) vectorString.size()));
            }
        } else if ((yyvsp[(1) - (3)].interm.intermTypedNode)->isMatrix()) {
            TMatrixFields fields;
            if (! context->parseMatrixFields(*(yyvsp[(3) - (3)].lex).string, (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize(), fields, (yylsp[(3) - (3)]))) {
                fields.wholeRow = false;
                fields.wholeCol = false;
                fields.row = 0;
                fields.col = 0;
                context->recover();
            }

            if (fields.wholeRow || fields.wholeCol) {
                context->error((yylsp[(2) - (3)]), " non-scalar fields not implemented yet", ".");
                context->recover();
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setIConst(0);
                TIntermTyped* index = context->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yylsp[(3) - (3)]));
                (yyval.interm.intermTypedNode) = context->intermediate.addIndex(EOpIndexDirect, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yylsp[(2) - (3)]));
                (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision(),EvqTemporary, (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize()));
            } else {
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setIConst(fields.col * (yyvsp[(1) - (3)].interm.intermTypedNode)->getNominalSize() + fields.row);
                TIntermTyped* index = context->intermediate.addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), (yylsp[(3) - (3)]));
                (yyval.interm.intermTypedNode) = context->intermediate.addIndex(EOpIndexDirect, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yylsp[(2) - (3)]));
                (yyval.interm.intermTypedNode)->setType(TType((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType(), (yyvsp[(1) - (3)].interm.intermTypedNode)->getPrecision()));
            }
        } else if ((yyvsp[(1) - (3)].interm.intermTypedNode)->getBasicType() == EbtStruct) {
            bool fieldFound = false;
            const TFieldList& fields = (yyvsp[(1) - (3)].interm.intermTypedNode)->getType().getStruct()->fields();
            unsigned int i;
            for (i = 0; i < fields.size(); ++i) {
                if (fields[i]->name() == *(yyvsp[(3) - (3)].lex).string) {
                    fieldFound = true;
                    break;
                }
            }
            if (fieldFound) {
                if ((yyvsp[(1) - (3)].interm.intermTypedNode)->getType().getQualifier() == EvqConst) {
                    (yyval.interm.intermTypedNode) = context->addConstStruct(*(yyvsp[(3) - (3)].lex).string, (yyvsp[(1) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
                    if ((yyval.interm.intermTypedNode) == 0) {
                        context->recover();
                        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
                    }
                    else {
                        (yyval.interm.intermTypedNode)->setType(*fields[i]->type());
                        // change the qualifier of the return type, not of the structure field
                        // as the structure definition is shared between various structures.
                        (yyval.interm.intermTypedNode)->getTypePointer()->setQualifier(EvqConst);
                    }
                } else {
                    ConstantUnion *unionArray = new ConstantUnion[1];
                    unionArray->setIConst(i);
                    TIntermTyped* index = context->intermediate.addConstantUnion(unionArray, *fields[i]->type(), (yylsp[(3) - (3)]));
                    (yyval.interm.intermTypedNode) = context->intermediate.addIndex(EOpIndexDirectStruct, (yyvsp[(1) - (3)].interm.intermTypedNode), index, (yylsp[(2) - (3)]));
                    (yyval.interm.intermTypedNode)->setType(*fields[i]->type());
                }
            } else {
                context->error((yylsp[(2) - (3)]), " no such field in structure", (yyvsp[(3) - (3)].lex).string->c_str());
                context->recover();
                (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
            }
        } else {
            context->error((yylsp[(2) - (3)]), " field selection requires structure, vector, or matrix on left hand side", (yyvsp[(3) - (3)].lex).string->c_str());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
        // don't delete $3.string, it's from the pool
    ;}
    break;

  case 14:

    {
        if (context->lValueErrorCheck((yylsp[(2) - (2)]), "++", (yyvsp[(1) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPostIncrement, (yyvsp[(1) - (2)].interm.intermTypedNode), (yylsp[(2) - (2)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(2) - (2)]), "++", (yyvsp[(1) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (2)].interm.intermTypedNode);
        }
    ;}
    break;

  case 15:

    {
        if (context->lValueErrorCheck((yylsp[(2) - (2)]), "--", (yyvsp[(1) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPostDecrement, (yyvsp[(1) - (2)].interm.intermTypedNode), (yylsp[(2) - (2)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(2) - (2)]), "--", (yyvsp[(1) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (2)].interm.intermTypedNode);
        }
    ;}
    break;

  case 16:

    {
        if (context->integerErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode), "[]"))
            context->recover();
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 17:

    {
        TFunction* fnCall = (yyvsp[(1) - (1)].interm).function;
        TOperator op = fnCall->getBuiltInOp();

        if (op != EOpNull)
        {
            //
            // Then this should be a constructor.
            // Don't go through the symbol table for constructors.
            // Their parameters will be verified algorithmically.
            //
            TType type(EbtVoid, EbpUndefined);  // use this to get the type back
            if (context->constructorErrorCheck((yylsp[(1) - (1)]), (yyvsp[(1) - (1)].interm).intermNode, *fnCall, op, &type)) {
                (yyval.interm.intermTypedNode) = 0;
            } else {
                //
                // It's a constructor, of type 'type'.
                //
                (yyval.interm.intermTypedNode) = context->addConstructor((yyvsp[(1) - (1)].interm).intermNode, &type, op, fnCall, (yylsp[(1) - (1)]));
            }

            if ((yyval.interm.intermTypedNode) == 0) {
                context->recover();
                (yyval.interm.intermTypedNode) = context->intermediate.setAggregateOperator(0, op, (yylsp[(1) - (1)]));
            }
            (yyval.interm.intermTypedNode)->setType(type);
        } else {
            //
            // Not a constructor.  Find it in the symbol table.
            //
            const TFunction* fnCandidate;
            bool builtIn;
            fnCandidate = context->findFunction((yylsp[(1) - (1)]), fnCall, &builtIn);
            if (fnCandidate) {
                //
                // A declared function.
                //
                if (builtIn && !fnCandidate->getExtension().empty() &&
                    context->extensionErrorCheck((yylsp[(1) - (1)]), fnCandidate->getExtension())) {
                    context->recover();
                }
                op = fnCandidate->getBuiltInOp();
                if (builtIn && op != EOpNull) {
                    //
                    // A function call mapped to a built-in operation.
                    //
                    if (fnCandidate->getParamCount() == 1) {
                        //
                        // Treat it like a built-in unary operator.
                        //
                        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(op, (yyvsp[(1) - (1)].interm).intermNode, (yylsp[(1) - (1)]), context->symbolTable);
                        if ((yyval.interm.intermTypedNode) == 0)  {
                            std::stringstream extraInfoStream;
                            extraInfoStream << "built in unary operator function.  Type: " << static_cast<TIntermTyped*>((yyvsp[(1) - (1)].interm).intermNode)->getCompleteString();
                            std::string extraInfo = extraInfoStream.str();
                            context->error((yyvsp[(1) - (1)].interm).intermNode->getLine(), " wrong operand type", "Internal Error", extraInfo.c_str());
                            YYERROR;
                        }
                    } else {
                        (yyval.interm.intermTypedNode) = context->intermediate.setAggregateOperator((yyvsp[(1) - (1)].interm).intermAggregate, op, (yylsp[(1) - (1)]));
                    }
                } else {
                    // This is a real function call

                    (yyval.interm.intermTypedNode) = context->intermediate.setAggregateOperator((yyvsp[(1) - (1)].interm).intermAggregate, EOpFunctionCall, (yylsp[(1) - (1)]));
                    (yyval.interm.intermTypedNode)->setType(fnCandidate->getReturnType());

                    // this is how we know whether the given function is a builtIn function or a user defined function
                    // if builtIn == false, it's a userDefined -> could be an overloaded builtIn function also
                    // if builtIn == true, it's definitely a builtIn function with EOpNull
                    if (!builtIn)
                        (yyval.interm.intermTypedNode)->getAsAggregate()->setUserDefined();
                    (yyval.interm.intermTypedNode)->getAsAggregate()->setName(fnCandidate->getMangledName());

                    TQualifier qual;
                    for (size_t i = 0; i < fnCandidate->getParamCount(); ++i) {
                        qual = fnCandidate->getParam(i).type->getQualifier();
                        if (qual == EvqOut || qual == EvqInOut) {
                            if (context->lValueErrorCheck((yyval.interm.intermTypedNode)->getLine(), "assign", (yyval.interm.intermTypedNode)->getAsAggregate()->getSequence()[i]->getAsTyped())) {
                                context->error((yyvsp[(1) - (1)].interm).intermNode->getLine(), "Constant value cannot be passed for 'out' or 'inout' parameters.", "Error");
                                context->recover();
                            }
                        }
                    }
                }
                (yyval.interm.intermTypedNode)->setType(fnCandidate->getReturnType());
            } else {
                // error message was put out by PaFindFunction()
                // Put on a dummy node for error recovery
                ConstantUnion *unionArray = new ConstantUnion[1];
                unionArray->setFConst(0.0f);
                (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtFloat, EbpUndefined, EvqConst), (yylsp[(1) - (1)]));
                context->recover();
            }
        }
        delete fnCall;
    ;}
    break;

  case 18:

    {
        (yyval.interm) = (yyvsp[(1) - (1)].interm);
    ;}
    break;

  case 19:

    {
        context->error((yylsp[(3) - (3)]), "methods are not supported", "");
        context->recover();
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
    ;}
    break;

  case 20:

    {
        (yyval.interm) = (yyvsp[(1) - (2)].interm);
    ;}
    break;

  case 21:

    {
        (yyval.interm) = (yyvsp[(1) - (2)].interm);
    ;}
    break;

  case 22:

    {
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);
        (yyval.interm).intermNode = 0;
    ;}
    break;

  case 23:

    {
        (yyval.interm).function = (yyvsp[(1) - (1)].interm.function);
        (yyval.interm).intermNode = 0;
    ;}
    break;

  case 24:

    {
        TParameter param = { 0, new TType((yyvsp[(2) - (2)].interm.intermTypedNode)->getType()) };
        (yyvsp[(1) - (2)].interm.function)->addParameter(param);
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);
        (yyval.interm).intermNode = (yyvsp[(2) - (2)].interm.intermTypedNode);
    ;}
    break;

  case 25:

    {
        TParameter param = { 0, new TType((yyvsp[(3) - (3)].interm.intermTypedNode)->getType()) };
        (yyvsp[(1) - (3)].interm).function->addParameter(param);
        (yyval.interm).function = (yyvsp[(1) - (3)].interm).function;
        (yyval.interm).intermNode = context->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermNode, (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
    ;}
    break;

  case 26:

    {
        (yyval.interm.function) = (yyvsp[(1) - (2)].interm.function);
    ;}
    break;

  case 27:

    {
        //
        // Constructor
        //
        TOperator op = EOpNull;
        if ((yyvsp[(1) - (1)].interm.type).userDef) {
            op = EOpConstructStruct;
        } else {
            switch ((yyvsp[(1) - (1)].interm.type).type) {
            case EbtFloat:
                if ((yyvsp[(1) - (1)].interm.type).matrix) {
                    switch((yyvsp[(1) - (1)].interm.type).size) {
                    case 2: op = EOpConstructMat2;  break;
                    case 3: op = EOpConstructMat3;  break;
                    case 4: op = EOpConstructMat4;  break;
                    }
                } else {
                    switch((yyvsp[(1) - (1)].interm.type).size) {
                    case 1: op = EOpConstructFloat; break;
                    case 2: op = EOpConstructVec2;  break;
                    case 3: op = EOpConstructVec3;  break;
                    case 4: op = EOpConstructVec4;  break;
                    }
                }
                break;
            case EbtInt:
                switch((yyvsp[(1) - (1)].interm.type).size) {
                case 1: op = EOpConstructInt;   break;
                case 2: op = EOpConstructIVec2; break;
                case 3: op = EOpConstructIVec3; break;
                case 4: op = EOpConstructIVec4; break;
                }
                break;
            case EbtBool:
                switch((yyvsp[(1) - (1)].interm.type).size) {
                case 1: op = EOpConstructBool;  break;
                case 2: op = EOpConstructBVec2; break;
                case 3: op = EOpConstructBVec3; break;
                case 4: op = EOpConstructBVec4; break;
                }
                break;
            default: break;
            }
            if (op == EOpNull) {
                context->error((yylsp[(1) - (1)]), "cannot construct this type", getBasicString((yyvsp[(1) - (1)].interm.type).type));
                context->recover();
                (yyvsp[(1) - (1)].interm.type).type = EbtFloat;
                op = EOpConstructFloat;
            }
        }
        TString tempString;
        TType type((yyvsp[(1) - (1)].interm.type));
        TFunction *function = new TFunction(&tempString, type, op);
        (yyval.interm.function) = function;
    ;}
    break;

  case 28:

    {
        if (context->reservedErrorCheck((yylsp[(1) - (1)]), *(yyvsp[(1) - (1)].lex).string))
            context->recover();
        TType type(EbtVoid, EbpUndefined);
        TFunction *function = new TFunction((yyvsp[(1) - (1)].lex).string, type);
        (yyval.interm.function) = function;
    ;}
    break;

  case 29:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 30:

    {
        if (context->lValueErrorCheck((yylsp[(1) - (2)]), "++", (yyvsp[(2) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPreIncrement, (yyvsp[(2) - (2)].interm.intermTypedNode), (yylsp[(1) - (2)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(1) - (2)]), "++", (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
        }
    ;}
    break;

  case 31:

    {
        if (context->lValueErrorCheck((yylsp[(1) - (2)]), "--", (yyvsp[(2) - (2)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath(EOpPreDecrement, (yyvsp[(2) - (2)].interm.intermTypedNode), (yylsp[(1) - (2)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->unaryOpError((yylsp[(1) - (2)]), "--", (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
        }
    ;}
    break;

  case 32:

    {
        if ((yyvsp[(1) - (2)].interm).op != EOpNull) {
            (yyval.interm.intermTypedNode) = context->intermediate.addUnaryMath((yyvsp[(1) - (2)].interm).op, (yyvsp[(2) - (2)].interm.intermTypedNode), (yylsp[(1) - (2)]), context->symbolTable);
            if ((yyval.interm.intermTypedNode) == 0) {
                const char* errorOp = "";
                switch((yyvsp[(1) - (2)].interm).op) {
                case EOpNegative:   errorOp = "-"; break;
                case EOpLogicalNot: errorOp = "!"; break;
                default: break;
                }
                context->unaryOpError((yylsp[(1) - (2)]), errorOp, (yyvsp[(2) - (2)].interm.intermTypedNode)->getCompleteString());
                context->recover();
                (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
            }
        } else
            (yyval.interm.intermTypedNode) = (yyvsp[(2) - (2)].interm.intermTypedNode);
    ;}
    break;

  case 33:

    { (yyval.interm).op = EOpNull; ;}
    break;

  case 34:

    { (yyval.interm).op = EOpNegative; ;}
    break;

  case 35:

    { (yyval.interm).op = EOpLogicalNot; ;}
    break;

  case 36:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 37:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpMul, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "*", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    ;}
    break;

  case 38:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpDiv, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "/", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    ;}
    break;

  case 39:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 40:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpAdd, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "+", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    ;}
    break;

  case 41:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpSub, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "-", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    ;}
    break;

  case 42:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 43:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 44:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLessThan, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "<", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 45:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpGreaterThan, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), ">", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 46:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLessThanEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "<=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 47:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpGreaterThanEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), ">=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 48:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 49:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "==", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 50:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpNotEqual, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "!=", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 51:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 52:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 53:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 54:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 55:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLogicalAnd, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "&&", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 56:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 57:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLogicalXor, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "^^", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 58:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 59:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addBinaryMath(EOpLogicalOr, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]), context->symbolTable);
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), "||", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            ConstantUnion *unionArray = new ConstantUnion[1];
            unionArray->setBConst(false);
            (yyval.interm.intermTypedNode) = context->intermediate.addConstantUnion(unionArray, TType(EbtBool, EbpUndefined, EvqConst), (yylsp[(2) - (3)]));
        }
    ;}
    break;

  case 60:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 61:

    {
       if (context->boolErrorCheck((yylsp[(2) - (5)]), (yyvsp[(1) - (5)].interm.intermTypedNode)))
            context->recover();

        (yyval.interm.intermTypedNode) = context->intermediate.addSelection((yyvsp[(1) - (5)].interm.intermTypedNode), (yyvsp[(3) - (5)].interm.intermTypedNode), (yyvsp[(5) - (5)].interm.intermTypedNode), (yylsp[(2) - (5)]));
        if ((yyvsp[(3) - (5)].interm.intermTypedNode)->getType() != (yyvsp[(5) - (5)].interm.intermTypedNode)->getType())
            (yyval.interm.intermTypedNode) = 0;

        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (5)]), ":", (yyvsp[(3) - (5)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(5) - (5)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(5) - (5)].interm.intermTypedNode);
        }
    ;}
    break;

  case 62:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 63:

    {
        if (context->lValueErrorCheck((yylsp[(2) - (3)]), "assign", (yyvsp[(1) - (3)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = context->intermediate.addAssign((yyvsp[(2) - (3)].interm).op, (yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->assignError((yylsp[(2) - (3)]), "assign", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(1) - (3)].interm.intermTypedNode);
        }
    ;}
    break;

  case 64:

    { (yyval.interm).op = EOpAssign; ;}
    break;

  case 65:

    { (yyval.interm).op = EOpMulAssign; ;}
    break;

  case 66:

    { (yyval.interm).op = EOpDivAssign; ;}
    break;

  case 67:

    { (yyval.interm).op = EOpAddAssign; ;}
    break;

  case 68:

    { (yyval.interm).op = EOpSubAssign; ;}
    break;

  case 69:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 70:

    {
        (yyval.interm.intermTypedNode) = context->intermediate.addComma((yyvsp[(1) - (3)].interm.intermTypedNode), (yyvsp[(3) - (3)].interm.intermTypedNode), (yylsp[(2) - (3)]));
        if ((yyval.interm.intermTypedNode) == 0) {
            context->binaryOpError((yylsp[(2) - (3)]), ",", (yyvsp[(1) - (3)].interm.intermTypedNode)->getCompleteString(), (yyvsp[(3) - (3)].interm.intermTypedNode)->getCompleteString());
            context->recover();
            (yyval.interm.intermTypedNode) = (yyvsp[(3) - (3)].interm.intermTypedNode);
        }
    ;}
    break;

  case 71:

    {
        if (context->constErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 72:

    {
        TFunction &function = *((yyvsp[(1) - (2)].interm).function);
        
        TIntermAggregate *prototype = new TIntermAggregate;
        prototype->setType(function.getReturnType());
        prototype->setName(function.getName());
        
        for (size_t i = 0; i < function.getParamCount(); i++)
        {
            const TParameter &param = function.getParam(i);
            if (param.name != 0)
            {
                TVariable variable(param.name, *param.type);
                
                prototype = context->intermediate.growAggregate(prototype, context->intermediate.addSymbol(variable.getUniqueId(), variable.getName(), variable.getType(), (yylsp[(1) - (2)])), (yylsp[(1) - (2)]));
            }
            else
            {
                prototype = context->intermediate.growAggregate(prototype, context->intermediate.addSymbol(0, "", *param.type, (yylsp[(1) - (2)])), (yylsp[(1) - (2)]));
            }
        }
        
        prototype->setOp(EOpPrototype);
        (yyval.interm.intermNode) = prototype;

        context->symbolTable.pop();
    ;}
    break;

  case 73:

    {
        if ((yyvsp[(1) - (2)].interm).intermAggregate)
            (yyvsp[(1) - (2)].interm).intermAggregate->setOp(EOpDeclaration);
        (yyval.interm.intermNode) = (yyvsp[(1) - (2)].interm).intermAggregate;
    ;}
    break;

  case 74:

    {
        if (((yyvsp[(2) - (4)].interm.precision) == EbpHigh) && (context->shaderType == SH_FRAGMENT_SHADER) && !context->fragmentPrecisionHigh) {
            context->error((yylsp[(1) - (4)]), "precision is not supported in fragment shader", "highp");
            context->recover();
        }
        if (!context->symbolTable.setDefaultPrecision( (yyvsp[(3) - (4)].interm.type), (yyvsp[(2) - (4)].interm.precision) )) {
            context->error((yylsp[(1) - (4)]), "illegal type argument for default precision qualifier", getBasicString((yyvsp[(3) - (4)].interm.type).type));
            context->recover();
        }
        (yyval.interm.intermNode) = 0;
    ;}
    break;

  case 75:

    {
        //
        // Multiple declarations of the same function are allowed.
        //
        // If this is a definition, the definition production code will check for redefinitions
        // (we don't know at this point if it's a definition or not).
        //
        // Redeclarations are allowed.  But, return types and parameter qualifiers must match.
        //
        TFunction* prevDec = static_cast<TFunction*>(context->symbolTable.find((yyvsp[(1) - (2)].interm.function)->getMangledName()));
        if (prevDec) {
            if (prevDec->getReturnType() != (yyvsp[(1) - (2)].interm.function)->getReturnType()) {
                context->error((yylsp[(2) - (2)]), "overloaded functions must have the same return type", (yyvsp[(1) - (2)].interm.function)->getReturnType().getBasicString());
                context->recover();
            }
            for (size_t i = 0; i < prevDec->getParamCount(); ++i) {
                if (prevDec->getParam(i).type->getQualifier() != (yyvsp[(1) - (2)].interm.function)->getParam(i).type->getQualifier()) {
                    context->error((yylsp[(2) - (2)]), "overloaded functions must have the same parameter qualifiers", (yyvsp[(1) - (2)].interm.function)->getParam(i).type->getQualifierString());
                    context->recover();
                }
            }
        }

        //
        // Check for previously declared variables using the same name.
        //
        TSymbol *prevSym = context->symbolTable.find((yyvsp[(1) - (2)].interm.function)->getName());
        if (prevSym)
        {
            if (!prevSym->isFunction())
            {
                context->error((yylsp[(2) - (2)]), "redefinition", (yyvsp[(1) - (2)].interm.function)->getName().c_str(), "function");
                context->recover();
            }
        }
        else
        {
            // Insert the unmangled name to detect potential future redefinition as a variable.
            context->symbolTable.getOuterLevel()->insert((yyvsp[(1) - (2)].interm.function)->getName(), *(yyvsp[(1) - (2)].interm.function));
        }

        //
        // If this is a redeclaration, it could also be a definition,
        // in which case, we want to use the variable names from this one, and not the one that's
        // being redeclared.  So, pass back up this declaration, not the one in the symbol table.
        //
        (yyval.interm).function = (yyvsp[(1) - (2)].interm.function);

        // We're at the inner scope level of the function's arguments and body statement.
        // Add the function prototype to the surrounding scope instead.
        context->symbolTable.getOuterLevel()->insert(*(yyval.interm).function);
    ;}
    break;

  case 76:

    {
        (yyval.interm.function) = (yyvsp[(1) - (1)].interm.function);
    ;}
    break;

  case 77:

    {
        (yyval.interm.function) = (yyvsp[(1) - (1)].interm.function);
    ;}
    break;

  case 78:

    {
        // Add the parameter
        (yyval.interm.function) = (yyvsp[(1) - (2)].interm.function);
        if ((yyvsp[(2) - (2)].interm).param.type->getBasicType() != EbtVoid)
            (yyvsp[(1) - (2)].interm.function)->addParameter((yyvsp[(2) - (2)].interm).param);
        else
            delete (yyvsp[(2) - (2)].interm).param.type;
    ;}
    break;

  case 79:

    {
        //
        // Only first parameter of one-parameter functions can be void
        // The check for named parameters not being void is done in parameter_declarator
        //
        if ((yyvsp[(3) - (3)].interm).param.type->getBasicType() == EbtVoid) {
            //
            // This parameter > first is void
            //
            context->error((yylsp[(2) - (3)]), "cannot be an argument type except for '(void)'", "void");
            context->recover();
            delete (yyvsp[(3) - (3)].interm).param.type;
        } else {
            // Add the parameter
            (yyval.interm.function) = (yyvsp[(1) - (3)].interm.function);
            (yyvsp[(1) - (3)].interm.function)->addParameter((yyvsp[(3) - (3)].interm).param);
        }
    ;}
    break;

  case 80:

    {
        if ((yyvsp[(1) - (3)].interm.type).qualifier != EvqGlobal && (yyvsp[(1) - (3)].interm.type).qualifier != EvqTemporary) {
            context->error((yylsp[(2) - (3)]), "no qualifiers allowed for function return", getQualifierString((yyvsp[(1) - (3)].interm.type).qualifier));
            context->recover();
        }
        // make sure a sampler is not involved as well...
        if (context->structQualifierErrorCheck((yylsp[(2) - (3)]), (yyvsp[(1) - (3)].interm.type)))
            context->recover();

        // Add the function as a prototype after parsing it (we do not support recursion)
        TFunction *function;
        TType type((yyvsp[(1) - (3)].interm.type));
        function = new TFunction((yyvsp[(2) - (3)].lex).string, type);
        (yyval.interm.function) = function;
        
        context->symbolTable.push();
    ;}
    break;

  case 81:

    {
        if ((yyvsp[(1) - (2)].interm.type).type == EbtVoid) {
            context->error((yylsp[(2) - (2)]), "illegal use of type 'void'", (yyvsp[(2) - (2)].lex).string->c_str());
            context->recover();
        }
        if (context->reservedErrorCheck((yylsp[(2) - (2)]), *(yyvsp[(2) - (2)].lex).string))
            context->recover();
        TParameter param = {(yyvsp[(2) - (2)].lex).string, new TType((yyvsp[(1) - (2)].interm.type))};
        (yyval.interm).param = param;
    ;}
    break;

  case 82:

    {
        // Check that we can make an array out of this type
        if (context->arrayTypeErrorCheck((yylsp[(3) - (5)]), (yyvsp[(1) - (5)].interm.type)))
            context->recover();

        if (context->reservedErrorCheck((yylsp[(2) - (5)]), *(yyvsp[(2) - (5)].lex).string))
            context->recover();

        int size;
        if (context->arraySizeErrorCheck((yylsp[(3) - (5)]), (yyvsp[(4) - (5)].interm.intermTypedNode), size))
            context->recover();
        (yyvsp[(1) - (5)].interm.type).setArray(true, size);

        TType* type = new TType((yyvsp[(1) - (5)].interm.type));
        TParameter param = { (yyvsp[(2) - (5)].lex).string, type };
        (yyval.interm).param = param;
    ;}
    break;

  case 83:

    {
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
        if (context->paramErrorCheck((yylsp[(3) - (3)]), (yyvsp[(1) - (3)].interm.type).qualifier, (yyvsp[(2) - (3)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    ;}
    break;

  case 84:

    {
        (yyval.interm) = (yyvsp[(2) - (2)].interm);
        if (context->parameterSamplerErrorCheck((yylsp[(2) - (2)]), (yyvsp[(1) - (2)].interm.qualifier), *(yyvsp[(2) - (2)].interm).param.type))
            context->recover();
        if (context->paramErrorCheck((yylsp[(2) - (2)]), EvqTemporary, (yyvsp[(1) - (2)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    ;}
    break;

  case 85:

    {
        (yyval.interm) = (yyvsp[(3) - (3)].interm);
        if (context->paramErrorCheck((yylsp[(3) - (3)]), (yyvsp[(1) - (3)].interm.type).qualifier, (yyvsp[(2) - (3)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    ;}
    break;

  case 86:

    {
        (yyval.interm) = (yyvsp[(2) - (2)].interm);
        if (context->parameterSamplerErrorCheck((yylsp[(2) - (2)]), (yyvsp[(1) - (2)].interm.qualifier), *(yyvsp[(2) - (2)].interm).param.type))
            context->recover();
        if (context->paramErrorCheck((yylsp[(2) - (2)]), EvqTemporary, (yyvsp[(1) - (2)].interm.qualifier), (yyval.interm).param.type))
            context->recover();
    ;}
    break;

  case 87:

    {
        (yyval.interm.qualifier) = EvqIn;
    ;}
    break;

  case 88:

    {
        (yyval.interm.qualifier) = EvqIn;
    ;}
    break;

  case 89:

    {
        (yyval.interm.qualifier) = EvqOut;
    ;}
    break;

  case 90:

    {
        (yyval.interm.qualifier) = EvqInOut;
    ;}
    break;

  case 91:

    {
        TParameter param = { 0, new TType((yyvsp[(1) - (1)].interm.type)) };
        (yyval.interm).param = param;
    ;}
    break;

  case 92:

    {
        (yyval.interm) = (yyvsp[(1) - (1)].interm);
    ;}
    break;

  case 93:

    {
        if ((yyvsp[(1) - (3)].interm).type.type == EbtInvariant && !(yyvsp[(3) - (3)].lex).symbol)
        {
            context->error((yylsp[(3) - (3)]), "undeclared identifier declared as invariant", (yyvsp[(3) - (3)].lex).string->c_str());
            context->recover();
        }

        TIntermSymbol* symbol = context->intermediate.addSymbol(0, *(yyvsp[(3) - (3)].lex).string, TType((yyvsp[(1) - (3)].interm).type), (yylsp[(3) - (3)]));
        (yyval.interm).intermAggregate = context->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermNode, symbol, (yylsp[(3) - (3)]));
        
        if (context->structQualifierErrorCheck((yylsp[(3) - (3)]), (yyval.interm).type))
            context->recover();

        if (context->nonInitConstErrorCheck((yylsp[(3) - (3)]), *(yyvsp[(3) - (3)].lex).string, (yyval.interm).type, false))
            context->recover();

        TVariable* variable = 0;
        if (context->nonInitErrorCheck((yylsp[(3) - (3)]), *(yyvsp[(3) - (3)].lex).string, (yyval.interm).type, variable))
            context->recover();
        if (symbol && variable)
            symbol->setId(variable->getUniqueId());
    ;}
    break;

  case 94:

    {
        if (context->structQualifierErrorCheck((yylsp[(3) - (5)]), (yyvsp[(1) - (5)].interm).type))
            context->recover();

        if (context->nonInitConstErrorCheck((yylsp[(3) - (5)]), *(yyvsp[(3) - (5)].lex).string, (yyvsp[(1) - (5)].interm).type, true))
            context->recover();

        (yyval.interm) = (yyvsp[(1) - (5)].interm);

        if (context->arrayTypeErrorCheck((yylsp[(4) - (5)]), (yyvsp[(1) - (5)].interm).type) || context->arrayQualifierErrorCheck((yylsp[(4) - (5)]), (yyvsp[(1) - (5)].interm).type))
            context->recover();
        else {
            (yyvsp[(1) - (5)].interm).type.setArray(true);
            TVariable* variable;
            if (context->arrayErrorCheck((yylsp[(4) - (5)]), *(yyvsp[(3) - (5)].lex).string, (yyvsp[(1) - (5)].interm).type, variable))
                context->recover();
        }
    ;}
    break;

  case 95:

    {
        if (context->structQualifierErrorCheck((yylsp[(3) - (6)]), (yyvsp[(1) - (6)].interm).type))
            context->recover();

        if (context->nonInitConstErrorCheck((yylsp[(3) - (6)]), *(yyvsp[(3) - (6)].lex).string, (yyvsp[(1) - (6)].interm).type, true))
            context->recover();

        (yyval.interm) = (yyvsp[(1) - (6)].interm);

        if (context->arrayTypeErrorCheck((yylsp[(4) - (6)]), (yyvsp[(1) - (6)].interm).type) || context->arrayQualifierErrorCheck((yylsp[(4) - (6)]), (yyvsp[(1) - (6)].interm).type))
            context->recover();
        else {
            int size;
            if (context->arraySizeErrorCheck((yylsp[(4) - (6)]), (yyvsp[(5) - (6)].interm.intermTypedNode), size))
                context->recover();
            (yyvsp[(1) - (6)].interm).type.setArray(true, size);
            TVariable* variable = 0;
            if (context->arrayErrorCheck((yylsp[(4) - (6)]), *(yyvsp[(3) - (6)].lex).string, (yyvsp[(1) - (6)].interm).type, variable))
                context->recover();
            TType type = TType((yyvsp[(1) - (6)].interm).type);
            type.setArraySize(size);
            (yyval.interm).intermAggregate = context->intermediate.growAggregate((yyvsp[(1) - (6)].interm).intermNode, context->intermediate.addSymbol(variable ? variable->getUniqueId() : 0, *(yyvsp[(3) - (6)].lex).string, type, (yylsp[(3) - (6)])), (yylsp[(3) - (6)]));
        }
    ;}
    break;

  case 96:

    {
        if (context->structQualifierErrorCheck((yylsp[(3) - (5)]), (yyvsp[(1) - (5)].interm).type))
            context->recover();

        (yyval.interm) = (yyvsp[(1) - (5)].interm);

        TIntermNode* intermNode;
        if (!context->executeInitializer((yylsp[(3) - (5)]), *(yyvsp[(3) - (5)].lex).string, (yyvsp[(1) - (5)].interm).type, (yyvsp[(5) - (5)].interm.intermTypedNode), intermNode)) {
            //
            // build the intermediate representation
            //
            if (intermNode)
        (yyval.interm).intermAggregate = context->intermediate.growAggregate((yyvsp[(1) - (5)].interm).intermNode, intermNode, (yylsp[(4) - (5)]));
            else
                (yyval.interm).intermAggregate = (yyvsp[(1) - (5)].interm).intermAggregate;
        } else {
            context->recover();
            (yyval.interm).intermAggregate = 0;
        }
    ;}
    break;

  case 97:

    {
        (yyval.interm).type = (yyvsp[(1) - (1)].interm.type);
        (yyval.interm).intermAggregate = context->intermediate.makeAggregate(context->intermediate.addSymbol(0, "", TType((yyvsp[(1) - (1)].interm.type)), (yylsp[(1) - (1)])), (yylsp[(1) - (1)]));
    ;}
    break;

  case 98:

    {
        TIntermSymbol* symbol = context->intermediate.addSymbol(0, *(yyvsp[(2) - (2)].lex).string, TType((yyvsp[(1) - (2)].interm.type)), (yylsp[(2) - (2)]));
        (yyval.interm).intermAggregate = context->intermediate.makeAggregate(symbol, (yylsp[(2) - (2)]));
        
        if (context->structQualifierErrorCheck((yylsp[(2) - (2)]), (yyval.interm).type))
            context->recover();

        if (context->nonInitConstErrorCheck((yylsp[(2) - (2)]), *(yyvsp[(2) - (2)].lex).string, (yyval.interm).type, false))
            context->recover();
            
            (yyval.interm).type = (yyvsp[(1) - (2)].interm.type);

        TVariable* variable = 0;
        if (context->nonInitErrorCheck((yylsp[(2) - (2)]), *(yyvsp[(2) - (2)].lex).string, (yyval.interm).type, variable))
            context->recover();
        if (variable && symbol)
            symbol->setId(variable->getUniqueId());
    ;}
    break;

  case 99:

    {
        context->error((yylsp[(2) - (4)]), "unsized array declarations not supported", (yyvsp[(2) - (4)].lex).string->c_str());
        context->recover();

        TIntermSymbol* symbol = context->intermediate.addSymbol(0, *(yyvsp[(2) - (4)].lex).string, TType((yyvsp[(1) - (4)].interm.type)), (yylsp[(2) - (4)]));
        (yyval.interm).intermAggregate = context->intermediate.makeAggregate(symbol, (yylsp[(2) - (4)]));
        (yyval.interm).type = (yyvsp[(1) - (4)].interm.type);
    ;}
    break;

  case 100:

    {
        TType type = TType((yyvsp[(1) - (5)].interm.type));
        int size;
        if (context->arraySizeErrorCheck((yylsp[(2) - (5)]), (yyvsp[(4) - (5)].interm.intermTypedNode), size))
            context->recover();
        type.setArraySize(size);
        TIntermSymbol* symbol = context->intermediate.addSymbol(0, *(yyvsp[(2) - (5)].lex).string, type, (yylsp[(2) - (5)]));
        (yyval.interm).intermAggregate = context->intermediate.makeAggregate(symbol, (yylsp[(2) - (5)]));
        
        if (context->structQualifierErrorCheck((yylsp[(2) - (5)]), (yyvsp[(1) - (5)].interm.type)))
            context->recover();

        if (context->nonInitConstErrorCheck((yylsp[(2) - (5)]), *(yyvsp[(2) - (5)].lex).string, (yyvsp[(1) - (5)].interm.type), true))
            context->recover();

        (yyval.interm).type = (yyvsp[(1) - (5)].interm.type);

        if (context->arrayTypeErrorCheck((yylsp[(3) - (5)]), (yyvsp[(1) - (5)].interm.type)) || context->arrayQualifierErrorCheck((yylsp[(3) - (5)]), (yyvsp[(1) - (5)].interm.type)))
            context->recover();
        else {
            int size;
            if (context->arraySizeErrorCheck((yylsp[(3) - (5)]), (yyvsp[(4) - (5)].interm.intermTypedNode), size))
                context->recover();

            (yyvsp[(1) - (5)].interm.type).setArray(true, size);
            TVariable* variable = 0;
            if (context->arrayErrorCheck((yylsp[(3) - (5)]), *(yyvsp[(2) - (5)].lex).string, (yyvsp[(1) - (5)].interm.type), variable))
                context->recover();
            if (variable && symbol)
                symbol->setId(variable->getUniqueId());
        }
    ;}
    break;

  case 101:

    {
        if (context->structQualifierErrorCheck((yylsp[(2) - (4)]), (yyvsp[(1) - (4)].interm.type)))
            context->recover();

        (yyval.interm).type = (yyvsp[(1) - (4)].interm.type);

        TIntermNode* intermNode;
        if (!context->executeInitializer((yylsp[(2) - (4)]), *(yyvsp[(2) - (4)].lex).string, (yyvsp[(1) - (4)].interm.type), (yyvsp[(4) - (4)].interm.intermTypedNode), intermNode)) {
        //
        // Build intermediate representation
        //
            if(intermNode)
                (yyval.interm).intermAggregate = context->intermediate.makeAggregate(intermNode, (yylsp[(3) - (4)]));
            else
                (yyval.interm).intermAggregate = 0;
        } else {
            context->recover();
            (yyval.interm).intermAggregate = 0;
        }
    ;}
    break;

  case 102:

    {
        VERTEX_ONLY("invariant declaration", (yylsp[(1) - (2)]));
        if (context->globalErrorCheck((yylsp[(1) - (2)]), context->symbolTable.atGlobalLevel(), "invariant varying"))
            context->recover();
        (yyval.interm).type.setBasic(EbtInvariant, EvqInvariantVaryingOut, (yylsp[(2) - (2)]));
        if (!(yyvsp[(2) - (2)].lex).symbol)
        {
            context->error((yylsp[(2) - (2)]), "undeclared identifier declared as invariant", (yyvsp[(2) - (2)].lex).string->c_str());
            context->recover();
            
            (yyval.interm).intermAggregate = 0;
        }
        else
        {
            TIntermSymbol *symbol = context->intermediate.addSymbol(0, *(yyvsp[(2) - (2)].lex).string, TType((yyval.interm).type), (yylsp[(2) - (2)]));
            (yyval.interm).intermAggregate = context->intermediate.makeAggregate(symbol, (yylsp[(2) - (2)]));
        }
    ;}
    break;

  case 103:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);

        if ((yyvsp[(1) - (1)].interm.type).array) {
            context->error((yylsp[(1) - (1)]), "not supported", "first-class array");
            context->recover();
            (yyvsp[(1) - (1)].interm.type).setArray(false);
        }
    ;}
    break;

  case 104:

    {
        if ((yyvsp[(2) - (2)].interm.type).array) {
            context->error((yylsp[(2) - (2)]), "not supported", "first-class array");
            context->recover();
            (yyvsp[(2) - (2)].interm.type).setArray(false);
        }

        if ((yyvsp[(1) - (2)].interm.type).qualifier == EvqAttribute &&
            ((yyvsp[(2) - (2)].interm.type).type == EbtBool || (yyvsp[(2) - (2)].interm.type).type == EbtInt)) {
            context->error((yylsp[(2) - (2)]), "cannot be bool or int", getQualifierString((yyvsp[(1) - (2)].interm.type).qualifier));
            context->recover();
        }
        if (((yyvsp[(1) - (2)].interm.type).qualifier == EvqVaryingIn || (yyvsp[(1) - (2)].interm.type).qualifier == EvqVaryingOut) &&
            ((yyvsp[(2) - (2)].interm.type).type == EbtBool || (yyvsp[(2) - (2)].interm.type).type == EbtInt)) {
            context->error((yylsp[(2) - (2)]), "cannot be bool or int", getQualifierString((yyvsp[(1) - (2)].interm.type).qualifier));
            context->recover();
        }
        (yyval.interm.type) = (yyvsp[(2) - (2)].interm.type);
        (yyval.interm.type).qualifier = (yyvsp[(1) - (2)].interm.type).qualifier;
    ;}
    break;

  case 105:

    {
        (yyval.interm.type).setBasic(EbtVoid, EvqConst, (yylsp[(1) - (1)]));
    ;}
    break;

  case 106:

    {
        VERTEX_ONLY("attribute", (yylsp[(1) - (1)]));
        if (context->globalErrorCheck((yylsp[(1) - (1)]), context->symbolTable.atGlobalLevel(), "attribute"))
            context->recover();
        (yyval.interm.type).setBasic(EbtVoid, EvqAttribute, (yylsp[(1) - (1)]));
    ;}
    break;

  case 107:

    {
        if (context->globalErrorCheck((yylsp[(1) - (1)]), context->symbolTable.atGlobalLevel(), "varying"))
            context->recover();
        if (context->shaderType == SH_VERTEX_SHADER)
            (yyval.interm.type).setBasic(EbtVoid, EvqVaryingOut, (yylsp[(1) - (1)]));
        else
            (yyval.interm.type).setBasic(EbtVoid, EvqVaryingIn, (yylsp[(1) - (1)]));
    ;}
    break;

  case 108:

    {
        if (context->globalErrorCheck((yylsp[(1) - (2)]), context->symbolTable.atGlobalLevel(), "invariant varying"))
            context->recover();
        if (context->shaderType == SH_VERTEX_SHADER)
            (yyval.interm.type).setBasic(EbtVoid, EvqInvariantVaryingOut, (yylsp[(1) - (2)]));
        else
            (yyval.interm.type).setBasic(EbtVoid, EvqInvariantVaryingIn, (yylsp[(1) - (2)]));
    ;}
    break;

  case 109:

    {
        if (context->globalErrorCheck((yylsp[(1) - (1)]), context->symbolTable.atGlobalLevel(), "uniform"))
            context->recover();
        (yyval.interm.type).setBasic(EbtVoid, EvqUniform, (yylsp[(1) - (1)]));
    ;}
    break;

  case 110:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);

        if ((yyval.interm.type).precision == EbpUndefined) {
            (yyval.interm.type).precision = context->symbolTable.getDefaultPrecision((yyvsp[(1) - (1)].interm.type).type);
            if (context->precisionErrorCheck((yylsp[(1) - (1)]), (yyval.interm.type).precision, (yyvsp[(1) - (1)].interm.type).type)) {
                context->recover();
            }
        }
    ;}
    break;

  case 111:

    {
        (yyval.interm.type) = (yyvsp[(2) - (2)].interm.type);
        (yyval.interm.type).precision = (yyvsp[(1) - (2)].interm.precision);
    ;}
    break;

  case 112:

    {
        (yyval.interm.precision) = EbpHigh;
    ;}
    break;

  case 113:

    {
        (yyval.interm.precision) = EbpMedium;
    ;}
    break;

  case 114:

    {
        (yyval.interm.precision) = EbpLow;
    ;}
    break;

  case 115:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);
    ;}
    break;

  case 116:

    {
        (yyval.interm.type) = (yyvsp[(1) - (4)].interm.type);

        if (context->arrayTypeErrorCheck((yylsp[(2) - (4)]), (yyvsp[(1) - (4)].interm.type)))
            context->recover();
        else {
            int size;
            if (context->arraySizeErrorCheck((yylsp[(2) - (4)]), (yyvsp[(3) - (4)].interm.intermTypedNode), size))
                context->recover();
            (yyval.interm.type).setArray(true, size);
        }
    ;}
    break;

  case 117:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtVoid, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 118:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 119:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 120:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 121:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2);
    ;}
    break;

  case 122:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3);
    ;}
    break;

  case 123:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4);
    ;}
    break;

  case 124:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2);
    ;}
    break;

  case 125:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3);
    ;}
    break;

  case 126:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtBool, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4);
    ;}
    break;

  case 127:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2);
    ;}
    break;

  case 128:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3);
    ;}
    break;

  case 129:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtInt, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4);
    ;}
    break;

  case 130:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(2, true);
    ;}
    break;

  case 131:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(3, true);
    ;}
    break;

  case 132:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtFloat, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).setAggregate(4, true);
    ;}
    break;

  case 133:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2D, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 134:

    {
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSamplerCube, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 135:

    {
        if (!context->supportsExtension("GL_OES_EGL_image_external")) {
            context->error((yylsp[(1) - (1)]), "unsupported type", "samplerExternalOES");
            context->recover();
        }
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSamplerExternalOES, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 136:

    {
        if (!context->supportsExtension("GL_ARB_texture_rectangle")) {
            context->error((yylsp[(1) - (1)]), "unsupported type", "sampler2DRect");
            context->recover();
        }
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtSampler2DRect, qual, (yylsp[(1) - (1)]));
    ;}
    break;

  case 137:

    {
        (yyval.interm.type) = (yyvsp[(1) - (1)].interm.type);
        (yyval.interm.type).qualifier = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
    ;}
    break;

  case 138:

    {
        //
        // This is for user defined type names.  The lexical phase looked up the
        // type.
        //
        TType& structure = static_cast<TVariable*>((yyvsp[(1) - (1)].lex).symbol)->getType();
        TQualifier qual = context->symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        (yyval.interm.type).setBasic(EbtStruct, qual, (yylsp[(1) - (1)]));
        (yyval.interm.type).userDef = &structure;
    ;}
    break;

  case 139:

    { if (context->enterStructDeclaration((yylsp[(2) - (3)]), *(yyvsp[(2) - (3)].lex).string)) context->recover(); ;}
    break;

  case 140:

    {
        if (context->reservedErrorCheck((yylsp[(2) - (6)]), *(yyvsp[(2) - (6)].lex).string))
            context->recover();

        TType* structure = new TType(new TStructure((yyvsp[(2) - (6)].lex).string, (yyvsp[(5) - (6)].interm.fieldList)));
        TVariable* userTypeDef = new TVariable((yyvsp[(2) - (6)].lex).string, *structure, true);
        if (! context->symbolTable.insert(*userTypeDef)) {
            context->error((yylsp[(2) - (6)]), "redefinition", (yyvsp[(2) - (6)].lex).string->c_str(), "struct");
            context->recover();
        }
        (yyval.interm.type).setBasic(EbtStruct, EvqTemporary, (yylsp[(1) - (6)]));
        (yyval.interm.type).userDef = structure;
        context->exitStructDeclaration();
    ;}
    break;

  case 141:

    { if (context->enterStructDeclaration((yylsp[(2) - (2)]), *(yyvsp[(2) - (2)].lex).string)) context->recover(); ;}
    break;

  case 142:

    {
        TType* structure = new TType(new TStructure(NewPoolTString(""), (yyvsp[(4) - (5)].interm.fieldList)));
        (yyval.interm.type).setBasic(EbtStruct, EvqTemporary, (yylsp[(1) - (5)]));
        (yyval.interm.type).userDef = structure;
        context->exitStructDeclaration();
    ;}
    break;

  case 143:

    {
        (yyval.interm.fieldList) = (yyvsp[(1) - (1)].interm.fieldList);
    ;}
    break;

  case 144:

    {
        (yyval.interm.fieldList) = (yyvsp[(1) - (2)].interm.fieldList);
        for (size_t i = 0; i < (yyvsp[(2) - (2)].interm.fieldList)->size(); ++i) {
            TField* field = (*(yyvsp[(2) - (2)].interm.fieldList))[i];
            for (size_t j = 0; j < (yyval.interm.fieldList)->size(); ++j) {
                if ((*(yyval.interm.fieldList))[j]->name() == field->name()) {
                    context->error((yylsp[(2) - (2)]), "duplicate field name in structure:", "struct", field->name().c_str());
                    context->recover();
                }
            }
            (yyval.interm.fieldList)->push_back(field);
        }
    ;}
    break;

  case 145:

    {
        (yyval.interm.fieldList) = (yyvsp[(2) - (3)].interm.fieldList);

        if (context->voidErrorCheck((yylsp[(1) - (3)]), (*(yyvsp[(2) - (3)].interm.fieldList))[0]->name(), (yyvsp[(1) - (3)].interm.type))) {
            context->recover();
        }
        for (unsigned int i = 0; i < (yyval.interm.fieldList)->size(); ++i) {
            //
            // Careful not to replace already known aspects of type, like array-ness
            //
            TType* type = (*(yyval.interm.fieldList))[i]->type();
            type->setBasicType((yyvsp[(1) - (3)].interm.type).type);
            type->setNominalSize((yyvsp[(1) - (3)].interm.type).size);
            type->setMatrix((yyvsp[(1) - (3)].interm.type).matrix);
            type->setPrecision((yyvsp[(1) - (3)].interm.type).precision);

            // don't allow arrays of arrays
            if (type->isArray()) {
                if (context->arrayTypeErrorCheck((yylsp[(1) - (3)]), (yyvsp[(1) - (3)].interm.type)))
                    context->recover();
            }
            if ((yyvsp[(1) - (3)].interm.type).array)
                type->setArraySize((yyvsp[(1) - (3)].interm.type).arraySize);
            if ((yyvsp[(1) - (3)].interm.type).userDef)
                type->setStruct((yyvsp[(1) - (3)].interm.type).userDef->getStruct());

            if (context->structNestingErrorCheck((yylsp[(1) - (3)]), *(*(yyval.interm.fieldList))[i]))
                context->recover();
        }
    ;}
    break;

  case 146:

    {
        (yyval.interm.fieldList) = NewPoolTFieldList();
        (yyval.interm.fieldList)->push_back((yyvsp[(1) - (1)].interm.field));
    ;}
    break;

  case 147:

    {
        (yyval.interm.fieldList)->push_back((yyvsp[(3) - (3)].interm.field));
    ;}
    break;

  case 148:

    {
        if (context->reservedErrorCheck((yylsp[(1) - (1)]), *(yyvsp[(1) - (1)].lex).string))
            context->recover();

        TType* type = new TType(EbtVoid, EbpUndefined);
        (yyval.interm.field) = new TField(type, (yyvsp[(1) - (1)].lex).string);
    ;}
    break;

  case 149:

    {
        if (context->reservedErrorCheck((yylsp[(1) - (4)]), *(yyvsp[(1) - (4)].lex).string))
            context->recover();

        TType* type = new TType(EbtVoid, EbpUndefined);
        int size = 0;
        if (context->arraySizeErrorCheck((yylsp[(3) - (4)]), (yyvsp[(3) - (4)].interm.intermTypedNode), size))
            context->recover();
        type->setArraySize(size);

        (yyval.interm.field) = new TField(type, (yyvsp[(1) - (4)].lex).string);
    ;}
    break;

  case 150:

    { (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode); ;}
    break;

  case 151:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 152:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermAggregate); ;}
    break;

  case 153:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 154:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 155:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 156:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 157:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 158:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 159:

    { (yyval.interm.intermAggregate) = 0; ;}
    break;

  case 160:

    { context->symbolTable.push(); ;}
    break;

  case 161:

    { context->symbolTable.pop(); ;}
    break;

  case 162:

    {
        if ((yyvsp[(3) - (5)].interm.intermAggregate) != 0) {
            (yyvsp[(3) - (5)].interm.intermAggregate)->setOp(EOpSequence);
            (yyvsp[(3) - (5)].interm.intermAggregate)->setLine((yyloc));
        }
        (yyval.interm.intermAggregate) = (yyvsp[(3) - (5)].interm.intermAggregate);
    ;}
    break;

  case 163:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 164:

    { (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode); ;}
    break;

  case 165:

    { context->symbolTable.push(); ;}
    break;

  case 166:

    { context->symbolTable.pop(); (yyval.interm.intermNode) = (yyvsp[(2) - (2)].interm.intermNode); ;}
    break;

  case 167:

    { context->symbolTable.push(); ;}
    break;

  case 168:

    { context->symbolTable.pop(); (yyval.interm.intermNode) = (yyvsp[(2) - (2)].interm.intermNode); ;}
    break;

  case 169:

    {
        (yyval.interm.intermNode) = 0;
    ;}
    break;

  case 170:

    {
        if ((yyvsp[(2) - (3)].interm.intermAggregate)) {
            (yyvsp[(2) - (3)].interm.intermAggregate)->setOp(EOpSequence);
            (yyvsp[(2) - (3)].interm.intermAggregate)->setLine((yyloc));
        }
        (yyval.interm.intermNode) = (yyvsp[(2) - (3)].interm.intermAggregate);
    ;}
    break;

  case 171:

    {
        (yyval.interm.intermAggregate) = context->intermediate.makeAggregate((yyvsp[(1) - (1)].interm.intermNode), (yyloc));
    ;}
    break;

  case 172:

    {
        (yyval.interm.intermAggregate) = context->intermediate.growAggregate((yyvsp[(1) - (2)].interm.intermAggregate), (yyvsp[(2) - (2)].interm.intermNode), (yyloc));
    ;}
    break;

  case 173:

    { (yyval.interm.intermNode) = 0; ;}
    break;

  case 174:

    { (yyval.interm.intermNode) = static_cast<TIntermNode*>((yyvsp[(1) - (2)].interm.intermTypedNode)); ;}
    break;

  case 175:

    {
        if (context->boolErrorCheck((yylsp[(1) - (5)]), (yyvsp[(3) - (5)].interm.intermTypedNode)))
            context->recover();
        (yyval.interm.intermNode) = context->intermediate.addSelection((yyvsp[(3) - (5)].interm.intermTypedNode), (yyvsp[(5) - (5)].interm.nodePair), (yylsp[(1) - (5)]));
    ;}
    break;

  case 176:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (3)].interm.intermNode);
        (yyval.interm.nodePair).node2 = (yyvsp[(3) - (3)].interm.intermNode);
    ;}
    break;

  case 177:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (1)].interm.intermNode);
        (yyval.interm.nodePair).node2 = 0;
    ;}
    break;

  case 178:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
        if (context->boolErrorCheck((yyvsp[(1) - (1)].interm.intermTypedNode)->getLine(), (yyvsp[(1) - (1)].interm.intermTypedNode)))
            context->recover();
    ;}
    break;

  case 179:

    {
        TIntermNode* intermNode;
        if (context->structQualifierErrorCheck((yylsp[(2) - (4)]), (yyvsp[(1) - (4)].interm.type)))
            context->recover();
        if (context->boolErrorCheck((yylsp[(2) - (4)]), (yyvsp[(1) - (4)].interm.type)))
            context->recover();

        if (!context->executeInitializer((yylsp[(2) - (4)]), *(yyvsp[(2) - (4)].lex).string, (yyvsp[(1) - (4)].interm.type), (yyvsp[(4) - (4)].interm.intermTypedNode), intermNode))
            (yyval.interm.intermTypedNode) = (yyvsp[(4) - (4)].interm.intermTypedNode);
        else {
            context->recover();
            (yyval.interm.intermTypedNode) = 0;
        }
    ;}
    break;

  case 180:

    { context->symbolTable.push(); ++context->loopNestingLevel; ;}
    break;

  case 181:

    {
        context->symbolTable.pop();
        (yyval.interm.intermNode) = context->intermediate.addLoop(ELoopWhile, 0, (yyvsp[(4) - (6)].interm.intermTypedNode), 0, (yyvsp[(6) - (6)].interm.intermNode), (yylsp[(1) - (6)]));
        --context->loopNestingLevel;
    ;}
    break;

  case 182:

    { ++context->loopNestingLevel; ;}
    break;

  case 183:

    {
        if (context->boolErrorCheck((yylsp[(8) - (8)]), (yyvsp[(6) - (8)].interm.intermTypedNode)))
            context->recover();

        (yyval.interm.intermNode) = context->intermediate.addLoop(ELoopDoWhile, 0, (yyvsp[(6) - (8)].interm.intermTypedNode), 0, (yyvsp[(3) - (8)].interm.intermNode), (yylsp[(4) - (8)]));
        --context->loopNestingLevel;
    ;}
    break;

  case 184:

    { context->symbolTable.push(); ++context->loopNestingLevel; ;}
    break;

  case 185:

    {
        context->symbolTable.pop();
        (yyval.interm.intermNode) = context->intermediate.addLoop(ELoopFor, (yyvsp[(4) - (7)].interm.intermNode), reinterpret_cast<TIntermTyped*>((yyvsp[(5) - (7)].interm.nodePair).node1), reinterpret_cast<TIntermTyped*>((yyvsp[(5) - (7)].interm.nodePair).node2), (yyvsp[(7) - (7)].interm.intermNode), (yylsp[(1) - (7)]));
        --context->loopNestingLevel;
    ;}
    break;

  case 186:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    ;}
    break;

  case 187:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    ;}
    break;

  case 188:

    {
        (yyval.interm.intermTypedNode) = (yyvsp[(1) - (1)].interm.intermTypedNode);
    ;}
    break;

  case 189:

    {
        (yyval.interm.intermTypedNode) = 0;
    ;}
    break;

  case 190:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (2)].interm.intermTypedNode);
        (yyval.interm.nodePair).node2 = 0;
    ;}
    break;

  case 191:

    {
        (yyval.interm.nodePair).node1 = (yyvsp[(1) - (3)].interm.intermTypedNode);
        (yyval.interm.nodePair).node2 = (yyvsp[(3) - (3)].interm.intermTypedNode);
    ;}
    break;

  case 192:

    {
        if (context->loopNestingLevel <= 0) {
            context->error((yylsp[(1) - (2)]), "continue statement only allowed in loops", "");
            context->recover();
        }
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpContinue, (yylsp[(1) - (2)]));
    ;}
    break;

  case 193:

    {
        if (context->loopNestingLevel <= 0) {
            context->error((yylsp[(1) - (2)]), "break statement only allowed in loops", "");
            context->recover();
        }
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpBreak, (yylsp[(1) - (2)]));
    ;}
    break;

  case 194:

    {
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpReturn, (yylsp[(1) - (2)]));
        if (context->currentFunctionType->getBasicType() != EbtVoid) {
            context->error((yylsp[(1) - (2)]), "non-void function must return a value", "return");
            context->recover();
        }
    ;}
    break;

  case 195:

    {
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpReturn, (yyvsp[(2) - (3)].interm.intermTypedNode), (yylsp[(1) - (3)]));
        context->functionReturnsValue = true;
        if (context->currentFunctionType->getBasicType() == EbtVoid) {
            context->error((yylsp[(1) - (3)]), "void function cannot return a value", "return");
            context->recover();
        } else if (*(context->currentFunctionType) != (yyvsp[(2) - (3)].interm.intermTypedNode)->getType()) {
            context->error((yylsp[(1) - (3)]), "function return is not matching type:", "return");
            context->recover();
        }
    ;}
    break;

  case 196:

    {
        FRAG_ONLY("discard", (yylsp[(1) - (2)]));
        (yyval.interm.intermNode) = context->intermediate.addBranch(EOpKill, (yylsp[(1) - (2)]));
    ;}
    break;

  case 197:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
        context->treeRoot = (yyval.interm.intermNode);
    ;}
    break;

  case 198:

    {
        (yyval.interm.intermNode) = context->intermediate.growAggregate((yyvsp[(1) - (2)].interm.intermNode), (yyvsp[(2) - (2)].interm.intermNode), (yyloc));
        context->treeRoot = (yyval.interm.intermNode);
    ;}
    break;

  case 199:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    ;}
    break;

  case 200:

    {
        (yyval.interm.intermNode) = (yyvsp[(1) - (1)].interm.intermNode);
    ;}
    break;

  case 201:

    {
        TFunction* function = (yyvsp[(1) - (1)].interm).function;
        
        const TSymbol *builtIn = context->symbolTable.findBuiltIn(function->getMangledName());
        
        if (builtIn)
        {
            context->error((yylsp[(1) - (1)]), "built-in functions cannot be redefined", function->getName().c_str());
            context->recover();
        }
        
        TFunction* prevDec = static_cast<TFunction*>(context->symbolTable.find(function->getMangledName()));
        //
        // Note:  'prevDec' could be 'function' if this is the first time we've seen function
        // as it would have just been put in the symbol table.  Otherwise, we're looking up
        // an earlier occurance.
        //
        if (prevDec->isDefined()) {
            //
            // Then this function already has a body.
            //
            context->error((yylsp[(1) - (1)]), "function already has a body", function->getName().c_str());
            context->recover();
        }
        prevDec->setDefined();

        //
        // Raise error message if main function takes any parameters or return anything other than void
        //
        if (function->getName() == "main") {
            if (function->getParamCount() > 0) {
                context->error((yylsp[(1) - (1)]), "function cannot take any parameter(s)", function->getName().c_str());
                context->recover();
            }
            if (function->getReturnType().getBasicType() != EbtVoid) {
                context->error((yylsp[(1) - (1)]), "", function->getReturnType().getBasicString(), "main function cannot return a value");
                context->recover();
            }
        }

        //
        // Remember the return type for later checking for RETURN statements.
        //
        context->currentFunctionType = &(prevDec->getReturnType());
        context->functionReturnsValue = false;

        //
        // Insert parameters into the symbol table.
        // If the parameter has no name, it's not an error, just don't insert it
        // (could be used for unused args).
        //
        // Also, accumulate the list of parameters into the HIL, so lower level code
        // knows where to find parameters.
        //
        TIntermAggregate* paramNodes = new TIntermAggregate;
        for (size_t i = 0; i < function->getParamCount(); i++) {
            const TParameter& param = function->getParam(i);
            if (param.name != 0) {
                TVariable *variable = new TVariable(param.name, *param.type);
                //
                // Insert the parameters with name in the symbol table.
                //
                if (! context->symbolTable.insert(*variable)) {
                    context->error((yylsp[(1) - (1)]), "redefinition", variable->getName().c_str());
                    context->recover();
                    delete variable;
                }

                //
                // Add the parameter to the HIL
                //
                paramNodes = context->intermediate.growAggregate(
                                               paramNodes,
                                               context->intermediate.addSymbol(variable->getUniqueId(),
                                                                               variable->getName(),
                                                                               variable->getType(),
                                                                               (yylsp[(1) - (1)])),
                                               (yylsp[(1) - (1)]));
            } else {
                paramNodes = context->intermediate.growAggregate(paramNodes, context->intermediate.addSymbol(0, "", *param.type, (yylsp[(1) - (1)])), (yylsp[(1) - (1)]));
            }
        }
        context->intermediate.setAggregateOperator(paramNodes, EOpParameters, (yylsp[(1) - (1)]));
        (yyvsp[(1) - (1)].interm).intermAggregate = paramNodes;
        context->loopNestingLevel = 0;
    ;}
    break;

  case 202:

    {
        //?? Check that all paths return a value if return type != void ?
        //   May be best done as post process phase on intermediate code
        if (context->currentFunctionType->getBasicType() != EbtVoid && ! context->functionReturnsValue) {
            context->error((yylsp[(1) - (3)]), "function does not return a value:", "", (yyvsp[(1) - (3)].interm).function->getName().c_str());
            context->recover();
        }
        
        (yyval.interm.intermNode) = context->intermediate.growAggregate((yyvsp[(1) - (3)].interm).intermAggregate, (yyvsp[(3) - (3)].interm.intermNode), (yyloc));
        context->intermediate.setAggregateOperator((yyval.interm.intermNode), EOpFunction, (yylsp[(1) - (3)]));
        (yyval.interm.intermNode)->getAsAggregate()->setName((yyvsp[(1) - (3)].interm).function->getMangledName().c_str());
        (yyval.interm.intermNode)->getAsAggregate()->setType((yyvsp[(1) - (3)].interm).function->getReturnType());

        // store the pragma information for debug and optimize and other vendor specific
        // information. This information can be queried from the parse tree
        (yyval.interm.intermNode)->getAsAggregate()->setOptimize(context->pragma().optimize);
        (yyval.interm.intermNode)->getAsAggregate()->setDebug(context->pragma().debug);

        context->symbolTable.pop();
    ;}
    break;


/* Line 1267 of yacc.c.  */

      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, context, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, context, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, context, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, context);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, context);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, context, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, context);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, context);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}





void yyerror(YYLTYPE* yylloc, TParseContext* context, const char* reason) {
    context->error(*yylloc, reason, "");
    context->recover();
}

int glslang_parse(TParseContext* context) {
    return yyparse(context);
}

