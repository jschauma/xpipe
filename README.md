Quick Summary
=============

The xpipe command reads input from stdin and splits it
by the given number of bytes, lines, or if matching
the given pattern.  It then invokes the given utility
repeatedly, feeding it the generated data chunks as
input.

You can think of it as a Unix love-child of the
split(1), tee(1), and xargs(1) commands.

It's usefulness might best be illustrated by an
example.  Suppose you have a file 'certs.pem'
containing a number of x509 certificates in PEM
format, and you wish to extract e.g., the subject and
validity dates from each.

The openssl s\_client(1) utility can only accept a
single certificate at a time, so you'll have to
first split the input into individual files containing
exactly one cert, then repeatedly run the s\_client(1)
command against each file.

And, let's be honest, you probably have to google how
to use sed(1) or awk(1) to extract subsequent blocks
from a flip-flop pattern.

xpipe(1) can do the job for you in a single command:

```
$ <certs.pem xpipe -p '^-----END CERTIFICATE-----$' \
	openssl x509 -noout -subject -dates
notBefore=Aug 13 00:29:00 1998 GMT
notAfter=Aug 13 23:59:00 2018 GMT
subject= /C=US/O=GTE Corporation/OU=GTE CyberTrust Solutions, Inc./CN=GTE CyberTrust Global Root
notBefore=Aug  1 00:00:00 1996 GMT
notAfter=Dec 31 23:59:59 2020 GMT
subject= /C=ZA/ST=Western Cape/L=Cape Town/O=Thawte Consulting cc/OU=Certification Services Division/CN=Thawte Server CA/emailAddress=server-certs@thawte.com
notBefore=Aug  1 00:00:00 1996 GMT
notAfter=Dec 31 23:59:59 2020 GMT
subject= /C=ZA/ST=Western Cape/L=Cape Town/O=Thawte Consulting cc/OU=Certification Services Division/CN=Thawte Premium Server CA/emailAddress=premium-server@thawte.com
[...]
```

