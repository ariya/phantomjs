/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         _xkbcommon_parse
#define yylex           _xkbcommon_lex
#define yyerror         _xkbcommon_error
#define yylval          _xkbcommon_lval
#define yychar          _xkbcommon_char
#define yydebug         _xkbcommon_debug
#define yynerrs         _xkbcommon_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 33 "parser.y"

#include "xkbcomp-priv.h"
#include "ast-build.h"
#include "parser-priv.h"

struct parser_param {
    struct xkb_context *ctx;
    struct scanner *scanner;
    XkbFile *rtrn;
    bool more_maps;
};

#define parser_err(param, fmt, ...) \
    scanner_err((param)->scanner, fmt, ##__VA_ARGS__)

#define parser_warn(param, fmt, ...) \
    scanner_warn((param)->scanner, fmt, ##__VA_ARGS__)

static void
_xkbcommon_error(struct parser_param *param, const char *msg)
{
    parser_err(param, "%s", msg);
}

static bool
resolve_keysym(const char *str, xkb_keysym_t *sym_rtrn)
{
    xkb_keysym_t sym;

    if (!str || istreq(str, "any") || istreq(str, "nosymbol")) {
        *sym_rtrn = XKB_KEY_NoSymbol;
        return true;
    }

    if (istreq(str, "none") || istreq(str, "voidsymbol")) {
        *sym_rtrn = XKB_KEY_VoidSymbol;
        return true;
    }

    sym = xkb_keysym_from_name(str, XKB_KEYSYM_NO_FLAGS);
    if (sym != XKB_KEY_NoSymbol) {
        *sym_rtrn = sym;
        return true;
    }

    return false;
}

#define param_scanner param->scanner


/* Line 268 of yacc.c  */
#line 131 "src/xkbcomp/parser.c"

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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     END_OF_FILE = 0,
     ERROR_TOK = 255,
     XKB_KEYMAP = 1,
     XKB_KEYCODES = 2,
     XKB_TYPES = 3,
     XKB_SYMBOLS = 4,
     XKB_COMPATMAP = 5,
     XKB_GEOMETRY = 6,
     XKB_SEMANTICS = 7,
     XKB_LAYOUT = 8,
     INCLUDE = 10,
     OVERRIDE = 11,
     AUGMENT = 12,
     REPLACE = 13,
     ALTERNATE = 14,
     VIRTUAL_MODS = 20,
     TYPE = 21,
     INTERPRET = 22,
     ACTION_TOK = 23,
     KEY = 24,
     ALIAS = 25,
     GROUP = 26,
     MODIFIER_MAP = 27,
     INDICATOR = 28,
     SHAPE = 29,
     KEYS = 30,
     ROW = 31,
     SECTION = 32,
     OVERLAY = 33,
     TEXT = 34,
     OUTLINE = 35,
     SOLID = 36,
     LOGO = 37,
     VIRTUAL = 38,
     EQUALS = 40,
     PLUS = 41,
     MINUS = 42,
     DIVIDE = 43,
     TIMES = 44,
     OBRACE = 45,
     CBRACE = 46,
     OPAREN = 47,
     CPAREN = 48,
     OBRACKET = 49,
     CBRACKET = 50,
     DOT = 51,
     COMMA = 52,
     SEMI = 53,
     EXCLAM = 54,
     INVERT = 55,
     STRING = 60,
     INTEGER = 61,
     FLOAT = 62,
     IDENT = 63,
     KEYNAME = 64,
     PARTIAL = 70,
     DEFAULT = 71,
     HIDDEN = 72,
     ALPHANUMERIC_KEYS = 73,
     MODIFIER_KEYS = 74,
     KEYPAD_KEYS = 75,
     FUNCTION_KEYS = 76,
     ALTERNATE_GROUP = 77
   };
#endif
/* Tokens.  */
#define END_OF_FILE 0
#define ERROR_TOK 255
#define XKB_KEYMAP 1
#define XKB_KEYCODES 2
#define XKB_TYPES 3
#define XKB_SYMBOLS 4
#define XKB_COMPATMAP 5
#define XKB_GEOMETRY 6
#define XKB_SEMANTICS 7
#define XKB_LAYOUT 8
#define INCLUDE 10
#define OVERRIDE 11
#define AUGMENT 12
#define REPLACE 13
#define ALTERNATE 14
#define VIRTUAL_MODS 20
#define TYPE 21
#define INTERPRET 22
#define ACTION_TOK 23
#define KEY 24
#define ALIAS 25
#define GROUP 26
#define MODIFIER_MAP 27
#define INDICATOR 28
#define SHAPE 29
#define KEYS 30
#define ROW 31
#define SECTION 32
#define OVERLAY 33
#define TEXT 34
#define OUTLINE 35
#define SOLID 36
#define LOGO 37
#define VIRTUAL 38
#define EQUALS 40
#define PLUS 41
#define MINUS 42
#define DIVIDE 43
#define TIMES 44
#define OBRACE 45
#define CBRACE 46
#define OPAREN 47
#define CPAREN 48
#define OBRACKET 49
#define CBRACKET 50
#define DOT 51
#define COMMA 52
#define SEMI 53
#define EXCLAM 54
#define INVERT 55
#define STRING 60
#define INTEGER 61
#define FLOAT 62
#define IDENT 63
#define KEYNAME 64
#define PARTIAL 70
#define DEFAULT 71
#define HIDDEN 72
#define ALPHANUMERIC_KEYS 73
#define MODIFIER_KEYS 74
#define KEYPAD_KEYS 75
#define FUNCTION_KEYS 76
#define ALTERNATE_GROUP 77




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 161 "parser.y"

        int              ival;
        int64_t          num;
        enum xkb_file_type file_type;
        char            *str;
        xkb_atom_t      sval;
        enum merge_mode merge;
        enum xkb_map_flags mapFlags;
        xkb_keysym_t    keysym;
        ParseCommon     *any;
        ExprDef         *expr;
        VarDef          *var;
        VModDef         *vmod;
        InterpDef       *interp;
        KeyTypeDef      *keyType;
        SymbolsDef      *syms;
        ModMapDef       *modMask;
        GroupCompatDef  *groupCompat;
        LedMapDef       *ledMap;
        LedNameDef      *ledName;
        KeycodeDef      *keyCode;
        KeyAliasDef     *keyAlias;
        void            *geom;
        XkbFile         *file;



/* Line 293 of yacc.c  */
#line 325 "src/xkbcomp/parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 337 "src/xkbcomp/parser.c"

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
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
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
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  16
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   735

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  72
/* YYNRULES -- Number of rules.  */
#define YYNRULES  184
/* YYNRULES -- Number of states.  */
#define YYNSTATES  334

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   257

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     4,     5,     6,     7,     8,     9,    10,    11,     2,
      12,    13,    14,    15,    16,     2,     2,     2,     2,     2,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,     2,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,     2,     2,     2,     2,
      52,    53,    54,    55,    56,     2,     2,     2,     2,     2,
      57,    58,    59,    60,    61,    62,    63,    64,     2,     2,
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
       2,     2,     2,     2,     2,     3,     1,     2
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    17,    19,    21,    23,
      26,    28,    36,    38,    40,    42,    44,    46,    48,    49,
      52,    54,    56,    58,    60,    62,    64,    66,    68,    70,
      73,    74,    77,    80,    83,    86,    89,    92,    95,    98,
     101,   104,   107,   110,   113,   116,   119,   124,   127,   131,
     136,   142,   146,   150,   152,   154,   158,   165,   169,   171,
     174,   176,   183,   190,   194,   196,   197,   201,   205,   207,
     210,   212,   216,   220,   226,   233,   240,   246,   253,   260,
     267,   274,   277,   279,   285,   287,   289,   291,   293,   296,
     298,   304,   306,   310,   312,   314,   318,   325,   329,   331,
     335,   339,   341,   345,   351,   355,   359,   361,   367,   374,
     376,   378,   380,   382,   384,   386,   388,   390,   392,   394,
     396,   398,   400,   402,   404,   406,   408,   410,   411,   413,
     415,   417,   419,   421,   423,   424,   428,   430,   434,   438,
     442,   446,   450,   452,   455,   458,   461,   464,   466,   471,
     473,   477,   481,   483,   488,   490,   494,   499,   506,   508,
     510,   512,   514,   516,   517,   521,   525,   527,   529,   533,
     535,   537,   539,   542,   544,   546,   548,   550,   552,   554,
     556,   558,   560,   562,   563
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      66,     0,    -1,    67,    -1,    70,    -1,     0,    -1,    72,
      68,   135,    41,    69,    42,    49,    -1,     4,    -1,    10,
      -1,    11,    -1,    69,    70,    -1,    70,    -1,    72,    71,
     135,    41,    75,    42,    49,    -1,     5,    -1,     6,    -1,
       8,    -1,     7,    -1,     9,    -1,    73,    -1,    -1,    73,
      74,    -1,    74,    -1,    57,    -1,    58,    -1,    59,    -1,
      60,    -1,    61,    -1,    62,    -1,    63,    -1,    64,    -1,
      75,    76,    -1,    -1,   114,    77,    -1,   114,    80,    -1,
     114,    83,    -1,   114,    78,    -1,   114,    79,    -1,   114,
      86,    -1,   114,    87,    -1,   114,    92,    -1,   114,    91,
      -1,   114,    93,    -1,   114,    94,    -1,   114,    95,    -1,
     114,    96,    -1,   114,   110,    -1,   115,    52,    -1,   122,
      36,   118,    49,    -1,   133,    49,    -1,    50,   133,    49,
      -1,    56,    36,   132,    49,    -1,    22,    56,    36,    56,
      49,    -1,    17,    81,    49,    -1,    81,    48,    82,    -1,
      82,    -1,   133,    -1,   133,    36,   118,    -1,    19,    84,
      41,    85,    42,    49,    -1,   127,    37,   118,    -1,   127,
      -1,    85,    77,    -1,    77,    -1,    18,   134,    41,    85,
      42,    49,    -1,    21,    56,    41,    88,    42,    49,    -1,
      88,    48,    89,    -1,    89,    -1,    -1,   122,    36,   118,
      -1,   122,    36,    90,    -1,   133,    -1,    50,   133,    -1,
      90,    -1,    45,   124,    46,    -1,    45,   120,    46,    -1,
      23,   131,    36,   118,    49,    -1,    24,   133,    41,   117,
      42,    49,    -1,    25,   134,    41,    85,    42,    49,    -1,
      25,   131,    36,   118,    49,    -1,    35,    25,   131,    36,
     118,    49,    -1,    26,   134,    41,   106,    42,    49,    -1,
      26,   134,    41,   108,    42,    49,    -1,    29,   134,    41,
      97,    42,    49,    -1,    97,    98,    -1,    98,    -1,    28,
      41,    99,    42,    49,    -1,    77,    -1,   110,    -1,    93,
      -1,   103,    -1,    99,   100,    -1,   100,    -1,    27,    41,
     101,    42,    49,    -1,    77,    -1,   101,    48,   102,    -1,
     102,    -1,    56,    -1,    41,   117,    42,    -1,    30,   134,
      41,   104,    42,    49,    -1,   104,    48,   105,    -1,   105,
      -1,    56,    36,    56,    -1,   106,    48,   107,    -1,   107,
      -1,    41,   108,    42,    -1,   133,    36,    41,   108,    42,
      -1,   133,    36,   118,    -1,   108,    48,   109,    -1,   109,
      -1,    45,   128,    48,   128,    46,    -1,   111,   134,    41,
      85,    42,    49,    -1,    31,    -1,    32,    -1,    33,    -1,
      34,    -1,   133,    -1,   113,    -1,    20,    -1,    19,    -1,
      18,    -1,    21,    -1,    23,    -1,    24,    -1,    25,    -1,
      26,    -1,    28,    -1,    29,    -1,    31,    -1,   115,    -1,
      -1,    12,    -1,    14,    -1,    13,    -1,    15,    -1,    16,
      -1,   117,    -1,    -1,   117,    48,   118,    -1,   118,    -1,
     118,    39,   118,    -1,   118,    37,   118,    -1,   118,    38,
     118,    -1,   118,    40,   118,    -1,   122,    36,   118,    -1,
     119,    -1,    38,   119,    -1,    37,   119,    -1,    50,   119,
      -1,    51,   119,    -1,   122,    -1,   112,    43,   116,    44,
      -1,   123,    -1,    43,   118,    44,    -1,   120,    48,   121,
      -1,   121,    -1,   112,    43,   116,    44,    -1,   112,    -1,
     112,    47,   112,    -1,   112,    45,   118,    46,    -1,   112,
      47,   112,    45,   118,    46,    -1,   134,    -1,   131,    -1,
     130,    -1,    56,    -1,   125,    -1,    -1,   125,    48,   127,
      -1,   125,    48,   126,    -1,   127,    -1,   126,    -1,    41,
     125,    42,    -1,    55,    -1,    29,    -1,   131,    -1,    38,
     129,    -1,   129,    -1,    54,    -1,    53,    -1,    54,    -1,
      53,    -1,    53,    -1,    55,    -1,    58,    -1,    52,    -1,
     136,    -1,    -1,    52,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   232,   232,   234,   236,   240,   246,   247,   248,   251,
     259,   263,   278,   279,   280,   281,   282,   285,   286,   289,
     290,   293,   294,   295,   296,   297,   298,   299,   300,   303,
     305,   308,   313,   318,   323,   328,   333,   338,   343,   348,
     353,   358,   363,   364,   365,   366,   373,   375,   377,   381,
     385,   389,   393,   396,   400,   402,   406,   412,   414,   418,
     421,   425,   431,   437,   440,   442,   445,   446,   447,   448,
     449,   452,   454,   458,   462,   466,   470,   472,   476,   478,
     482,   486,   487,   490,   492,   494,   496,   498,   502,   503,
     506,   507,   511,   512,   515,   517,   521,   525,   526,   529,
     532,   534,   538,   540,   542,   546,   548,   552,   556,   560,
     561,   562,   563,   566,   567,   570,   572,   574,   576,   578,
     580,   582,   584,   586,   588,   590,   594,   595,   598,   599,
     600,   601,   602,   612,   613,   616,   619,   623,   625,   627,
     629,   631,   633,   637,   639,   641,   643,   645,   647,   649,
     651,   655,   658,   662,   666,   668,   670,   672,   676,   678,
     680,   682,   686,   687,   690,   692,   694,   696,   700,   704,
     710,   711,   731,   732,   735,   736,   739,   742,   745,   748,
     749,   752,   755,   756,   759
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "END_OF_FILE", "error", "$undefined", "ERROR_TOK", "XKB_KEYMAP",
  "XKB_KEYCODES", "XKB_TYPES", "XKB_SYMBOLS", "XKB_COMPATMAP",
  "XKB_GEOMETRY", "XKB_SEMANTICS", "XKB_LAYOUT", "INCLUDE", "OVERRIDE",
  "AUGMENT", "REPLACE", "ALTERNATE", "VIRTUAL_MODS", "TYPE", "INTERPRET",
  "ACTION_TOK", "KEY", "ALIAS", "GROUP", "MODIFIER_MAP", "INDICATOR",
  "SHAPE", "KEYS", "ROW", "SECTION", "OVERLAY", "TEXT", "OUTLINE", "SOLID",
  "LOGO", "VIRTUAL", "EQUALS", "PLUS", "MINUS", "DIVIDE", "TIMES",
  "OBRACE", "CBRACE", "OPAREN", "CPAREN", "OBRACKET", "CBRACKET", "DOT",
  "COMMA", "SEMI", "EXCLAM", "INVERT", "STRING", "INTEGER", "FLOAT",
  "IDENT", "KEYNAME", "PARTIAL", "DEFAULT", "HIDDEN", "ALPHANUMERIC_KEYS",
  "MODIFIER_KEYS", "KEYPAD_KEYS", "FUNCTION_KEYS", "ALTERNATE_GROUP",
  "$accept", "XkbFile", "XkbCompositeMap", "XkbCompositeType",
  "XkbMapConfigList", "XkbMapConfig", "FileType", "OptFlags", "Flags",
  "Flag", "DeclList", "Decl", "VarDecl", "KeyNameDecl", "KeyAliasDecl",
  "VModDecl", "VModDefList", "VModDef", "InterpretDecl", "InterpretMatch",
  "VarDeclList", "KeyTypeDecl", "SymbolsDecl", "SymbolsBody",
  "SymbolsVarDecl", "ArrayInit", "GroupCompatDecl", "ModMapDecl",
  "LedMapDecl", "LedNameDecl", "ShapeDecl", "SectionDecl", "SectionBody",
  "SectionBodyItem", "RowBody", "RowBodyItem", "Keys", "Key",
  "OverlayDecl", "OverlayKeyList", "OverlayKey", "OutlineList",
  "OutlineInList", "CoordList", "Coord", "DoodadDecl", "DoodadType",
  "FieldSpec", "Element", "OptMergeMode", "MergeMode", "OptExprList",
  "ExprList", "Expr", "Term", "ActionList", "Action", "Lhs", "Terminal",
  "OptKeySymList", "KeySymList", "KeySyms", "KeySym", "SignedNumber",
  "Number", "Float", "Integer", "KeyCode", "Ident", "String", "OptMapName",
  "MapName", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   255,     1,     2,     3,     4,     5,     6,
       7,     8,    10,    11,    12,    13,    14,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    60,    61,    62,    63,    64,    70,    71,    72,
      73,    74,    75,    76,    77
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    65,    66,    66,    66,    67,    68,    68,    68,    69,
      69,    70,    71,    71,    71,    71,    71,    72,    72,    73,
      73,    74,    74,    74,    74,    74,    74,    74,    74,    75,
      75,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    77,    77,    77,    78,
      79,    80,    81,    81,    82,    82,    83,    84,    84,    85,
      85,    86,    87,    88,    88,    88,    89,    89,    89,    89,
      89,    90,    90,    91,    92,    93,    94,    94,    95,    95,
      96,    97,    97,    98,    98,    98,    98,    98,    99,    99,
     100,   100,   101,   101,   102,   102,   103,   104,   104,   105,
     106,   106,   107,   107,   107,   108,   108,   109,   110,   111,
     111,   111,   111,   112,   112,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   114,   114,   115,   115,
     115,   115,   115,   116,   116,   117,   117,   118,   118,   118,
     118,   118,   118,   119,   119,   119,   119,   119,   119,   119,
     119,   120,   120,   121,   122,   122,   122,   122,   123,   123,
     123,   123,   124,   124,   125,   125,   125,   125,   126,   127,
     127,   127,   128,   128,   129,   129,   130,   131,   132,   133,
     133,   134,   135,   135,   136
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     7,     1,     1,     1,     2,
       1,     7,     1,     1,     1,     1,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     4,     2,     3,     4,
       5,     3,     3,     1,     1,     3,     6,     3,     1,     2,
       1,     6,     6,     3,     1,     0,     3,     3,     1,     2,
       1,     3,     3,     5,     6,     6,     5,     6,     6,     6,
       6,     2,     1,     5,     1,     1,     1,     1,     2,     1,
       5,     1,     3,     1,     1,     3,     6,     3,     1,     3,
       3,     1,     3,     5,     3,     3,     1,     5,     6,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     0,     3,     1,     3,     3,     3,
       3,     3,     1,     2,     2,     2,     2,     1,     4,     1,
       3,     3,     1,     4,     1,     3,     4,     6,     1,     1,
       1,     1,     1,     0,     3,     3,     1,     1,     3,     1,
       1,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      18,     4,    21,    22,    23,    24,    25,    26,    27,    28,
       0,     2,     3,     0,    17,    20,     1,     6,    12,    13,
      15,    14,    16,     7,     8,   183,   183,    19,   184,     0,
     182,     0,    18,    30,    18,    10,     0,   127,     0,     9,
     128,   130,   129,   131,   132,     0,    29,     0,   126,     5,
      11,     0,   117,   116,   115,   118,     0,   119,   120,   121,
     122,   123,   124,   125,   110,   111,   112,     0,     0,   179,
       0,   180,    31,    34,    35,    32,    33,    36,    37,    39,
      38,    40,    41,    42,    43,    44,     0,   154,   114,     0,
     113,    45,     0,    53,    54,   181,     0,   170,   177,   169,
       0,    58,   171,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    47,     0,
      51,     0,     0,     0,     0,    65,     0,     0,     0,     0,
       0,     0,     0,     0,    48,   178,     0,     0,   117,   116,
     118,   119,   120,   121,   122,   124,   125,     0,     0,     0,
       0,     0,   176,   161,   154,     0,   142,   147,   149,   160,
     159,   113,   158,   155,     0,    52,    55,    60,     0,     0,
      57,   163,     0,     0,    64,    70,     0,   113,     0,     0,
       0,   136,     0,     0,     0,     0,     0,   101,     0,   106,
       0,   121,   123,     0,    84,    86,     0,    82,    87,    85,
       0,    49,     0,   144,   147,   143,     0,   145,   146,   134,
       0,     0,     0,     0,   156,     0,     0,    46,     0,    59,
       0,   170,     0,   169,     0,     0,   152,     0,   162,   167,
     166,    69,     0,     0,     0,    50,    73,     0,     0,    76,
       0,     0,     0,   175,   174,     0,   173,     0,     0,     0,
       0,     0,     0,     0,     0,    81,     0,     0,   150,     0,
     133,   138,   139,   137,   140,   141,     0,    61,    56,     0,
     134,    72,     0,    71,     0,    62,    63,    67,    66,    74,
     135,    75,   102,   172,     0,    78,   100,    79,   105,     0,
     104,     0,    91,     0,    89,     0,    80,    77,   108,   148,
     157,   168,     0,   151,   165,   164,     0,     0,     0,     0,
      88,     0,     0,    98,   153,   107,   103,     0,    94,     0,
      93,    83,     0,     0,     0,     0,     0,     0,    99,    96,
      97,    95,    90,    92
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    10,    11,    25,    34,    12,    26,    36,    14,    15,
      37,    46,   167,    73,    74,    75,    92,    93,    76,   100,
     168,    77,    78,   173,   174,   175,    79,    80,   195,    82,
      83,    84,   196,   197,   293,   294,   319,   320,   198,   312,
     313,   186,   187,   188,   189,   199,    86,   154,    88,    47,
      48,   259,   260,   181,   156,   225,   226,   157,   158,   227,
     228,   229,   230,   245,   246,   159,   160,   136,   161,   162,
      29,    30
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -182
static const yytype_int16 yypact[] =
{
     176,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,
       6,  -182,  -182,   271,   227,  -182,  -182,  -182,  -182,  -182,
    -182,  -182,  -182,  -182,  -182,   -38,   -38,  -182,  -182,   -24,
    -182,    33,   227,  -182,   210,  -182,   353,    44,     5,  -182,
    -182,  -182,  -182,  -182,  -182,    32,  -182,    13,    41,  -182,
    -182,   -48,    55,    11,  -182,    79,    87,    58,   -48,    -2,
      55,  -182,    55,    72,  -182,  -182,  -182,   107,   -48,  -182,
     110,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,  -182,
    -182,  -182,  -182,  -182,  -182,  -182,    55,   -18,  -182,   127,
     121,  -182,    66,  -182,   138,  -182,   136,  -182,  -182,  -182,
     144,   147,  -182,   152,   180,   182,   178,   184,   187,   188,
     190,    58,   198,   201,   214,   367,   677,   367,  -182,   -48,
    -182,   367,   663,   663,   367,   494,   200,   367,   367,   367,
     663,    68,   449,   223,  -182,  -182,   212,   663,  -182,  -182,
    -182,  -182,  -182,  -182,  -182,  -182,  -182,   367,   367,   367,
     367,   367,  -182,  -182,    57,   157,  -182,   224,  -182,  -182,
    -182,  -182,  -182,   218,    91,  -182,   333,  -182,   509,   537,
     333,   552,   -48,     1,  -182,  -182,   228,    40,   216,   143,
      70,   333,   150,   593,   247,   -30,    97,  -182,   105,  -182,
     261,    55,   259,    55,  -182,  -182,   408,  -182,  -182,  -182,
     367,  -182,   608,  -182,  -182,  -182,   287,  -182,  -182,   367,
     367,   367,   367,   367,  -182,   367,   367,  -182,   252,  -182,
     253,   264,    24,   269,   272,   163,  -182,   273,   270,  -182,
    -182,  -182,   280,   494,   285,  -182,  -182,   283,   367,  -182,
     284,   112,     8,  -182,  -182,   294,  -182,   299,   -36,   304,
     247,   326,   649,   279,   307,  -182,   204,   316,  -182,   322,
     320,   111,   111,  -182,  -182,   333,   211,  -182,  -182,   116,
     367,  -182,   677,  -182,    24,  -182,  -182,  -182,   333,  -182,
     333,  -182,  -182,  -182,   -30,  -182,  -182,  -182,  -182,   247,
     333,   334,  -182,   466,  -182,   318,  -182,  -182,  -182,  -182,
    -182,  -182,   339,  -182,  -182,  -182,   343,   120,    14,   345,
    -182,   361,   124,  -182,  -182,  -182,  -182,   367,  -182,   131,
    -182,  -182,   344,   350,   318,   166,   352,    14,  -182,  -182,
    -182,  -182,  -182,  -182
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -182,  -182,  -182,  -182,  -182,   181,  -182,   402,  -182,   389,
    -182,  -182,   -35,  -182,  -182,  -182,  -182,   288,  -182,  -182,
     -50,  -182,  -182,  -182,   173,   174,  -182,  -182,   362,  -182,
    -182,  -182,  -182,   215,  -182,   119,  -182,    86,  -182,  -182,
      90,  -182,   167,  -181,   185,   369,  -182,   -27,  -182,  -182,
    -182,   154,  -126,    83,    76,  -182,   158,   -31,  -182,  -182,
     221,   170,   -52,   161,   205,  -182,   -44,  -182,   -47,   -34,
     420,  -182
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -180
static const yytype_int16 yytable[] =
{
      90,   101,   180,   241,    94,   184,    16,    69,   242,   102,
      71,   106,    72,   105,    28,   107,    89,    32,    96,    69,
      87,   112,    71,   243,   244,   108,   109,   115,   110,   116,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      97,    61,    62,   232,    63,    64,    65,    66,    67,   233,
      95,    98,   114,    97,    49,   317,    40,    41,    42,    43,
      44,   243,   244,    68,    98,   222,    99,   133,    69,    70,
     318,    71,    94,   169,    33,    90,    90,    98,   177,    99,
     183,    50,   -68,    90,   190,    90,    45,   202,   -68,   163,
      90,    89,    89,    91,   176,    87,    87,   194,    87,    89,
     209,    89,   115,    87,   116,    87,    89,    95,   307,   184,
      87,    98,   237,   185,   119,   120,   204,   204,   238,   204,
     204,    90,    90,    69,  -109,   231,    71,   102,   210,   211,
     212,   213,   111,   219,   219,   103,    90,    89,    89,   247,
     217,    87,    87,   104,   224,   248,   113,   249,   219,    90,
     212,   213,    89,   250,   282,    90,    87,   108,   301,   253,
     250,   194,   316,   117,   274,    89,   323,   219,   250,    87,
     118,    89,   324,   326,   121,    87,     1,   122,   102,   327,
     210,   211,   212,   213,   124,   123,   177,   210,   211,   212,
     213,   325,   236,   125,   210,   211,   212,   213,   155,   239,
     164,   190,   176,   214,   166,    90,    87,   170,   331,   271,
     179,   272,   182,    35,   238,    39,   126,   292,   127,   128,
     129,    89,   305,   203,   205,    87,   207,   208,   130,   131,
     102,   132,   206,     2,     3,     4,     5,     6,     7,     8,
       9,   210,   211,   212,   213,   224,    90,   134,   210,   211,
     212,   213,    38,   297,   135,   137,   178,   300,   292,   200,
     215,   201,    89,   216,   234,   235,    87,     2,     3,     4,
       5,     6,     7,     8,     9,    17,    18,    19,    20,    21,
      22,    23,    24,   256,     2,     3,     4,     5,     6,     7,
       8,     9,   185,   261,   262,   263,   264,   251,   265,   266,
     252,   267,   268,   138,   139,    54,   140,  -124,   141,   142,
     143,   144,  -179,    61,   145,   270,   146,   278,   274,   273,
     295,   280,   147,   148,   210,   211,   212,   213,   149,   275,
     171,   258,   279,   281,   290,   150,   151,    95,    98,   152,
      69,   153,   284,    71,   138,   139,    54,   140,   285,   141,
     142,   143,   144,   287,    61,   145,   296,   146,    18,    19,
      20,    21,    22,   147,   148,   298,   299,   289,   238,   149,
     210,   211,   212,   213,   311,   308,   150,   151,    95,    98,
     152,    69,   153,   314,    71,   138,   139,    54,   140,   315,
     141,   142,   143,   144,   321,    61,   145,   322,   146,   329,
     328,   332,    13,    27,   147,   148,   276,   165,   277,    81,
     149,   255,   310,   333,   330,   286,    85,   150,   151,    95,
      98,   152,    69,   153,   302,    71,   138,   139,    54,   140,
     303,   141,   142,   191,   144,   288,   192,   145,   193,    63,
      64,    65,    66,   269,   304,   306,    31,   283,     0,     0,
     254,     0,     0,     0,     0,     0,     0,     0,    68,     0,
       0,     0,     0,    69,     0,     0,    71,   138,   139,    54,
     140,     0,   141,   142,   191,   144,     0,   192,   145,   193,
      63,    64,    65,    66,   138,   139,    54,   140,     0,   141,
     142,   143,   144,   291,    61,   145,     0,   146,     0,    68,
       0,     0,     0,     0,    69,     0,     0,    71,   309,     0,
       0,     0,   138,   139,    54,   140,    68,   141,   142,   143,
     144,    69,    61,   145,    71,   146,     0,   138,   139,    54,
     140,     0,   141,   142,   143,   144,     0,    61,   145,   171,
     146,     0,     0,     0,   172,     0,     0,     0,     0,    69,
       0,   218,    71,     0,     0,   138,   139,    54,   140,    68,
     141,   142,   143,   144,    69,    61,   145,    71,   146,     0,
     138,   139,    54,   140,     0,   141,   142,   143,   144,   220,
      61,   221,     0,   146,     0,     0,     0,    68,     0,     0,
       0,     0,    69,   222,     0,    71,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    98,     0,   223,     0,     0,
      71,   138,   139,    54,   140,     0,   141,   142,   143,   144,
       0,    61,   145,     0,   146,     0,   138,   139,    54,   140,
       0,   141,   142,   143,   144,   240,    61,   145,     0,   146,
       0,     0,     0,    68,     0,     0,     0,     0,    69,     0,
     257,    71,     0,     0,     0,     0,     0,     0,    68,     0,
       0,     0,     0,    69,     0,     0,    71,   138,   139,    54,
     140,     0,   141,   142,   143,   144,   291,    61,   145,     0,
     146,   138,   139,    54,   140,     0,   141,   142,   143,   144,
       0,    61,   145,     0,   146,   138,   139,    54,   140,    68,
     141,   142,   143,   144,    69,    61,   145,    71,   146,     0,
       0,     0,     0,    68,     0,     0,     0,     0,    69,     0,
       0,    71,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    69,     0,     0,    71
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-182))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      47,    53,   128,   184,    51,    41,     0,    55,    38,    53,
      58,    58,    47,    57,    52,    59,    47,    41,    52,    55,
      47,    68,    58,    53,    54,    59,    60,    45,    62,    47,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      29,    28,    29,    42,    31,    32,    33,    34,    35,    48,
      52,    53,    86,    29,    49,    41,    12,    13,    14,    15,
      16,    53,    54,    50,    53,    41,    55,   111,    55,    56,
      56,    58,   119,   123,    41,   122,   123,    53,   125,    55,
     130,    49,    42,   130,   131,   132,    42,   137,    48,   116,
     137,   122,   123,    52,   125,   122,   123,   132,   125,   130,
      43,   132,    45,   130,    47,   132,   137,    52,   289,    41,
     137,    53,    42,    45,    48,    49,   147,   148,    48,   150,
     151,   168,   169,    55,    52,   172,    58,   171,    37,    38,
      39,    40,    25,   168,   169,    56,   183,   168,   169,    42,
      49,   168,   169,    56,   171,    48,    36,    42,   183,   196,
      39,    40,   183,    48,    42,   202,   183,   191,    42,   193,
      48,   196,    42,    36,    48,   196,    42,   202,    48,   196,
      49,   202,    48,    42,    36,   202,     0,    41,   222,    48,
      37,    38,    39,    40,    37,    41,   233,    37,    38,    39,
      40,   317,    49,    41,    37,    38,    39,    40,   115,    49,
     117,   248,   233,    46,   121,   252,   233,   124,    42,    46,
     127,    48,   129,    32,    48,    34,    36,   252,    36,    41,
      36,   252,   274,   147,   148,   252,   150,   151,    41,    41,
     274,    41,   149,    57,    58,    59,    60,    61,    62,    63,
      64,    37,    38,    39,    40,   272,   293,    49,    37,    38,
      39,    40,    42,    49,    53,    41,    56,    46,   293,    36,
      36,    49,   293,    45,    36,    49,   293,    57,    58,    59,
      60,    61,    62,    63,    64,     4,     5,     6,     7,     8,
       9,    10,    11,   200,    57,    58,    59,    60,    61,    62,
      63,    64,    45,   210,   211,   212,   213,    36,   215,   216,
      41,    49,    49,    18,    19,    20,    21,    43,    23,    24,
      25,    26,    43,    28,    29,    43,    31,   234,    48,    46,
      41,   238,    37,    38,    37,    38,    39,    40,    43,    49,
      45,    44,    49,    49,   251,    50,    51,    52,    53,    54,
      55,    56,    48,    58,    18,    19,    20,    21,    49,    23,
      24,    25,    26,    49,    28,    29,    49,    31,     5,     6,
       7,     8,     9,    37,    38,    49,    44,    41,    48,    43,
      37,    38,    39,    40,    56,    41,    50,    51,    52,    53,
      54,    55,    56,    44,    58,    18,    19,    20,    21,    46,
      23,    24,    25,    26,    49,    28,    29,    36,    31,    49,
      56,    49,     0,    14,    37,    38,   233,   119,   234,    47,
      43,   196,   293,   327,   324,   248,    47,    50,    51,    52,
      53,    54,    55,    56,   270,    58,    18,    19,    20,    21,
     272,    23,    24,    25,    26,   250,    28,    29,    30,    31,
      32,    33,    34,   222,   274,   284,    26,   242,    -1,    -1,
      42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    55,    -1,    -1,    58,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    18,    19,    20,    21,    -1,    23,
      24,    25,    26,    27,    28,    29,    -1,    31,    -1,    50,
      -1,    -1,    -1,    -1,    55,    -1,    -1,    58,    42,    -1,
      -1,    -1,    18,    19,    20,    21,    50,    23,    24,    25,
      26,    55,    28,    29,    58,    31,    -1,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    -1,    28,    29,    45,
      31,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,
      -1,    42,    58,    -1,    -1,    18,    19,    20,    21,    50,
      23,    24,    25,    26,    55,    28,    29,    58,    31,    -1,
      18,    19,    20,    21,    -1,    23,    24,    25,    26,    42,
      28,    29,    -1,    31,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    55,    41,    -1,    58,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    53,    -1,    55,    -1,    -1,
      58,    18,    19,    20,    21,    -1,    23,    24,    25,    26,
      -1,    28,    29,    -1,    31,    -1,    18,    19,    20,    21,
      -1,    23,    24,    25,    26,    42,    28,    29,    -1,    31,
      -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,    -1,
      42,    58,    -1,    -1,    -1,    -1,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    55,    -1,    -1,    58,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    27,    28,    29,    -1,
      31,    18,    19,    20,    21,    -1,    23,    24,    25,    26,
      -1,    28,    29,    -1,    31,    18,    19,    20,    21,    50,
      23,    24,    25,    26,    55,    28,    29,    58,    31,    -1,
      -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,    -1,
      -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    -1,    -1,    58
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     0,    57,    58,    59,    60,    61,    62,    63,    64,
      66,    67,    70,    72,    73,    74,     0,     4,     5,     6,
       7,     8,     9,    10,    11,    68,    71,    74,    52,   135,
     136,   135,    41,    41,    69,    70,    72,    75,    42,    70,
      12,    13,    14,    15,    16,    42,    76,   114,   115,    49,
      49,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    28,    29,    31,    32,    33,    34,    35,    50,    55,
      56,    58,    77,    78,    79,    80,    83,    86,    87,    91,
      92,    93,    94,    95,    96,   110,   111,   112,   113,   122,
     133,    52,    81,    82,   133,    52,   134,    29,    53,    55,
      84,   127,   131,    56,    56,   131,   133,   131,   134,   134,
     134,    25,   133,    36,   134,    45,    47,    36,    49,    48,
      49,    36,    41,    41,    37,    41,    36,    36,    41,    36,
      41,    41,    41,   131,    49,    53,   132,    41,    18,    19,
      21,    23,    24,    25,    26,    29,    31,    37,    38,    43,
      50,    51,    54,    56,   112,   118,   119,   122,   123,   130,
     131,   133,   134,   112,   118,    82,   118,    77,    85,    85,
     118,    45,    50,    88,    89,    90,   122,   133,    56,   118,
     117,   118,   118,    85,    41,    45,   106,   107,   108,   109,
     133,    25,    28,    30,    77,    93,    97,    98,   103,   110,
      36,    49,    85,   119,   122,   119,   118,   119,   119,    43,
      37,    38,    39,    40,    46,    36,    45,    49,    42,    77,
      42,    29,    41,    55,   112,   120,   121,   124,   125,   126,
     127,   133,    42,    48,    36,    49,    49,    42,    48,    49,
      42,   108,    38,    53,    54,   128,   129,    42,    48,    42,
      48,    36,    41,   134,    42,    98,   118,    42,    44,   116,
     117,   118,   118,   118,   118,   118,   118,    49,    49,   125,
      43,    46,    48,    46,    48,    49,    89,    90,   118,    49,
     118,    49,    42,   129,    48,    49,   107,    49,   109,    41,
     118,    27,    77,    99,   100,    41,    49,    49,    49,    44,
      46,    42,   116,   121,   126,   127,   128,   108,    41,    42,
     100,    56,   104,   105,    44,    46,    42,    41,    56,   101,
     102,    49,    36,    42,    48,   117,    42,    48,    56,    49,
     105,    42,    49,   102
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
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (param, YY_("syntax error: cannot back up")); \
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


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, param_scanner)
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
		  Type, Value, param); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct parser_param *param)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, param)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    struct parser_param *param;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (param);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct parser_param *param)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, param)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    struct parser_param *param;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, param);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
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
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, struct parser_param *param)
#else
static void
yy_reduce_print (yyvsp, yyrule, param)
    YYSTYPE *yyvsp;
    int yyrule;
    struct parser_param *param;
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
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , param);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, param); \
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, struct parser_param *param)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, param)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    struct parser_param *param;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (param);

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
int yyparse (struct parser_param *param);
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
yyparse (struct parser_param *param)
#else
int
yyparse (param)
    struct parser_param *param;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

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

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

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
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 233 "parser.y"
    { (yyval.file) = param->rtrn = (yyvsp[(1) - (1)].file); param->more_maps = true; }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 235 "parser.y"
    { (yyval.file) = param->rtrn = (yyvsp[(1) - (1)].file); param->more_maps = true; YYACCEPT; }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 237 "parser.y"
    { (yyval.file) = param->rtrn = NULL; param->more_maps = false; }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 243 "parser.y"
    { (yyval.file) = XkbFileCreate((yyvsp[(2) - (7)].file_type), (yyvsp[(3) - (7)].str), (ParseCommon *) (yyvsp[(5) - (7)].file), (yyvsp[(1) - (7)].mapFlags)); }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 246 "parser.y"
    { (yyval.file_type) = FILE_TYPE_KEYMAP; }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 247 "parser.y"
    { (yyval.file_type) = FILE_TYPE_KEYMAP; }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 248 "parser.y"
    { (yyval.file_type) = FILE_TYPE_KEYMAP; }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 252 "parser.y"
    {
                            if (!(yyvsp[(2) - (2)].file))
                                (yyval.file) = (yyvsp[(1) - (2)].file);
                            else
                                (yyval.file) = (XkbFile *) AppendStmt((ParseCommon *) (yyvsp[(1) - (2)].file),
                                                            (ParseCommon *) (yyvsp[(2) - (2)].file));
                        }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 260 "parser.y"
    { (yyval.file) = (yyvsp[(1) - (1)].file); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 266 "parser.y"
    {
                            if ((yyvsp[(2) - (7)].file_type) == FILE_TYPE_GEOMETRY) {
                                free((yyvsp[(3) - (7)].str));
                                FreeStmt((yyvsp[(5) - (7)].any));
                                (yyval.file) = NULL;
                            }
                            else {
                                (yyval.file) = XkbFileCreate((yyvsp[(2) - (7)].file_type), (yyvsp[(3) - (7)].str), (yyvsp[(5) - (7)].any), (yyvsp[(1) - (7)].mapFlags));
                            }
                        }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 278 "parser.y"
    { (yyval.file_type) = FILE_TYPE_KEYCODES; }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 279 "parser.y"
    { (yyval.file_type) = FILE_TYPE_TYPES; }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 280 "parser.y"
    { (yyval.file_type) = FILE_TYPE_COMPAT; }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 281 "parser.y"
    { (yyval.file_type) = FILE_TYPE_SYMBOLS; }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 282 "parser.y"
    { (yyval.file_type) = FILE_TYPE_GEOMETRY; }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 285 "parser.y"
    { (yyval.mapFlags) = (yyvsp[(1) - (1)].mapFlags); }
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 286 "parser.y"
    { (yyval.mapFlags) = 0; }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 289 "parser.y"
    { (yyval.mapFlags) = ((yyvsp[(1) - (2)].mapFlags) | (yyvsp[(2) - (2)].mapFlags)); }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 290 "parser.y"
    { (yyval.mapFlags) = (yyvsp[(1) - (1)].mapFlags); }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 293 "parser.y"
    { (yyval.mapFlags) = MAP_IS_PARTIAL; }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 294 "parser.y"
    { (yyval.mapFlags) = MAP_IS_DEFAULT; }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 295 "parser.y"
    { (yyval.mapFlags) = MAP_IS_HIDDEN; }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 296 "parser.y"
    { (yyval.mapFlags) = MAP_HAS_ALPHANUMERIC; }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 297 "parser.y"
    { (yyval.mapFlags) = MAP_HAS_MODIFIER; }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 298 "parser.y"
    { (yyval.mapFlags) = MAP_HAS_KEYPAD; }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 299 "parser.y"
    { (yyval.mapFlags) = MAP_HAS_FN; }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 300 "parser.y"
    { (yyval.mapFlags) = MAP_IS_ALTGR; }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 304 "parser.y"
    { (yyval.any) = AppendStmt((yyvsp[(1) - (2)].any), (yyvsp[(2) - (2)].any)); }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 305 "parser.y"
    { (yyval.any) = NULL; }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 309 "parser.y"
    {
                            (yyvsp[(2) - (2)].var)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].var);
                        }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 314 "parser.y"
    {
                            (yyvsp[(2) - (2)].vmod)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].vmod);
                        }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 319 "parser.y"
    {
                            (yyvsp[(2) - (2)].interp)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].interp);
                        }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 324 "parser.y"
    {
                            (yyvsp[(2) - (2)].keyCode)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].keyCode);
                        }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 329 "parser.y"
    {
                            (yyvsp[(2) - (2)].keyAlias)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].keyAlias);
                        }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 334 "parser.y"
    {
                            (yyvsp[(2) - (2)].keyType)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].keyType);
                        }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 339 "parser.y"
    {
                            (yyvsp[(2) - (2)].syms)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].syms);
                        }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 344 "parser.y"
    {
                            (yyvsp[(2) - (2)].modMask)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].modMask);
                        }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 349 "parser.y"
    {
                            (yyvsp[(2) - (2)].groupCompat)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].groupCompat);
                        }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 354 "parser.y"
    {
                            (yyvsp[(2) - (2)].ledMap)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].ledMap);
                        }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 359 "parser.y"
    {
                            (yyvsp[(2) - (2)].ledName)->merge = (yyvsp[(1) - (2)].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[(2) - (2)].ledName);
                        }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 363 "parser.y"
    { (yyval.any) = NULL; }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 364 "parser.y"
    { (yyval.any) = NULL; }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 365 "parser.y"
    { (yyval.any) = NULL; }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 367 "parser.y"
    {
                            (yyval.any) = (ParseCommon *) IncludeCreate(param->ctx, (yyvsp[(2) - (2)].str), (yyvsp[(1) - (2)].merge));
                            free((yyvsp[(2) - (2)].str));
                        }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 374 "parser.y"
    { (yyval.var) = VarCreate((yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 376 "parser.y"
    { (yyval.var) = BoolVarCreate((yyvsp[(1) - (2)].sval), true); }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 378 "parser.y"
    { (yyval.var) = BoolVarCreate((yyvsp[(2) - (3)].sval), false); }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 382 "parser.y"
    { (yyval.keyCode) = KeycodeCreate((yyvsp[(1) - (4)].sval), (yyvsp[(3) - (4)].num)); }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 386 "parser.y"
    { (yyval.keyAlias) = KeyAliasCreate((yyvsp[(2) - (5)].sval), (yyvsp[(4) - (5)].sval)); }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 390 "parser.y"
    { (yyval.vmod) = (yyvsp[(2) - (3)].vmod); }
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 394 "parser.y"
    { (yyval.vmod) = (VModDef *) AppendStmt((ParseCommon *) (yyvsp[(1) - (3)].vmod),
                                                      (ParseCommon *) (yyvsp[(3) - (3)].vmod)); }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 397 "parser.y"
    { (yyval.vmod) = (yyvsp[(1) - (1)].vmod); }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 401 "parser.y"
    { (yyval.vmod) = VModCreate((yyvsp[(1) - (1)].sval), NULL); }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 403 "parser.y"
    { (yyval.vmod) = VModCreate((yyvsp[(1) - (3)].sval), (yyvsp[(3) - (3)].expr)); }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 409 "parser.y"
    { (yyvsp[(2) - (6)].interp)->def = (yyvsp[(4) - (6)].var); (yyval.interp) = (yyvsp[(2) - (6)].interp); }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 413 "parser.y"
    { (yyval.interp) = InterpCreate((yyvsp[(1) - (3)].keysym), (yyvsp[(3) - (3)].expr)); }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 415 "parser.y"
    { (yyval.interp) = InterpCreate((yyvsp[(1) - (1)].keysym), NULL); }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 419 "parser.y"
    { (yyval.var) = (VarDef *) AppendStmt((ParseCommon *) (yyvsp[(1) - (2)].var),
                                                     (ParseCommon *) (yyvsp[(2) - (2)].var)); }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 422 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (1)].var); }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 428 "parser.y"
    { (yyval.keyType) = KeyTypeCreate((yyvsp[(2) - (6)].sval), (yyvsp[(4) - (6)].var)); }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 434 "parser.y"
    { (yyval.syms) = SymbolsCreate((yyvsp[(2) - (6)].sval), (yyvsp[(4) - (6)].var)); }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 438 "parser.y"
    { (yyval.var) = (VarDef *) AppendStmt((ParseCommon *) (yyvsp[(1) - (3)].var),
                                                     (ParseCommon *) (yyvsp[(3) - (3)].var)); }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 441 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (1)].var); }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 442 "parser.y"
    { (yyval.var) = NULL; }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 445 "parser.y"
    { (yyval.var) = VarCreate((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 446 "parser.y"
    { (yyval.var) = VarCreate((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 447 "parser.y"
    { (yyval.var) = BoolVarCreate((yyvsp[(1) - (1)].sval), true); }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 448 "parser.y"
    { (yyval.var) = BoolVarCreate((yyvsp[(2) - (2)].sval), false); }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 449 "parser.y"
    { (yyval.var) = VarCreate(NULL, (yyvsp[(1) - (1)].expr)); }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 453 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 455 "parser.y"
    { (yyval.expr) = ExprCreateUnary(EXPR_ACTION_LIST, EXPR_TYPE_ACTION, (yyvsp[(2) - (3)].expr)); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 459 "parser.y"
    { (yyval.groupCompat) = GroupCompatCreate((yyvsp[(2) - (5)].ival), (yyvsp[(4) - (5)].expr)); }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 463 "parser.y"
    { (yyval.modMask) = ModMapCreate((yyvsp[(2) - (6)].sval), (yyvsp[(4) - (6)].expr)); }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 467 "parser.y"
    { (yyval.ledMap) = LedMapCreate((yyvsp[(2) - (6)].sval), (yyvsp[(4) - (6)].var)); }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 471 "parser.y"
    { (yyval.ledName) = LedNameCreate((yyvsp[(2) - (5)].ival), (yyvsp[(4) - (5)].expr), false); }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 473 "parser.y"
    { (yyval.ledName) = LedNameCreate((yyvsp[(3) - (6)].ival), (yyvsp[(5) - (6)].expr), true); }
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 477 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 479 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 483 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 486 "parser.y"
    { (yyval.geom) = NULL;}
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 487 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 491 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 493 "parser.y"
    { FreeStmt((ParseCommon *) (yyvsp[(1) - (1)].var)); (yyval.geom) = NULL; }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 495 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 497 "parser.y"
    { FreeStmt((ParseCommon *) (yyvsp[(1) - (1)].ledMap)); (yyval.geom) = NULL; }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 499 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 502 "parser.y"
    { (yyval.geom) = NULL;}
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 503 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 506 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 508 "parser.y"
    { FreeStmt((ParseCommon *) (yyvsp[(1) - (1)].var)); (yyval.geom) = NULL; }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 511 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 512 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 516 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 518 "parser.y"
    { FreeStmt((ParseCommon *) (yyvsp[(2) - (3)].expr)); (yyval.geom) = NULL; }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 522 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 525 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 526 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 529 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 533 "parser.y"
    { (yyval.geom) = NULL;}
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 535 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 539 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 541 "parser.y"
    { (yyval.geom) = NULL; }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 543 "parser.y"
    { FreeStmt((ParseCommon *) (yyvsp[(3) - (3)].expr)); (yyval.geom) = NULL; }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 547 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 549 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 553 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 557 "parser.y"
    { FreeStmt((ParseCommon *) (yyvsp[(4) - (6)].var)); (yyval.geom) = NULL; }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 560 "parser.y"
    { (yyval.ival) = 0; }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 561 "parser.y"
    { (yyval.ival) = 0; }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 562 "parser.y"
    { (yyval.ival) = 0; }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 563 "parser.y"
    { (yyval.ival) = 0; }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 566 "parser.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].sval); }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 567 "parser.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].sval); }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 571 "parser.y"
    { (yyval.sval) = xkb_atom_intern_literal(param->ctx, "action"); }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 573 "parser.y"
    { (yyval.sval) = xkb_atom_intern_literal(param->ctx, "interpret"); }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 575 "parser.y"
    { (yyval.sval) = xkb_atom_intern_literal(param->ctx, "type"); }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 577 "parser.y"
    { (yyval.sval) = xkb_atom_intern_literal(param->ctx, "key"); }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 579 "parser.y"
    { (yyval.sval) = xkb_atom_intern_literal(param->ctx, "group"); }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 581 "parser.y"
    {(yyval.sval) = xkb_atom_intern_literal(param->ctx, "modifier_map");}
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 583 "parser.y"
    { (yyval.sval) = xkb_atom_intern_literal(param->ctx, "indicator"); }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 585 "parser.y"
    { (yyval.sval) = XKB_ATOM_NONE; }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 587 "parser.y"
    { (yyval.sval) = XKB_ATOM_NONE; }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 589 "parser.y"
    { (yyval.sval) = XKB_ATOM_NONE; }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 591 "parser.y"
    { (yyval.sval) = XKB_ATOM_NONE; }
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 594 "parser.y"
    { (yyval.merge) = (yyvsp[(1) - (1)].merge); }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 595 "parser.y"
    { (yyval.merge) = MERGE_DEFAULT; }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 598 "parser.y"
    { (yyval.merge) = MERGE_DEFAULT; }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 599 "parser.y"
    { (yyval.merge) = MERGE_AUGMENT; }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 600 "parser.y"
    { (yyval.merge) = MERGE_OVERRIDE; }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 601 "parser.y"
    { (yyval.merge) = MERGE_REPLACE; }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 603 "parser.y"
    {
                    /*
                     * This used to be MERGE_ALT_FORM. This functionality was
                     * unused and has been removed.
                     */
                    (yyval.merge) = MERGE_DEFAULT;
                }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 612 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 613 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 617 "parser.y"
    { (yyval.expr) = (ExprDef *) AppendStmt((ParseCommon *) (yyvsp[(1) - (3)].expr),
                                                      (ParseCommon *) (yyvsp[(3) - (3)].expr)); }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 620 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 624 "parser.y"
    { (yyval.expr) = ExprCreateBinary(EXPR_DIVIDE, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 626 "parser.y"
    { (yyval.expr) = ExprCreateBinary(EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 628 "parser.y"
    { (yyval.expr) = ExprCreateBinary(EXPR_SUBTRACT, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 630 "parser.y"
    { (yyval.expr) = ExprCreateBinary(EXPR_MULTIPLY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 632 "parser.y"
    { (yyval.expr) = ExprCreateBinary(EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 634 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 638 "parser.y"
    { (yyval.expr) = ExprCreateUnary(EXPR_NEGATE, (yyvsp[(2) - (2)].expr)->expr.value_type, (yyvsp[(2) - (2)].expr)); }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 640 "parser.y"
    { (yyval.expr) = ExprCreateUnary(EXPR_UNARY_PLUS, (yyvsp[(2) - (2)].expr)->expr.value_type, (yyvsp[(2) - (2)].expr)); }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 642 "parser.y"
    { (yyval.expr) = ExprCreateUnary(EXPR_NOT, EXPR_TYPE_BOOLEAN, (yyvsp[(2) - (2)].expr)); }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 644 "parser.y"
    { (yyval.expr) = ExprCreateUnary(EXPR_INVERT, (yyvsp[(2) - (2)].expr)->expr.value_type, (yyvsp[(2) - (2)].expr)); }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 646 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);  }
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 648 "parser.y"
    { (yyval.expr) = ExprCreateAction((yyvsp[(1) - (4)].sval), (yyvsp[(3) - (4)].expr)); }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 650 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);  }
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr);  }
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 656 "parser.y"
    { (yyval.expr) = (ExprDef *) AppendStmt((ParseCommon *) (yyvsp[(1) - (3)].expr),
                                                      (ParseCommon *) (yyvsp[(3) - (3)].expr)); }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 659 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 663 "parser.y"
    { (yyval.expr) = ExprCreateAction((yyvsp[(1) - (4)].sval), (yyvsp[(3) - (4)].expr)); }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 667 "parser.y"
    { (yyval.expr) = ExprCreateIdent((yyvsp[(1) - (1)].sval)); }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 669 "parser.y"
    { (yyval.expr) = ExprCreateFieldRef((yyvsp[(1) - (3)].sval), (yyvsp[(3) - (3)].sval)); }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 671 "parser.y"
    { (yyval.expr) = ExprCreateArrayRef(XKB_ATOM_NONE, (yyvsp[(1) - (4)].sval), (yyvsp[(3) - (4)].expr)); }
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 673 "parser.y"
    { (yyval.expr) = ExprCreateArrayRef((yyvsp[(1) - (6)].sval), (yyvsp[(3) - (6)].sval), (yyvsp[(5) - (6)].expr)); }
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 677 "parser.y"
    { (yyval.expr) = ExprCreateString((yyvsp[(1) - (1)].sval)); }
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 679 "parser.y"
    { (yyval.expr) = ExprCreateInteger((yyvsp[(1) - (1)].ival)); }
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 681 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 683 "parser.y"
    { (yyval.expr) = ExprCreateKeyName((yyvsp[(1) - (1)].sval)); }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 686 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 687 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 691 "parser.y"
    { (yyval.expr) = ExprAppendKeysymList((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].keysym)); }
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 693 "parser.y"
    { (yyval.expr) = ExprAppendMultiKeysymList((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 695 "parser.y"
    { (yyval.expr) = ExprCreateKeysymList((yyvsp[(1) - (1)].keysym)); }
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 697 "parser.y"
    { (yyval.expr) = ExprCreateMultiKeysymList((yyvsp[(1) - (1)].expr)); }
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 701 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); }
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 705 "parser.y"
    {
                            if (!resolve_keysym((yyvsp[(1) - (1)].str), &(yyval.keysym)))
                                parser_warn(param, "unrecognized keysym");
                            free((yyvsp[(1) - (1)].str));
                        }
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 710 "parser.y"
    { (yyval.keysym) = XKB_KEY_section; }
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 712 "parser.y"
    {
                            if ((yyvsp[(1) - (1)].ival) < 0) {
                                parser_warn(param, "unrecognized keysym");
                                (yyval.keysym) = XKB_KEY_NoSymbol;
                            }
                            else if ((yyvsp[(1) - (1)].ival) < 10) {      /* XKB_KEY_0 .. XKB_KEY_9 */
                                (yyval.keysym) = XKB_KEY_0 + (xkb_keysym_t) (yyvsp[(1) - (1)].ival);
                            }
                            else {
                                char buf[17];
                                snprintf(buf, sizeof(buf), "0x%x", (yyvsp[(1) - (1)].ival));
                                if (!resolve_keysym(buf, &(yyval.keysym))) {
                                    parser_warn(param, "unrecognized keysym");
                                    (yyval.keysym) = XKB_KEY_NoSymbol;
                                }
                            }
                        }
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 731 "parser.y"
    { (yyval.ival) = -(yyvsp[(2) - (2)].ival); }
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 732 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); }
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 735 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].num); }
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 736 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].num); }
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 739 "parser.y"
    { (yyval.ival) = 0; }
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 742 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].num); }
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 745 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); }
    break;

  case 179:

