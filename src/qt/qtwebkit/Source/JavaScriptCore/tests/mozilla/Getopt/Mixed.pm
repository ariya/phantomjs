#---------------------------------------------------------------------
package Getopt::Mixed;
#
# Copyright 1995 Christopher J. Madsen
#
# Author: Christopher J. Madsen <ac608@yfn.ysu.edu>
# Created: 1 Jan 1995
# Version: $Revision: 1.8 $ ($Date: 1996/02/09 00:05:00 $)
#    Note that RCS revision 1.23 => $Getopt::Mixed::VERSION = "1.023"
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Perl; see the file COPYING.  If not, write to the
# Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Process both single-character and extended options
#---------------------------------------------------------------------

require 5.000;
use Carp;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = ();
@EXPORT_OK = qw(abortMsg getOptions nextOption);

#=====================================================================
# Package Global Variables:

BEGIN
{
    # The permissible settings for $order:
    $REQUIRE_ORDER   = 0;
    $PERMUTE         = 1;
    $RETURN_IN_ORDER = 2;

    # Regular expressions:
    $intRegexp   = '^[-+]?\d+$';               # Match an integer
    $floatRegexp = '^[-+]?(\d*\.?\d+|\d+\.)$'; # Match a real number
    $typeChars   = 'sif';                      # Match type characters

    # Convert RCS revision number (must be main branch) to d.ddd format:
    ' $Revision: 1.8 $ ' =~ / (\d+)\.(\d{1,3}) /
        or die "Invalid version number";
    $VERSION = sprintf("%d.%03d",$1,$2);
} # end BEGIN

#=====================================================================
# Subroutines:
#---------------------------------------------------------------------
# Initialize the option processor:
#
# You should set any customization variables *after* calling init.
#
# For a description of option declarations, see the documentation at
# the end of this file.
#
# Input:
#   List of option declarations (separated by whitespace)
#     If the first argument is entirely non-alphanumeric characters
#     with no whitespace, it is the characters that start options.

sub init
{
    undef %options;
    my($opt,$type);

    $ignoreCase  = 1;           # Ignore case by default
    $optionStart = "-";         # Dash is the default option starter

    # If the first argument is entirely non-alphanumeric characters
    # with no whitespace, it is the desired value for $optionStart:
    $optionStart = shift @_ if $_[0] =~ /^[^a-z0-9\s]+$/i;

    foreach $group (@_) {
        # Ignore case unless there are upper-case options:
        $ignoreCase = 0 if $group =~ /[A-Z]/;
        foreach $option (split(/\s+/,$group)) {
            croak "Invalid option declaration `$option'"
                unless $option =~ /^([^=:>]+)([=:][$typeChars]|>[^=:>]+)?$/o;
            $opt  = $1;
            $type = $2 || "";
            if ($type =~ /^>(.*)$/) {
                $type = $1;
                croak "Invalid synonym `$option'"
                    if (not defined $options{$type}
                        or $options{$type} =~ /^[^:=]/);
            } # end if synonym
            $options{$opt} = $type;
        } # end foreach option
    } # end foreach group

    # Handle POSIX compliancy:
    if (defined $ENV{"POSIXLY_CORRECT"}) {
        $order = $REQUIRE_ORDER;
    } else {
        $order = $PERMUTE;
    }

    $optionEnd = 0;
    $badOption = \&badOption;
    $checkArg  = \&checkArg;
} # end init

#---------------------------------------------------------------------
# Clean up when we're done:
#
# This just releases the memory used by the %options hash.
#
# If 'help' was defined as an option, a new hash with just 'help' is
# created, in case the program calls abortMsg.

sub cleanup
{
    my $help = defined($options{'help'});
    undef %options;
    $options{'help'} = "" if $help;
} # end cleanup

#---------------------------------------------------------------------
# Abort program with message:
#
# Prints program name and arguments to STDERR
# If --help is an option, prints message saying 'Try --help'
# Exits with code 1

