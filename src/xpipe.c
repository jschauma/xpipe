/*
 * Copyright (c) 2020, Jan Schaumann <jschauma@netmeister.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/wait.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define XPIPE_VERSION	"1.0"
extern char *__progname;

char *pattern, *replstr;

int IFlag = 0;
int JFlag = 0;
int bFlag = 0;
int cFlag = 0;
int nFlag = 0;
int byteCount = 0;
int lineCount = 0;
int execFailCount = 0;

char **utility;

void execCommand(char **, int, int, char *);
char *getNextChunk(int);
char *replaceNum(char *, int);
char **replaceNumInArgs(char **, int, int);
void usage();
void xpipe(char **, int);

#define EXIT_SIGNALLED  125
#define EXEC_FAILURE    126
#define EXEC_NOT_FOUND  127

int
main(int argc, char **argv) {
	int ch;

	pattern = replstr = NULL;
	byteCount = lineCount = 0;

	/* GNU getopt(3) needs '+' to enable POSIXLY_CORRECT behavior. */
	while ((ch = getopt(argc, argv, "+IVJ:b:cn:p:")) != -1) {
		switch(ch) {
		case 'I':
			IFlag = 1;
			break;
		case 'J':
			JFlag = 1;
			replstr = optarg;
			if (replstr != NULL && *replstr == '\0') {
				errx(EXIT_FAILURE, "replstr may not be empty");
			}
			break;
		case 'V':
			printf("%s version %s\n", __progname, XPIPE_VERSION);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
		case 'b':
			bFlag = 1;
			lineCount = nFlag = 0;
			pattern = NULL;

			byteCount = atoi(optarg);
			if (byteCount <= 0) {
				errx(EXIT_FAILURE, "byte count must be > 0");
			}
			break;
		case 'c':
			cFlag = 1;
			break;
		case 'n':
			nFlag = 1;
			byteCount = bFlag = 0;
			pattern = NULL;

			lineCount = atoi(optarg);
			if (lineCount <= 0) {
				errx(EXIT_FAILURE, "line count must be > 0");
			}
			break;
		case 'p':
			byteCount = 0;
			lineCount = 0;

			if ((pattern = strdup(optarg)) == NULL) {
				err(EXIT_FAILURE, "unable to strdup");
				/* NOTREACHED */
			}
			if (pattern != NULL && *pattern == '\0') {
				errx(EXIT_FAILURE, "pattern may not be empty");
			}
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (!argc) {
		errx(EXIT_FAILURE, "you need to specify a utility to invoke");
		/* NOTREACHED */
	}

	if (!byteCount && !lineCount && (pattern == NULL)) {
		errx(EXIT_FAILURE, "you need to specify either '-b', '-n', or '-p'");
		/* NOTREACHED */
	}

	xpipe(argv, argc);

	return execFailCount;
}

void
execCommand(char **argv, int argc, int num, char *data) {
	int ipc[2];
	int e, i, pid, status;
	int freeLargv = 1;

	char **largv;

	if ((largv = replaceNumInArgs(argv, argc, num)) == NULL) {
		largv = argv;
		freeLargv = 0;
	}

	if (pipe(ipc) < 0) {
		err(EXIT_FAILURE, "unable to create pipe");
		/* NOTREACHED */
	}

	if ((pid = fork()) < 0) {
		errx(EXIT_FAILURE, "unable to fork");
		/* NOTREACHED */
	}

	if (pid == 0) {
		(void)close(ipc[1]);
		if (ipc[0] != STDIN_FILENO) {
			if (dup2(ipc[0], STDIN_FILENO) != STDIN_FILENO) {
				err(EXIT_FAILURE, "unable to dup2");
				/* NOTREACHED */
			}
		}
		execvp(largv[0], largv);
		err(errno, "unable to execute '%s'", largv[0]);
		/* NOTREACHED */
	} else {
		(void)close(ipc[0]);
		int count = strlen(data);
		if (write(ipc[1], data, count) != count) {
			err(EXIT_FAILURE, "unable to write to pipe");
			/* NOTREACHED */
		}
		(void)close(ipc[1]);
	}

	if (waitpid(pid, &status, 0) < 0) {
		err(EXIT_FAILURE, "unable to wait for pid %d", pid);
		/* NOTREACHED */
	}

	/* a lot like xargs(1) */
	if (WIFEXITED(status)) {
		e = WEXITSTATUS(status);
		if (e > 0) {
			if (cFlag) {
				execFailCount++;
			} else {
				e = (e == ENOENT) ? EXEC_NOT_FOUND : EXEC_FAILURE;
				exit(e);
				/* NOTREACHED */
			}
		}
	} else if (WIFSIGNALED(status)) {
		if (WTERMSIG(status) < NSIG) {
			warnx("%s terminated by SIG%s", largv[0], strsignal(WTERMSIG(status)));
		} else {
			warnx("%s terminated by signal %d", largv[0], WTERMSIG(status));
		}
		if (cFlag) {
			execFailCount++;
		} else {
			exit(EXIT_SIGNALLED);
			/* NOTREACHED */
		}
	}

	if (freeLargv) {
		for (i = 0; i < argc; i++) {
			free(largv[i]);
		}
		free(largv);
	}
}

void
usage() {
	(void)fprintf(stderr,
"Usage: %s [-IVc] [-J replstr] [-b bytes] [-n lines] [-p pattern]\n"
"             [utility [argument ...]]\n"
"    -I          don't write incomplete data.\n"
"    -J replstr  replace replstr with invocation number\n"
"    -V          print version number and exit\n"
"    -b num      split input every num bytes\n"
"    -c          continue even if utility fails\n"
"    -n num      split input every num lines\n"
"    -p pattern  split input by pattern\n"
, __progname);

}

void
xpipe(char **argv, int argc) {
	int num = 1;
	int last = '\n';

	for (;;) {
		char *nextChunk;
		nextChunk = getNextChunk(last);
		if ((nextChunk == NULL) || (strlen(nextChunk) == 0)) {
			break;
		}
		last = nextChunk[strlen(nextChunk)];
		execCommand(argv, argc, num, nextChunk);
		free(nextChunk);
		num++;
	}
}

char *
getNextChunk(int last) {
	int ch, bl, bsize, el, esc, plen, prevesc;
	int count, lcount, mcount;

	bl = count = el = esc = lcount = mcount = prevesc = 0;
	if (last == '\n') {
		bl = 1;
	}

	char *buf, *compare, *data;

	bsize = BUFSIZ;
	if ((data = calloc(bsize, sizeof(char))) == NULL) {
		err(EXIT_FAILURE, "Unable to allocate memory");
		/* NOTREACHED */
	}
	buf = data;

	compare = pattern;
	plen = pattern ? strlen(pattern) : 0;

	while ((ch = getchar()) != EOF) {
		count++;

		if (el) {
			el = 0;
			bl = 1;
		}
		if (ch == '\n') {
			lcount++;
			el = 1;
		}
		if (pattern) {
escapecheck:
			if (*compare == '\\') {
				esc = !esc;
				/* '\' at end of pattern */
				if (esc && (strlen(compare) == 1)) {
					esc = 0;
				}
				if (esc) {
					compare++;
					plen--;
				}
			}

			/* match and skip, so we then compare the next character */
			if (bl && !esc && (*compare == '^')) {
				compare++;
				mcount++;
				/* the new char may be an escape char, so
				 * we have to go back and check again */
				goto escapecheck;
			}

			/* an unescaped '^' not at the beginning of the line */
			if (!esc && !bl && (*compare == '^')) {
				goto nomatch;
			}

			/* We match as follows:
			 * - exact matches match
			 * - unescaped '$' at end of line matches
			 * - '\n' at end of line matches
			 * - '\t' matches tabs
			 */
			if ((ch == *compare) ||
				(!esc && el && (*compare == '$')) ||
				(esc && el && (*compare == 'n')) ||
				(esc && (ch == '\t') && (*compare == 't'))) {
				mcount++;
				compare++;
			} else {
nomatch:
				mcount = 0;
				compare = pattern;
				plen = strlen(pattern);
				if (!esc && prevesc) {
					prevesc = 0;
					goto escapecheck;
				}
			}

			if (esc) {
				prevesc = 1;
				esc = 0;
			}

		}
		bl = 0;

		if (count == bsize) {
			bsize += BUFSIZ;
			if ((buf = realloc(buf, bsize)) == NULL) {
				err(EXIT_FAILURE, "Unable to re-allocate memory");
				/* NOTREACHED */
			}
		}
		data = &buf[count-1];
		*data++ = ch;

		if (bFlag && (count == byteCount)) {
			break;
		} else if (nFlag && (lcount == lineCount)) {
			break;
		}
		if (pattern && (mcount == plen)) {
			break;
		}
	}
	*data = '\0';

	if (ferror(stdin)) {
		err(EXIT_FAILURE, "Unable to read data from stdin");
		/* NOTREACHED */
	}

	if (IFlag) {
	       if ((bFlag && (count != byteCount)) ||
			       (nFlag && (lcount != lineCount)) ||
			       (pattern && (mcount != plen))) {
			free(buf);
			buf = NULL;
	       }
	}

	return buf;
}

char **
replaceNumInArgs(char **argv, int argc, int num) {
	int i;
	char **largv;

	if (!JFlag) {
		return NULL;
	}

	if ((largv = calloc(argc + 1, sizeof(char *))) == NULL) {
		err(EXIT_FAILURE, "unable to calloc");
		/* NOTREACHED */
	}

	for (i = 0; argv[i] != NULL; i++) {
		largv[i] = replaceNum(argv[i], num);
	}
	return largv;
}

/* based on xargs/strnsubst.c */
char *
replaceNum(char *input, int num) {
	assert(input != NULL);
	assert(replstr != NULL);

	char *newstr, *numstr, *ptr;

	int ndigits = 1;
	if (num > 9) {
		ndigits = floor(log10(num)) + 1;
	}

	if ((numstr = calloc(ndigits, sizeof(char))) == NULL) {
		err(EXIT_FAILURE, "unable to calloc");
		/* NOTREACHED */
	}
	(void)snprintf(numstr, ndigits + 1, "%d", num);

	/* Not always exact, but always enough. */
	int newlen = strlen(input) + ndigits + 1;
	if ((newstr = malloc(newlen)) == NULL) {
		err(EXIT_FAILURE, "unable to malloc");
		/* NOTREACHED */
	}
	bzero(newstr, newlen);

	for (;;) {
		if ((ptr = strstr(input, replstr)) == NULL) {
			(void)strncat(newstr, input, strlen(input));
			break;
		}
		(void)strncat(newstr, input, ptr - input);
		(void)strncat(newstr, numstr, ndigits);
		input = ptr + strlen(replstr);
	}
	return newstr;
}
