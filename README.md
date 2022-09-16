Quick Summary
=============

The xpipe command reads input from stdin and splits it
by the given number of bytes, lines, or if matching
the given pattern.  It then invokes the given utility
repeatedly, feeding it the generated data chunks as
input.

You can think of it as a Unix love-child of the
split(1), tee(1), and xargs(1) commands.

It is generally most useful when you have input data
that you want to feed in chunks one at a time into the
same command.  So for example, consider the following
conceptual sequence of commands:

```
head -20 input | command
head -40 input | tail -20 | command
head -60 input | tail -20 | command
```

or

```
split -n 100 ../input
for f in *; do
        command <$f
done
```

These might be replaced with single line invocations
of `xpipe(1)`.  This is particularly useful when you
need to extract data from the input in more complex
ways:

Suppose you have a file 'certs.pem' containing a
number of x509 certificates in PEM format, and you
wish to extract e.g., the subject and validity dates
from each.

The openssl x509(1) utility can only accept a
single certificate at a time, so you'll have to
first split the input into individual files containing
exactly one cert, then repeatedly run the x509(1)
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

If you are using macOS with
[Homebrew](https://brew.sh/), then you should be able
to simply run this command:

```
$ brew install xpipe
```

_Note_: on Linux systems, you will need
`strlcat(3)/strlcpy(3)`.  These are provided by
`libbsd`, so you will need to install `libbsd` and
possibly `libbsd-devel` or equivalent, depending on
your Linux distribution.

Platforms
=========

`xpipe(1)` was developed on a NetBSD 8.0 system.  It was
tested and confirmed to build and pass all tests on:

- NetBSD 8.0
- macOS 12.6
- Fedora 36

If you run into build/test issues, please [email
me](mailto:jschauma@netmeister.org).

---

```
NAME
     xpipe - split input and feed it into the given utility

SYNOPSIS
     xpipe [-IVc] [-J replstr] [-b bytes] [-n lines] [-p pattern]
	   [utility [argument ...]]

DESCRIPTION
     The xpipe command reads input from stdin and splits it by the given number
     of bytes, lines, or if matching the given pattern.	 It then invokes the
     given utility repeatedly, feeding it the generated data chunks as input.

OPTIONS
     The following options are supported by xpipe:

     -I		 Do not write incomplete data.

     -J replstr	 When constructing the command to execute, replace the
		 occurrence of replstr with the integer representing the number
		 of invocation performed.

     -V		 Print version number and exit.

     -b num	 Split input every num bytes.

     -c		 Continue even if utility failed.

     -n num	 Split input every num lines.

     -p pattern	 Split input by the given pattern.  See section PATTERNS for
		 details.

DETAILS
     xpipe conceptually combines some of the functionality of the split(1),
     tee(1), and xargs(1) utilities.  That is, it allows for repeated execution
     of the given utitity, but unlike xargs(1), xpipe allows you to split the
     input and pipe it into the utility rather than passing it as arguments.

     This allows you to process input either in byte-, line-, or pattern-
     separated chunks and pipe those chunks into the same tool without having to
     manually split the input or store data in temporary files.

     Input processing is done sequentially: data is read up until the end of the
     given pattern (or until the given number of bytes or lines has been
     encountered); when that condition is encountered, utility is invoked and
     the data chunk is written into a pipe to that process; xpipe then waits for
     the process to complete, upon which it continues to read input.

     If EOF is reached before the given condition is met, then xpipe will
     execute utility and pipe into it whatever data it encountered so far.  This
     can be avoided by passing the -I flag, which leads to xpipe discarding any
     partial data without invoking utility (again or at all).

PATTERNS
     When the -p flag is specified, xpipe will split input by the given pattern
     with each chunk including the pattern as the last bytes.

     A pattern is a common extended regular expression as described in
     re_format(7), and is matched against each line of the input data.

COMMAND INVOCATION
     xpipe will invoke the given utility with any subsequent arguments.	 If the
     -J flag is specified, then the given replstr in any of the arguments
     (including the utility itself) will be replaced with the number of the
     invocation.

     Since I/O redirection is processed by the invoking shell, you'd have to
     invoke a new shell to allow for redirection to e.g. a per-invocation output
     file.

     For example, to split the input into unique files ending in the given
     pattern:

	   <input xpipe -J % -p pattern /bin/sh -c "cat >%.out"

EXAMPLES
     The following examples illustrate common usage of this tool.

     Suppose you have a large number of files in your current directory and wish
     to create tar archives containing 1000 files each:

	   $ ls | wc -l
	   100000
	   $ ls | xpipe -n 1000 -J % tar zcf ../archives/%.tgz --files-from -
	   $ ls ../archives/*.tgz | wc -l
	   100

     To split a large, uncompressed log file into multiple, compressed files,
     named 1.gz, 2.gz, ... n.gz, you could use split(1), and then iterate over
     each file and compress it; with xpipe, this becomes a one-liner:

	   $ <logfile xpipe -n 1000 -J % /bin/sh -c "gzip >%.gz"

     To extract the subjects of all certificates in a standard PEM formatted
     x.509 trust bundle:

	   $ <certs.pem xpipe -p '^-----END CERTIFICATE-----$' \
		   openssl x509 -noout -subject

EXIT STATUS
     The xpipe command exits with a value of 0 if no error occurs.

     If the -c flag is passed, xpipe will exit with an integer value
     representing the total number of invocations of the utility that failed.

     If the -c was not passed, then any failure in the execution of the utility
     leads to the termination of xpipe.	 That is, if the utility cannot be
     found, xpipe exits with a value of 127; if utility cannot be executed,
     xpipe exits with a value of 126; if utility terminated because of a signal,
     xpipe exits with a value of 125.  If any other error occurs, xpipe exits
     with a value of 1.

SEE ALSO
     split(1), tee(1), xargs(1), re_format(7)

HISTORY
     xpipe was originally written by Jan Schaumann <jschauma@netmeister.org> in
     January 2020.

BUGS
     Please file bugs and feature requests by emailing the author.
```