sub abortMsg
{
    my $name = $0;
    $name =~ s|^.+[\\/]||;      # Remove any directories from name
    print STDERR $name,": ",@_,"\n";
    print STDERR "Try `$name --help' for more information.\n"
        if defined $options{"help"};
    exit 1;
} # end abortMsg

#---------------------------------------------------------------------
# Standard function for handling bad options:
#
# Prints an error message and exits.
#
# You can override this by setting $Getopt::Mixed::badOption to a
# function reference.
#
# Input:
#   Index into @ARGV
#   The option that caused the error
#   An optional string describing the problem
#     Currently, this can be
#       undef        The option was not recognized
#       'ambiguous'  The option could match several long options
#
# Note:
#   The option has already been removed from @ARGV.  To put it back,
#   you can say:
#     splice(@ARGV,$_[0],0,$_[1]);
#
#   If your function returns, it should return whatever you want
#   nextOption to return.

sub badOption
{
    my ($index, $option, $problem) = @_;

    $problem = 'unrecognized' unless $problem;

    abortMsg("$problem option `$option'");
} # end badOption

#---------------------------------------------------------------------
# Make sure we have the proper argument for this option:
#
# You can override this by setting $Getopt::Mixed::checkArg to a
# function reference.
#
# Input:
#   $i:       Position of argument in @ARGV
#   $value:   The text appended to the option (undef if no text)
#   $option:  The pretty name of the option (as the user typed it)
#   $type:    The type of the option
#
# Returns:
#   The value of the option's argument

sub checkArg
{
    my ($i,$value,$option,$type) = @_;

    abortMsg("option `$option' does not take an argument")
        if (not $type and defined $value);

    if ($type =~ /^=/) {
        # An argument is required for this option:
        $value = splice(@ARGV,$i,1) unless defined $value;
        abortMsg("option `$option' requires an argument")
            unless defined $value;
    }

    if ($type =~ /i$/) {
        abortMsg("option `$option' requires integer argument")
            if (defined $value and $value !~ /$intRegexp/o);
    }
    elsif ($type =~ /f$/) {
        abortMsg("option `$option' requires numeric argument")
            if (defined $value and $value !~ /$floatRegexp/o);
    }
    elsif ($type =~ /^[=:]/ and ref($checkType)) {
        $value = &$checkType($i,$value,$option,$type);
    }

    $value = "" if not defined $value and $type =~ /^:/;

    $value;
} # end checkArg

#---------------------------------------------------------------------
# Find a match for an incomplete long option:
#
# Input:
#   The option text to match
#
# Returns:
#   The option that matched, or
#   undef, if no option matched, or
#   (undef, 'ambiguous'), if multiple options matched

sub findMatch
{
    my $opt = shift;

    $opt =~ s/-/[^-]*-/g;
    $opt .= ".*";

    my @matches = grep(/^$opt$/, keys %options);

    return undef       if $#matches <  0;
    return $matches[0] if $#matches == 0;

    $opt = $matches[0];
    $opt = $options{$opt} if $options{$opt} =~ /^[^=:]/;

    foreach (@matches) {
        return (undef, 'ambiguous')
            unless $_ eq $opt or $options{$_} eq $opt;
    }

    $opt;
} # end findMatch

#---------------------------------------------------------------------
# Return the next option:
#
# Returns a list of 3 elements:  (OPTION, VALUE, PRETTYNAME), where
#   OPTION is the name of the option,
#   VALUE is its argument, and
#   PRETTYNAME is the option as the user entered it.
# Returns the null list if there are no more options to process
#
# If $order is $RETURN_IN_ORDER, and this is a normal argument (not an
# option), OPTION will be the null string, VALUE will be the argument,
# and PRETTYNAME will be undefined.