/* Line 1806 of yacc.c  */
#line 748 "parser.y"
    { (yyval.sval) = xkb_atom_steal(param->ctx, (yyvsp[(1) - (1)].str)); }
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 749 "parser.y"
    { (yyval.sval) = xkb_atom_intern_literal(param->ctx, "default"); }
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 752 "parser.y"
    { (yyval.sval) = xkb_atom_steal(param->ctx, (yyvsp[(1) - (1)].str)); }
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 755 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 756 "parser.y"
    { (yyval.str) = NULL; }
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 759 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;



/* Line 1806 of yacc.c  */
#line 3331 "src/xkbcomp/parser.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

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
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (param, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (param, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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
		      yytoken, &yylval, param);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
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
      if (!yypact_value_is_default (yyn))
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, param);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (param, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, param);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, param);
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



/* Line 2067 of yacc.c  */
#line 762 "parser.y"


XkbFile *
parse(struct xkb_context *ctx, struct scanner *scanner, const char *map)
{
    int ret;
    XkbFile *first = NULL;
    struct parser_param param = {
        .scanner = scanner,
        .ctx = ctx,
    };

    /*
     * If we got a specific map, we look for it exclusively and return
     * immediately upon finding it. Otherwise, we need to get the
     * default map. If we find a map marked as default, we return it
     * immediately. If there are no maps marked as default, we return
     * the first map in the file.
     */

    while ((ret = yyparse(&param)) == 0 && param.more_maps) {
        if (map) {
            if (streq_not_null(map, param.rtrn->name))
                return param.rtrn;
            else
                FreeXkbFile(param.rtrn);
        }
        else {
            if (param.rtrn->flags & MAP_IS_DEFAULT) {
                FreeXkbFile(first);
                return param.rtrn;
            }
            else if (!first) {
                first = param.rtrn;
            }
            else {
                FreeXkbFile(param.rtrn);
            }
        }
    }

    if (ret != 0) {
        FreeXkbFile(first);
        return NULL;
    }

    return first;
}

