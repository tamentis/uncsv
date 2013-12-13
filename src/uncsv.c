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
#include <stdbool.h>
#include <err.h>


#define READ_BUFFER_SIZE 4096

#define CONVERT_OUTCOME_ERROR -1
#define CONVERT_OUTCOME_KEEP 0
#define CONVERT_OUTCOME_DROP 1
#define CONVERT_OUTCOME_CARRIAGE_RETURN 2
#define CONVERT_OUTCOME_LINE_FEED 3

#define ERR_BAD_DELIMITER "error: delimiter should be one-byte only " \
			  "or one of: \\t, \\n, \\0"


extern char delimiter;
char previous = '\0';
bool quoted = false;
bool possible_quoted_quote = false;
extern char *r_replacement;
extern char *n_replacement;

void flush_output();
void write_character(char);
void write_string(char *, size_t);


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

	if (possible_quoted_quote) {
		possible_quoted_quote = false;
		if (*c == '"')
			return CONVERT_OUTCOME_KEEP;
		quoted = false;
	}

	/* Toggle quoted mode when finding a double-quote while. */
	if (!quoted && *c == '"') {
		quoted = true;
		return CONVERT_OUTCOME_DROP;
	}
	if (!quoted && *c == ',') {
		*c = delimiter;
		return CONVERT_OUTCOME_KEEP;
	}

	/*
	 * We are in the middle of a quoted field and we find another quote,
	 * two possible outcome: this is the end of the field, this is a quoted
	 * quote.
	 */
	if (quoted && *c == '"') {
		possible_quoted_quote = true;
		return CONVERT_OUTCOME_DROP;
	}

	/*
	 * These characters make any good line-based UNIX tools angry, give the
	 * user an option to convert them in a meaningful way.
	 */
	if (quoted && *c == '\r') {
		return CONVERT_OUTCOME_CARRIAGE_RETURN;
	}
	if (quoted && *c == '\n') {
		return CONVERT_OUTCOME_LINE_FEED;
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

			switch (retcode) {
			case CONVERT_OUTCOME_DROP:
				break;
			case CONVERT_OUTCOME_KEEP:
				write_character(c);
				break;
			case CONVERT_OUTCOME_CARRIAGE_RETURN:
				if (r_replacement == NULL) {
					write_character(' ');
				} else {
					write_string(r_replacement,
							strlen(r_replacement));
				}
				break;
			case CONVERT_OUTCOME_LINE_FEED:
				if (n_replacement == NULL) {
					write_character(' ');
				} else {
					write_string(n_replacement,
							strlen(n_replacement));
				}
				break;
			}
		}

		if (feof(fp) != 0) {
			break;
		}
	}

	flush_output();

	return 0;
}