sub nextOption
{
    return () if $#ARGV < 0;    # No more arguments

    if ($optionEnd) {
        # We aren't processing any more options:
        return ("", shift @ARGV) if $order == $RETURN_IN_ORDER;
        return ();
    }

    # Find the next option:
    my $i = 0;
    while (length($ARGV[$i]) < 2 or
           index($optionStart,substr($ARGV[$i],0,1)) < 0) {
        return ()                if $order == $REQUIRE_ORDER;
        return ("", shift @ARGV) if $order == $RETURN_IN_ORDER;
        ++$i;
        return () if $i > $#ARGV;
    } # end while

    # Process the option:
    my($option,$opt,$value,$optType,$prettyOpt);
    $option = $ARGV[$i];
    if (substr($option,0,1) eq substr($option,1,1)) {
        # If the option start character is repeated, it's a long option:
        splice @ARGV,$i,1;
        if (length($option) == 2) {
            # A double dash by itself marks the end of the options:
            $optionEnd = 1;     # Don't process any more options
            return nextOption();
        } # end if bare double dash
        $opt = substr($option,2);
        if ($opt =~ /^([^=]+)=(.*)$/) {
            $opt = $1;
            $value = $2;
        } # end if option is followed by value
        $opt =~ tr/A-Z/a-z/ if $ignoreCase;
        $prettyOpt = substr($option,0,2) . $opt;
        my $problem;
        ($opt, $problem) = findMatch($opt)
            unless defined $options{$opt} and length($opt) > 1;
        return &$badOption($i,$option,$problem) unless $opt;
        $optType = $options{$opt};
        if ($optType =~ /^[^:=]/) {
            $opt = $optType;
            $optType = $options{$opt};
        }
        $value = &$checkArg($i,$value,$prettyOpt,$optType);
    } # end if long option
    else {
        # It's a short option:
        $opt = substr($option,1,1);
        $opt =~ tr/A-Z/a-z/ if $ignoreCase;
        return &$badOption($i,$option) unless defined $options{$opt};
        $optType = $options{$opt};
        if ($optType =~ /^[^:=]/) {
            $opt = $optType;
            $optType = $options{$opt};
        }
        if (length($option) == 2 or $optType) {
            # This is the last option in the group, so remove the group:
            splice(@ARGV,$i,1);
        } else {
            # Just remove this option from the group:
            substr($ARGV[$i],1,1) = "";
        }
        if ($optType) {
            $value = (length($option) > 2) ? substr($option,2) : undef;
            $value =~ s/^=// if $value; # Allow either -d3 or -d=3
        } # end if option takes an argument
        $prettyOpt = substr($option,0,2);
        $value = &$checkArg($i,$value,$prettyOpt,$optType);
    } # end else short option
    ($opt,$value,$prettyOpt);
} # end nextOption

#---------------------------------------------------------------------
# Get options:
#
# Input:
#   The same as for init()
#   If no parameters are supplied, init() is NOT called.  This allows
#   you to call init() yourself and then change the configuration
#   variables.
#
# Output Variables:
#   Sets $opt_X for each `-X' option encountered.
#
#   Note that if --apple is a synonym for -a, then --apple will cause
#   $opt_a to be set, not $opt_apple.

sub getOptions
{
    &init if $#_ >= 0;        # Pass arguments (if any) on to init

    # If you want to use $RETURN_IN_ORDER, you have to call
    # nextOption yourself; getOptions doesn't support it:
    $order = $PERMUTE if $order == $RETURN_IN_ORDER;

    my ($option,$value,$package);

    $package = (caller)[0];

    while (($option, $value) = nextOption()) {
        $option =~ s/\W/_/g;    # Make a legal Perl identifier
        $value = 1 unless defined $value;
        eval("\$" . $package . '::opt_' . $option . ' = $value;');
    } # end while

    cleanup();
} # end getOptions

#=====================================================================
# Package return value:

$VERSION;

__END__

=head1 NAME

Getopt::Mixed - getopt processing with both long and short options

