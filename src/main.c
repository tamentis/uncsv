/*
 * Copyright (c) 2013 Bertrand Janin <b@janin.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>


#define READ_BUFFER_SIZE 1024

#define CONVERT_OUTCOME_ERROR -1
#define CONVERT_OUTCOME_KEEP 0
#define CONVERT_OUTCOME_DROP 1

#define ERR_BAD_DELIMITER "error: delimiter should be one-byte only " \
			  "or one of: \\t, \\n, \\0"


char delimiter = '|';
char previous = '\0';
char quoted = 0;
int possible_quoted_quote = 0;


void
usage(void)
{
	printf("usage: uncsv [-Vh] [-d delimiter] [file ...]\n");
	exit(100);
}


int
convert_char(char *c)
{
	if (*c == delimiter) {
		errx(100, "error: found output delimiter in input data");
	}

	if (possible_quoted_quote == 1) {
		possible_quoted_quote = 0;
		if (*c == '"')
			return CONVERT_OUTCOME_KEEP;
		quoted = 0;
	}

	/*
	 * We're not working on a quoted field, finding a double-quote means
	 * we are going in quoted mode.
	 */
	if (quoted == 0 && *c == '"') {
		quoted = 1;
		return CONVERT_OUTCOME_DROP;
	}
	if (quoted == 0 && *c == ',') {
		*c = delimiter;
		return CONVERT_OUTCOME_KEEP;
	}

	/*
	 * We are in the middle of a quoted field and we find another quote,
	 * two possible outcome: this is the end of the field, this is a quoted
	 * quote.
	 */
	if (quoted == 1 && *c == '"') {
		possible_quoted_quote = 1;
		return CONVERT_OUTCOME_DROP;
	}

	/*
	 * Replace new lines by spaces, that's the simplest way to keep the
	 * output awk-friendly.
	 */
	if (quoted == 1 && (*c == '\r' || *c == '\n')) {
		*c = ' ';
		return CONVERT_OUTCOME_KEEP;
	}

	previous = *c;

	return CONVERT_OUTCOME_KEEP;
}


/*
 * Given a file pointer, convert all the characters one by one to a delimited
 * stream (non-quoted, non-escaped).
 */
int
convert_from_fp(FILE *fp)
{
	int i, retcode;
	char buf[READ_BUFFER_SIZE], c;
	size_t s;

	for (;;) {
		s = fread(buf, 1, sizeof(buf), fp);
		if (ferror(fp) != 0) {
			return -1;
		}

		for (i = 0; i < s; i++) {
			c = buf[i];
			retcode = convert_char(&c);
			if (retcode == CONVERT_OUTCOME_ERROR) {
				return -1;
			}

			if (retcode == CONVERT_OUTCOME_DROP)
				continue;

			retcode = putchar(c);
			if (retcode == EOF) {
				return -1;
			}
		}

		if (feof(fp) != 0) {
			break;
		}
	}

	return 0;
}


int
main(int argc, char **argv)
{
	int opt, i, errcode;
	FILE *fp;
	extern char *optarg;
	extern int optind;

	while ((opt = getopt(argc, argv, "d:Vh")) != -1) {
		switch (opt) {
		case 'd':
			i = strlen(optarg);
			if (i == 2 && optarg[0] == '\\') {
				switch (optarg[1]) {
				case 't':
					delimiter = '\t';
					break;
				case 'n':
					delimiter = '\n';
					break;
				case '0':
					delimiter = '\0';
					break;
				default:
					errx(100, ERR_BAD_DELIMITER);
				}
			} else if (i == 1) {
				delimiter = optarg[0];
			} else {
				errx(100, ERR_BAD_DELIMITER);
			}
			break;
		case 'V':
			fprintf(stderr, "uncsv-" UNCSV_VERSION "\n");
			break;
		default:
			usage();
			return 100;
		}
	}

	argc -= optind;
	argv += optind;

	/* No file was provided on the command line, process stdin. */
	if (argc == 0) {
		errcode = convert_from_fp(stdin);
		if (errcode == -1) {
			errx(100, "error reading stdin");
		}
		return 0;
	}

	/* Process all the provided files in order. */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-") == 0) {
			convert_from_fp(stdin);
			continue;
		}

		fp = fopen(argv[i], "rb");
		if (fp == NULL) {
			err(100, "error opening %s", argv[i]);
		}

		errcode = convert_from_fp(fp);
		if (errcode == -1) {
			errx(100, "error reading %s", argv[i]);
		}
		fclose(fp);
	}

	return 0;
}
