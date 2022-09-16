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
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define XPIPE_VERSION	"2.0"
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
void *getNextByteChunk();
void *getNextChunk(regex_t, char *);
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
				/* NOTREACHED */
			}
			if (*pattern == '*') {
				errx(EXIT_FAILURE, "invalid wildcard at beginning of pattern");
				/* NOTREACHED */
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
		/* If the command fails, we'd get SIGPIPE upon
		 * write(2), so let's ignore that. */
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
			err(EXIT_FAILURE, "unable to ignore SIGPIPE");
			/* NOTREACHED */
		}
		(void)close(ipc[0]);
		int count = strlen(data);
		if (write(ipc[1], data, count) != count) {
			if (errno != EPIPE) {
				err(EXIT_FAILURE, "unable to write to pipe");
				/* NOTREACHED */
			}
		}
		(void)close(ipc[1]);
		if (signal(SIGPIPE, SIG_DFL) == SIG_ERR) {
			err(EXIT_FAILURE, "unable to restore SIGPIPE");
			/* NOTREACHED */
		}
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
	void *chunk;
	char *rest;

	regex_t preg;

	if (pattern) {
		if (regcomp(&preg, pattern, REG_EXTENDED | REG_NEWLINE) != 0) {
			errx(EXIT_FAILURE, "Invalid pattern");
			/* NOTREACHED */
		}

		if ((rest = malloc(BUFSIZ)) == NULL) {
			errx(EXIT_FAILURE, "Unable to malloc");
			/* NOTREACHED */
		}
	}

	for (;;) {
		if (bFlag) {
			chunk = getNextByteChunk();
		} else if (nFlag) {
			chunk = getNextChunk(preg, NULL);
		} else if (pattern) {
			chunk = getNextChunk(preg, rest);
		}

		if (chunk == NULL) {
			break;
		}
		execCommand(argv, argc, num, chunk);
		if (chunk) {
			free(chunk);
		}
		num++;
	}

	if (pattern) {
		regfree(&preg);
		if (rest) {
			free(rest);
		}
	}
}

void *
getNextChunk(regex_t preg, char *rest) {
	int lcount = 0, matched = 0, total = 0;

	size_t nmatch = 1;
	regmatch_t pmatch[1];

	char buf[BUFSIZ];
	bzero(buf, BUFSIZ);

	int dlen = BUFSIZ;
	int rlen = 0;
	if (rest) {
		rlen = strlen(rest);
		if (rlen > dlen) {
			dlen = rlen;
		}
	}

	char *data;
	if ((data = malloc(dlen)) == NULL) {
		errx(EXIT_FAILURE, "Unable to malloc data");
		/* NOTREACHED */
	}
	bzero(data, dlen);

	if (rest) {
		(void)strlcpy(data, rest, dlen);
		total += rlen;
		bzero(rest, rlen);

		if (regexec(&preg, data, nmatch, pmatch, REG_NOTBOL | REG_NOTEOL) == 0) {
			int bytes = (int)pmatch[0].rm_eo + 1;
			(void)strlcpy(rest, &data[bytes], rlen);
			return data;
		}
	}

	int plen = 0;
       	if (pattern) {
		plen = strlen(pattern);
	}

	char *line = NULL;
	size_t lsize = 0;
	int n = 0;

	while ((n = getline(&line, &lsize, stdin)) != -1) {
		lcount++;
		int bytes = n + 1; /* plus NUL */
		if (pattern &&
			(regexec(&preg, line, nmatch, pmatch, REG_NOTEOL) == 0)) {

			matched = 1;
			bytes = (int)pmatch[0].rm_eo;

			/* If we matched an EOL, increment so we add it to the
			 * returned data instead of rest. */
			if ((pattern[plen - 1] == '$') &&
					/* pattern was '$' */
					((plen == 1) ||
					 /* or it was _not_ '[...]\$' */
					 ((strlen(pattern) > 1) &&
					  (pattern[strlen(pattern) - 2] != '\\')))) {
				bytes++;
			}

			if (bytes < n) {
				if (bytes > BUFSIZ) {
					if ((rest = realloc(data, BUFSIZ + bytes)) == NULL) {
						errx(EXIT_FAILURE, "Unable to realloc rest");
						/* NOTREACHED */
					}
				}
				(void)strlcpy(rest, &line[bytes], BUFSIZ);
			}
		}

		if ((total + bytes + 1) > BUFSIZ) {
			if ((data = realloc(data, total + bytes)) == NULL) {
				errx(EXIT_FAILURE, "Unable to realloc data");
				/* NOTREACHED */
			}
		}
		(void)strlcat(data, line, total + bytes + 1);
		total = strlen(data);

		if (pattern && matched) {
			return data;
		}

		if (nFlag && (lcount == lineCount)) {
			break;
		}
	}

	if (ferror(stdin)) {
		errx(EXIT_FAILURE, "Unable to readline");
		/* NOTREACHED */
	}
	free(line);

	if ((total == 0) ||
		(IFlag && (pattern && !matched)) ||
		(IFlag && nFlag && (lcount < lineCount))) {
		if (data) {
			free(data);
		}
		return NULL;
	}

	/* If we get here, we have an incomplete chunk at EOF. */
	return data;
}

void *
getNextByteChunk() {
	int n;

	char *buf;
	if ((buf = malloc(byteCount)) == NULL) {
		errx(EXIT_FAILURE, "Unable to malloc buffer");
		/* NOTREACHED */
	}
	bzero(buf, byteCount);

	if ((n = read(STDIN_FILENO, buf, byteCount)) < 0) {
		errx(EXIT_FAILURE, "Unable to read data");
		/* NOTREACHED */
	}

	if ((n == 0) || ((n < byteCount) && IFlag)) {
		return NULL;
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
