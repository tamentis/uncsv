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


// Given the following MAXLINELEN, a field once quoted could be up to twice its
// original size if all the MAXLINELEN bytes are double quotes. Also, add two
// bytes for the quotes themselves.
#define MAXLINELEN 4096
#define MAXFIELDLEN MAXLINELEN * 2 + 2


extern char delimiter;
int start_of_line = 1;


void
usage(void)
{
	printf("usage: csv [-Vh] [-d delimiter] [file ...]\n");
	exit(100);
}


void
handle_field(char *c)
{
	char buf[MAXFIELDLEN] = "\"", *cur;
	char last = '\0';
	int offset = 1;
	int quoted = 0;

	if (start_of_line == 0) {
		putchar(',');
	}

	cur = buf + 1;

	while (*c != '\0') {
		if (*c == '"') {
			*(cur++) = '"';
			quoted = 1;
		}

		last = *c;
		*(cur++) = *(c++);
	}

	if (buf[1] == ' ' || buf[1] == '\t' || last == ' ' || last == '\t') {
		quoted = 1;
	}

	if (quoted) {
		offset = 0;
		*(cur++) = '"';
	}

	*cur = '\0';

	printf("%s", buf + offset);
	start_of_line = 0;
}


/*
 * Given a file pointer, convert all the characters one by one to a delimited
 * stream (non-quoted, non-escaped).
 */
int
convert_from_fp(FILE *fp)
{
	char c;
	char line[MAXLINELEN], *f, *cur;

	for (;;) {
		if (fgets(line, sizeof(line), fp) == NULL) {
			if (ferror(fp) != 0) {
				return -1;
			} else if (feof(fp) != 0) {
				break;
			}
		}

		cur = line;

		for (;;) {
			/* Find delimiter. */
			f = strchr(cur, delimiter);
			if (f != NULL) {
				*f = '\0';
				handle_field(cur);
				cur = f + 1;
				continue;
			}

			/*
			 * End of line, take care of the field, then replicate
			 * whatever end-of-line non-sense was already in place.
			 */
			f = strpbrk(cur, "\r\n");
			if (f != NULL) {
				c = *f;
				*f = '\0';
				handle_field(cur);
				*f = c;
				printf("%s", f);
				start_of_line = 1;
				break;
			}

			/*
			 * End of file situation. Take whatever is left.
			 */
			handle_field(cur);
			break;
		}
	}

	return 0;
}