=head1 SYNOPSIS

    use Getopt::Mixed;
    Getopt::Mixed::getOptions(...option-descriptions...);
    ...examine $opt_* variables...

or

    use Getopt::Mixed "nextOption";
    Getopt::Mixed::init(...option-descriptions...);
    while (($option, $value) = nextOption()) {
        ...process option...
    }
    Getopt::Mixed::cleanup();

=head1 DESCRIPTION

This package is my response to the standard modules Getopt::Std and
Getopt::Long.  C<Std> doesn't support long options, and C<Long>
doesn't support short options.  I wanted both, since long options are
easier to remember and short options are faster to type.

This package is intended to be the "Getopt-to-end-all-Getop's".  It
combines (I hope) flexibility and simplicity.  It supports both short
options (introduced by C<->) and long options (introduced by C<-->).
Short options which do not take an argument can be grouped together.
Short options which do take an argument must be the last option in
their group, because everything following the option will be
considered to be its argument.

There are two methods for using Getopt::Mixed:  the simple method and
the flexible method.  Both methods use the same format for option
descriptions.

=head2 Option Descriptions

The option-description arguments required by C<init> and C<getOptions>
are strings composed of individual option descriptions.  Several
option descriptions can appear in the same string if they are
separated by whitespace.

Each description consists of the option name and an optional trailing
argument specifier.  Option names may consist of any characters but
whitespace, C<=>, C<:>, and C<E<gt>>.