For more details and examples, please see the
[manual page](https://github.com/jschauma/xpipe/blob/master/doc/xpipe.1.txt).

Installation
============

To install the command and manual page somewhere
convenient, run `make install`; the Makefile defaults
to '/usr/local' but you can change the PREFIX:

```
$ make PREFIX=~ install
```

Platforms
=========

xpipe(1) was developed on a NetBSD 8.0 system.  It was
tested and confirmed to build and pass all tests on:

- NetBSD 8.0
- macOS 10.15.2
- RHEL 6.10
- RHEL 7.6

If you run into build/test issues, please [email
me](mailto:jschauma@netmeister.org).

---

```
NAME
     xpipe -- split input and feed it into the given utility

SYNOPSIS
     xpipe [-Ic] [-J replstr] [-b bytes] [-n lines] [-p pattern]
	   [utility [argument ...]]

DESCRIPTION
     The xpipe command reads input from stdin and splits it by the given num-
     ber of bytes, lines, or if matching the given pattern.  It then invokes
     the given utility repeatedly, feeding it the generated data chunks as
     input.

OPTIONS
     The following options are supported by xpipe:

     -I		 Do not write incomplete data.

     -J replstr	 When constructing the command to execute, replace the occur-
		 rence of replstr with the integer representing the number of
		 invocation performed.

     -b num	 Split input every num bytes.

     -c		 Continue even if utility failed.

     -n num	 Split input every num lines.

     -p pattern	 Split input by the given pattern.  See section PATTERNS for
		 details.

DETAILS
     xpipe conceptually combines some of the functionality of the split(1),
     tee(1), and xargs(1) utilities.  That is, it allows for repeated execu-
     tion of the given utitity, but unlike xargs(1), xpipe allows you to split
     the input and pipe it into the utility rather than passing it as argu-
     ments.

     This allows you to process input either in byte-, line-, or pattern-sepa-
     rated chunks and pipe those chunks into the same tool without having to
     manually split the input or store data in temporary files.

     Input processing is done sequentially: data is read up until the end of
     the given pattern (or until the given number of bytes or lines has been
     encountered); when that condition is encountered, utility is invoked and
     the data chunk is written into a pipe to that process; xpipe then waits
     for the process to complete, upon which it continues to read input.

     If EOF is reached before the given condition is met, then xpipe will exe-
     cute utility and pipe into it whatever data it encountered so far.	 This
     can be avoided by passing the -I flag, which leads to xpipe discarding
     any partial data without invoking utility (again or at all).

PATTERNS
     When the -p flag is specified, xpipe will split input by the given pat-
     tern with each chunk including the pattern as the last bytes.

     A pattern is, by and large, a simple, fixed string.  That is, you cannot
     specify regular expressions or shell globs; all characters or bytes are
     matched verbatim with the exception of the following:

     ^	  If the '^' character is specified at the beginning of the pattern,
	  then it will anchor the pattern at the beginning of the line.

	  To match a literal '^' at the beginning of the line, escape the
	  character using '^\^'.  For example, to match a line beginning with
	  the string '^foo', you'd use the pattern '^\^foo'.

     $	  If the '$' character is specified at the end of the pattern (i.e.,
	  as the last character), then it will anchor the pattern at the end
	  of the line.

	  To match a literal '$' at the end of the line, escape the character
	  using '\$$'.	For example, to match a line ending with 'foo', you'd
	  use the pattern 'foo$'; To match a line ending with 'foo$', you'd
	  use 'foo\$$'.

     \n	  This will match a new line (ASCII 10).  The difference to '$' is
	  that you can match a \n anywhere in your pattern, while a '$' will
	  only match at the end of he line.

	  For example, to match a line ending in 'foo' followed by a line
	  beginning with 'bar', you'd use the pattern 'foo\nbar'.

     \t	  This will match a tab character (ASCII 9).

	  For example, to match a line beginning with two tabs followed by the
	  string 'foo', you'd use the pattern '^\t\tfoo'.

COMMAND INVOCATION
     xpipe will invoke the given utility with any subsequent arguments.	 If
     the -J flag is specified, then the given replstr in any of the arguments
     (including the utility itself) will be replaced with the number of the
     invocation.

     Since I/O redirection is processed by the invoking shell, you'd have to
     invoke a new shell to allow for redirection to e.g. a per-invocation out-
     put file.

     For example, to split the input into unique files ending in the given
     pattern:

	   <input xpipe -J % -p pattern /bin/sh -c "cat >%.out"

EXAMPLES
     The following examples illustrate common usage of this tool.

     To count the number of words in each paragraph of 'Don Quijote':

	   curl -s https://www.gutenberg.org/cache/epub/2000/pg2000.txt | \
		   tr -d '^M' | \
		   xpipe -p '^$' wc -w

     To extract the subjects of all certificates in a standard PEM formatted
     x.509 trust bundle:

	   <certs.pem xpipe -p '^-----END CERTIFICATE-----$' \
		   openssl x509 -noout -subject

     To split a large, uncompressed log file into multiple, compressed files,
     named 1.gz, 2.gz, ... n.gz:

	   <logfile xpipe -n 1000 -J % /bin/sh -c "gzip >%.gz"

EXIT STATUS
     The xpipe command exits with a value of 0 if no error occurs.

     If the -c flag is passed, xpipe will exit with an integer value repre-
     senting the total number of invocations of the utility that failed.

     If the -c was not passed, then any failure in the execution of the
     utility leads to the termination of xpipe.	 That is, if the utility can-
     not be found, xpipe exits with a value of 127; if utility cannot be exe-
     cuted, xpipe exits with a value of 126; if utility terminated because of
     a signal, xpipe exits with a value of 125.	 If any other error occurs,
     xpipe exits with a value of 1.

SEE ALSO
     split(1), tee(1), xargs(1)

HISTORY
     xpipe was originally written by Jan Schaumann <jschauma@netmeister.org>
     in January 2020.

BUGS
     Please file bugs and feature requests by emailing the author.
```
