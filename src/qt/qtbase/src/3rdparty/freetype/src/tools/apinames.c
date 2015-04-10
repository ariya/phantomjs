/*
 * This little program is used to parse the FreeType headers and
 * find the declaration of all public APIs.  This is easy, because
 * they all look like the following:
 *
 *   FT_EXPORT( return_type )
 *   function_name( function arguments );
 *
 * You must pass the list of header files as arguments.  Wildcards are
 * accepted if you are using GCC for compilation (and probably by
 * other compilers too).
 *
 * Author: David Turner, 2005, 2006, 2008, 2009
 *
 * This code is explicitly placed into the public domain.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define  PROGRAM_NAME     "apinames"
#define  PROGRAM_VERSION  "0.1"

#define  LINEBUFF_SIZE  1024

typedef enum  OutputFormat_
{
  OUTPUT_LIST = 0,      /* output the list of names, one per line             */
  OUTPUT_WINDOWS_DEF,   /* output a Windows .DEF file for Visual C++ or Mingw */
  OUTPUT_BORLAND_DEF,   /* output a Windows .DEF file for Borland C++         */
  OUTPUT_WATCOM_LBC     /* output a Watcom Linker Command File                */

} OutputFormat;


static void
panic( const char*  message )
{
  fprintf( stderr, "PANIC: %s\n", message );
  exit(2);
}


typedef struct  NameRec_
{
  char*         name;
  unsigned int  hash;

} NameRec, *Name;

static Name  the_names;
static int   num_names;
static int   max_names;

static void
names_add( const char*  name,
           const char*  end )
{
  int   nn, len, h;
  Name  nm;

  if ( end <= name )
    return;

  /* compute hash value */
  len = (int)(end - name);
  h   = 0;
  for ( nn = 0; nn < len; nn++ )
    h = h*33 + name[nn];

  /* check for an pre-existing name */
  for ( nn = 0; nn < num_names; nn++ )
  {
    nm = the_names + nn;

    if ( (int)nm->hash                 == h &&
         memcmp( name, nm->name, len ) == 0 &&
         nm->name[len]                 == 0 )
      return;
  }

  /* add new name */
  if ( num_names >= max_names )
  {
    max_names += (max_names >> 1) + 4;
    the_names  = (NameRec*)realloc( the_names, sizeof(the_names[0])*max_names );
    if ( the_names == NULL )
      panic( "not enough memory" );
  }
  nm = &the_names[num_names++];

  nm->hash = h;
  nm->name = (char*)malloc( len+1 );
  if ( nm->name == NULL )
    panic( "not enough memory" );

  memcpy( nm->name, name, len );
  nm->name[len] = 0;
}


static int
name_compare( const void*  name1,
              const void*  name2 )
{
  Name  n1 = (Name)name1;
  Name  n2 = (Name)name2;

  return strcmp( n1->name, n2->name );
}

static void
names_sort( void )
{
  qsort( the_names, (size_t)num_names, sizeof(the_names[0]), name_compare );
}


static void
names_dump( FILE*         out,
            OutputFormat  format,
            const char*   dll_name )
{
  int  nn;

  switch ( format )
  {
    case OUTPUT_WINDOWS_DEF:
      if ( dll_name )
        fprintf( out, "LIBRARY %s\n", dll_name );

      fprintf( out, "DESCRIPTION  FreeType 2 DLL\n" );
      fprintf( out, "EXPORTS\n" );
      for ( nn = 0; nn < num_names; nn++ )
        fprintf( out, "  %s\n", the_names[nn].name );
      break;

    case OUTPUT_BORLAND_DEF:
      if ( dll_name )
        fprintf( out, "LIBRARY %s\n", dll_name );

      fprintf( out, "DESCRIPTION  FreeType 2 DLL\n" );
      fprintf( out, "EXPORTS\n" );
      for ( nn = 0; nn < num_names; nn++ )
        fprintf( out, "  _%s\n", the_names[nn].name );
      break;

    case OUTPUT_WATCOM_LBC:
      {
        /* we must omit the .dll suffix from the library name */
        char   temp[512];
        char*  dot;

        if ( dll_name == NULL )
        {
          fprintf( stderr,
                   "you must provide a DLL name with the -d option !!\n" );
          exit(4);
        }

        dot = strchr( dll_name, '.' );
        if ( dot != NULL )
        {
          int  len = (dot - dll_name);
          if ( len > (int)(sizeof(temp)-1) )
            len = sizeof(temp)-1;

          memcpy( temp, dll_name, len );
          temp[len] = 0;

          dll_name = (const char*)temp;
        }

        for ( nn = 0; nn < num_names; nn++ )
          fprintf( out, "++_%s.%s.%s\n", the_names[nn].name, dll_name,
                        the_names[nn].name );
      }
      break;

    default:  /* LIST */
      for ( nn = 0; nn < num_names; nn++ )
        fprintf( out, "%s\n", the_names[nn].name );
  }
}




/* states of the line parser */

