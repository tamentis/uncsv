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


extern char delimiter;
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
			previous = c;

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