Values for argument specifiers are:

  <none>   option does not take an argument
  =s :s    option takes a mandatory (=) or optional (:) string argument
  =i :i    option takes a mandatory (=) or optional (:) integer argument
  =f :f    option takes a mandatory (=) or optional (:) real number argument
  >new     option is a synonym for option `new'

The C<E<gt>> specifier is not really an argument specifier.  It
defines an option as being a synonym for another option.  For example,
"a=i apples>a" would define B<-a> as an option that requires an
integer argument and B<--apples> as a synonym for B<-a>.  Only one
level of synonyms is supported, and the root option must be listed
first.  For example, "apples>a a=i" and "a=i apples>a oranges>apples"
are illegal; use "a=i apples>a oranges>a" if that's what you want.

For example, in the option description:
     "a b=i c:s apple baker>b charlie:s"
         -a and --apple do not take arguments
         -b takes a mandatory integer argument
         --baker is a synonym for -b
         -c and --charlie take an optional string argument

If the first argument to C<init> or C<getOptions> is entirely
non-alphanumeric characters with no whitespace, it represents the
characters which can begin options.

=head2 User Interface

From the user's perspective, short options are introduced by a dash
(C<->) and long options are introduced by a double dash (C<-->).
Short options may be combined ("-a -b" can be written "-ab"), but an
option that takes an argument must be the last one in its group,
because anything following it is considered part of the argument.  A
double dash by itself marks the end of the options; all arguments
following it are treated as normal arguments, not options.  A single
dash by itself is treated as a normal argument, I<not> an option.

Long options may be abbreviated.  An option B<--all-the-time> could be
abbreviated B<--all>, B<--a--tim>, or even B<--a>.  Note that B<--time>
would not work; the abbreviation must start at the beginning of the
option name.  If an abbreviation is ambiguous, an error message will
be printed.

In the following examples, B<-i> and B<--int> take integer arguments,
B<-f> and B<--float> take floating point arguments, and B<-s> and
B<--string> take string arguments.  All other options do not take an
argument.

  -i24            -f24.5               -sHello
  -i=24 --int=-27 -f=24.5 --float=0.27 -s=Hello --string=Hello

If the argument is required, it can also be separated by whitespace:

  -i 24 --int -27 -f 24.5 --float 0.27 -s Hello --string Hello

Note that if the option is followed by C<=>, whatever follows the C<=>
I<is> the argument, even if it's the null string.  In the example

  -i= 24 -f= 24.5 -s= Hello

B<-i> and B<-f> will cause an error, because the null string is not a
number, but B<-s> is perfectly legal; its argument is the null string,
not "Hello".

Remember that optional arguments I<cannot> be separated from the
option by whitespace.

=head2 The Simple Method

The simple method is

    use Getopt::Mixed;
    Getopt::Mixed::getOptions(...option-descriptions...);

You then examine the C<$opt_*> variables to find out what options were
specified and the C<@ARGV> array to see what arguments are left.

If B<-a> is an option that doesn't take an argument, then C<$opt_a>
will be set to 1 if the option is present, or left undefined if the
option is not present.

If B<-b> is an option that takes an argument, then C<$opt_b> will be
set to the value of the argument if the option is present, or left
undefined if the option is not present.  If the argument is optional
but not supplied, C<$opt_b> will be set to the null string.

Note that even if you specify that an option I<requires> a string
argument, you can still get the null string (if the user specifically
enters it).  If the option requires a numeric argument, you will never
get the null string (because it isn't a number).

When converting the option name to a Perl identifier, any non-word
characters in the name will be converted to underscores (C<_>).

If the same option occurs more than once, only the last occurrence
will be recorded.  If that's not acceptable, you'll have to use the
flexible method instead.

=head2 The Flexible Method

The flexible method is

    use Getopt::Mixed "nextOption";
    Getopt::Mixed::init(...option-descriptions...);
    while (($option, $value, $pretty) = nextOption()) {
        ...process option...
    }
    Getopt::Mixed::cleanup();

This lets you process arguments one at a time.  You can then handle
repeated options any way you want to.  It also lets you see option
names with non-alphanumeric characters without any translation.  This
is also the only method that lets you find out what order the options
and other arguments were in.

First, you call Getopt::Mixed::init with the option descriptions.
Then, you keep calling nextOption until it returns an empty list.
Finally, you call Getopt::Mixed::cleanup when you're done.  The
remaining (non-option) arguments will be found in @ARGV.

Each call to nextOption returns a list of the next option, its value,
and the option as the user typed it.  The value will be undefined if
the option does not take an argument.  The option is stripped of its
starter (e.g., you get "a" and "foo", not "-a" or "--foo").  If you
want to print an error message, use the third element, which does
include the option starter.

=head1 OTHER FUNCTIONS

Getopt::Mixed provides one other function you can use.  C<abortMsg>
prints its arguments on STDERR, plus your program's name and a
newline.  It then exits with status 1.  For example, if F<foo.pl>
calls C<abortMsg> like this:

  Getopt::Mixed::abortMsg("Error");

The output will be:

  foo.pl: Error

=head1 CUSTOMIZATION

There are several customization variables you can set.  All of these
variables should be set I<after> calling Getopt::Mixed::init and
I<before> calling nextOption.

If you set any of these variables, you I<must> check the version
number first.  The easiest way to do this is like this:

    use Getopt::Mixed 1.006;

If you are using the simple method, and you want to set these
variables, you'll need to call init before calling getOptions, like
this:

    use Getopt::Mixed 1.006;
    Getopt::Mixed::init(...option-descriptions...);
    ...set configuration variables...
    Getopt::Mixed::getOptions();      # IMPORTANT: no parameters

=over 4

=item $order

$order can be set to $REQUIRE_ORDER, $PERMUTE, or $RETURN_IN_ORDER.
The default is $REQUIRE_ORDER if the environment variable
POSIXLY_CORRECT has been set, $PERMUTE otherwise.

$REQUIRE_ORDER means that no options can follow the first argument
which isn't an option.

$PERMUTE means that all options are treated as if they preceded all
other arguments.

$RETURN_IN_ORDER means that all arguments maintain their ordering.
When nextOption is called, and the next argument is not an option, it
returns the null string as the option and the argument as the value.
nextOption never returns the null list until all the arguments have
been processed.

=item $ignoreCase

Ignore case when matching options.  Default is 1 unless the option
descriptions contain an upper-case letter.

=item $optionStart

A string of characters that can start options.  Default is "-".

=item $badOption

A reference to a function that is called when an unrecognized option
is encountered.  The function receives three arguments.  $_[0] is the
position in @ARGV where the option came from.  $_[1] is the option as
the user typed it (including the option start character).  $_[2] is
either undef or a string describing the reason the option was not
recognized (Currently, the only possible value is 'ambiguous', for a
long option with several possible matches).  The option has already
been removed from @ARGV.  To put it back, you can say:

    splice(@ARGV,$_[0],0,$_[1]);

The function can do anything you want to @ARGV.  It should return
whatever you want nextOption to return.

The default is a function that prints an error message and exits the
program.

=item $checkArg

A reference to a function that is called to make sure the argument
type is correct.  The function receives four arguments.  $_[0] is the
position in @ARGV where the option came from.  $_[1] is the text
following the option, or undefined if there was no text following the
option.  $_[2] is the name of the option as the user typed it
(including the option start character), suitable for error messages.
$_[3] is the argument type specifier.

The function can do anything you want to @ARGV.  It should return
the value for this option.

The default is a function that prints an error message and exits the
program if the argument is not the right type for the option.  You can
also adjust the behavior of the default function by changing
$intRegexp or $floatRegexp.

=item $intRegexp

A regular expression that matches an integer.  Default is
'^[-+]?\d+$', which matches a string of digits preceded by an
optional sign.  Unlike the other configuration variables, this cannot
be changed after nextOption is called, because the pattern is compiled
only once.

=item $floatRegexp

A regular expression that matches a floating point number.  Default is
'^[-+]?(\d*\.?\d+|\d+\.)$', which matches the following formats:
"123", "123.", "123.45", and ".123" (plus an optional sign).  It does
not match exponential notation.  Unlike the other configuration
variables, this cannot be changed after nextOption is called, because
the pattern is compiled only once.

=item $typeChars

A string of the characters which are legal argument types.  The
default is 'sif', for String, Integer, and Floating point arguments.
The string should consist only of letters.  Upper case letters are
discouraged, since this will hamper the case-folding of options.  If
you change this, you should set $checkType to a function that will
check arguments of your new type.  Unlike the other configuration
variables, this must be set I<before> calling init(), and cannot be
changed afterwards.

=item $checkType

If you add new types to $typeChars, you should set this to a function
which will check arguments of the new types.

=back

=head1 BUGS

=over 4

=item *

This document should be expanded.

=item *

A long option must be at least two characters long.  Sorry.

=item *

The C<!> argument specifier of Getopt::Long is not supported, but you
could have options B<--foo> and B<--nofoo> and then do something like:

    $opt_foo = 0 if $opt_nofoo;

=item *

The C<@> argument specifier of Getopt::Long is not supported.  If you
want your values pushed into an array, you'll have to use nextOption
and do it yourself.

=back

=head1 LICENSE

Getopt::Mixed is distributed under the terms of the GNU General Public
License as published by the Free Software Foundation; either version
2, or (at your option) any later version.

This means it is distributed in the hope that it will be useful, but
I<without any warranty>; without even the implied warranty of
I<merchantability> or I<fitness for a particular purpose>.  See the
GNU General Public License for more details.

Since Perl scripts are only compiled at runtime, and simply calling
Getopt::Mixed does I<not> bring your program under the GPL, the only
real restriction is that you can't use Getopt::Mixed in an
binary-only distribution produced with C<dump> (unless you also
provide source code).

=head1 AUTHOR

Christopher J. Madsen E<lt>F<ac608@yfn.ysu.edu>E<gt>

Thanks are also due to Andreas Koenig for helping Getopt::Mixed
conform to the standards for Perl modules and for answering a bunch of
questions.  Any remaining deficiencies are my fault.

=cut