typedef enum  State_
{
  STATE_START = 0,  /* waiting for FT_EXPORT keyword and return type */
  STATE_TYPE        /* type was read, waiting for function name      */

} State;

static int
read_header_file( FILE*  file, int  verbose )
{
  static char  buff[ LINEBUFF_SIZE+1 ];
  State        state = STATE_START;

  while ( !feof( file ) )
  {
    char*  p;

    if ( !fgets( buff, LINEBUFF_SIZE, file ) )
      break;

    p = buff;

    while ( *p && (*p == ' ' || *p == '\\') )  /* skip leading whitespace */
      p++;

    if ( *p == '\n' || *p == '\r' )  /* skip empty lines */
      continue;

    switch ( state )
    {
      case STATE_START:
        {
          if ( memcmp( p, "FT_EXPORT(", 10 ) != 0 )
            break;

          p += 10;
          for (;;)
          {
            if ( *p == 0 || *p == '\n' || *p == '\r' )
              goto NextLine;

            if ( *p == ')' )
            {
              p++;
              break;
            }

            p++;
          }

          state = STATE_TYPE;

         /* sometimes, the name is just after the FT_EXPORT(...), so
          * skip whitespace, and fall-through if we find an alphanumeric
          * character
          */
          while ( *p == ' ' || *p == '\t' )
            p++;

          if ( !isalpha(*p) )
            break;
        }
        /* fall-through */

      case STATE_TYPE:
        {
          char*   name = p;

          while ( isalnum(*p) || *p == '_' )
            p++;

          if ( p > name )
          {
            if ( verbose )
              fprintf( stderr, ">>> %.*s\n", (int)(p - name), name );

            names_add( name, p );
          }

          state = STATE_START;
        }
        break;

      default:
        ;
    }

  NextLine:
    ;
  }

  return 0;
}


static void
usage( void )
{
  static const char* const  format =
   "%s %s: extract FreeType API names from header files\n\n"
   "this program is used to extract the list of public FreeType API\n"
   "functions. It receives the list of header files as argument and\n"
   "generates a sorted list of unique identifiers\n\n"

   "usage: %s header1 [options] [header2 ...]\n\n"

   "options:   -      : parse the content of stdin, ignore arguments\n"
   "           -v     : verbose mode, output sent to standard error\n"
   "           -oFILE : write output to FILE instead of standard output\n"
   "           -dNAME : indicate DLL file name, 'freetype.dll' by default\n"
   "           -w     : output .DEF file for Visual C++ and Mingw\n"
   "           -wB    : output .DEF file for Borland C++\n"
   "           -wW    : output Watcom Linker Response File\n"
   "\n";

  fprintf( stderr,
           format,
           PROGRAM_NAME,
           PROGRAM_VERSION,
           PROGRAM_NAME
           );
  exit(1);
}


int  main( int argc, const char* const*  argv )
{
  int           from_stdin = 0;
  int           verbose = 0;
  OutputFormat  format = OUTPUT_LIST;  /* the default */
  FILE*         out    = stdout;
  const char*   library_name = NULL;

  if ( argc < 2 )
    usage();

  /* '-' used as a single argument means read source file from stdin */
  while ( argc > 1 && argv[1][0] == '-' )
  {
    const char*  arg = argv[1];

    switch ( arg[1] )
    {
      case 'v':
        verbose = 1;
        break;

      case 'o':
        if ( arg[2] == 0 )
        {
          if ( argc < 2 )
            usage();

          arg = argv[2];
          argv++;
          argc--;
        }
        else
          arg += 2;

        out = fopen( arg, "wt" );
        if ( out == NULL )
        {
          fprintf( stderr, "could not open '%s' for writing\n", argv[2] );
          exit(3);
        }
        break;

      case 'd':
        if ( arg[2] == 0 )
        {
          if ( argc < 2 )
            usage();

          arg = argv[2];
          argv++;
          argc--;
        }
        else
          arg += 2;

        library_name = arg;
        break;

      case 'w':
        format = OUTPUT_WINDOWS_DEF;
        switch ( arg[2] )
        {
          case 'B':
            format = OUTPUT_BORLAND_DEF;
            break;

          case 'W':
            format = OUTPUT_WATCOM_LBC;
            break;

          case 0:
            break;

          default:
            usage();
        }
        break;

      case 0:
        from_stdin = 1;
        break;

      default:
        usage();
    }

    argc--;
    argv++;
  }

  if ( from_stdin )
  {
    read_header_file( stdin, verbose );
  }
  else
  {
    for ( --argc, argv++; argc > 0; argc--, argv++ )
    {
      FILE*  file = fopen( argv[0], "rb" );

      if ( file == NULL )
        fprintf( stderr, "unable to open '%s'\n", argv[0] );
      else
      {
        if ( verbose )
          fprintf( stderr, "opening '%s'\n", argv[0] );

        read_header_file( file, verbose );
        fclose( file );
      }
    }
  }

  if ( num_names == 0 )
    panic( "could not find exported functions !!\n" );

  names_sort();
  names_dump( out, format, library_name );

  if ( out != stdout )
    fclose( out );

  return 0;
}
