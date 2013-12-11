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


// Given the following MAXLINELEN, a field once quoted could be up to twice its
// original size if all the MAXLINELEN bytes are double quotes. Also, add two
// bytes for the quotes themselves.
#define MAXLINELEN 4096
#define MAXFIELDLEN MAXLINELEN * 2 + 2


extern char delimiter;
extern char quote_for_space;
bool start_of_line = true;


void flush_output();
void write_character(char);
void write_string(char *, size_t);


void
usage(void)
{
	printf("usage: csv [-Vh] [-d delimiter] [file ...]\n");
	exit(100);
}


/*
 * Quote/escape a single field.
 *
 * Add surrounding double-quotes on all fields containing a comma or a
 * doube-quote. Optionally quote the fields with heading/trailing spaces since
 * some sketchy implementations tend to delete them.
 *
 * The inner loop is implemented with memcpy/strchr because these two functions
 * are often implemented in assembly while a normal char-by-char implementation
 * would not. It was proven to be three times faster on OS X.
 */
void
convert_field(char *c, size_t len)
{
	static char buf[MAXFIELDLEN] = "\"";
	char *cur, *q;
	int offset = 1;
	bool quoted = false;
	size_t l;

	if (!start_of_line) {
		write_character(',');
	}

	cur = buf + 1;

	for (;;) {
		q = strchr(c, '"');
		if (q == NULL) {
			memcpy(cur, c, len);
			cur += len;
			break;
		}

		quoted = true;

		/* Copy everything up to the quote, including the quote. */
		l = q - c + 1;
		memcpy(cur, c, l);
		cur += l;
		c += l;

		/* Add the actual quote (after the escaping one). */
		*(cur++) = '"';

		len -= l;
	}

	if (quote_for_space) {
		if (buf[1] == ' ' || buf[1] == '\t' || buf[len] == ' '
				|| buf[len] == '\t') {
			quoted = true;
		}
	}

	if (quoted) {
		offset = 0;
		*(cur++) = '"';
	}

	write_string(buf + offset, cur - (buf + offset));
	start_of_line = false;
}


/*
 * Convert a single NUL-delimited line to CSV.
 *
 * Split the fields according to the delimiter and feed them to convert_field
 * for proper quoting/escaping then add a comma between each of them while
 * keeping the original end of line termination.
 */
int
convert_line(char *line)
{
	char c, *f, *cur;
	size_t fieldlen;

	cur = line;

	for (;;) {
		/* Find delimiter. */
		f = strchr(cur, delimiter);
		if (f != NULL) {
			fieldlen = f - cur;
			*f = '\0';
			convert_field(cur, fieldlen);
			cur = f + 1;
			continue;
		}

		/*
		 * End of line, take care of the field, then replicate
		 * whatever end-of-line non-sense was already in place.
		 */
		f = strpbrk(cur, "\r\n");
		if (f != NULL) {
			fieldlen = f - cur;
			c = *f;
			*f = '\0';
			convert_field(cur, fieldlen);
			*f = c;
			write_string(f, strlen(f));
			start_of_line = true;
			break;
		}

		/* End of file; take whatever is left. */
		convert_field(cur, strlen(cur));
		break;
	}

	return 0;
}


/*
 * Print a file pointer to script as CSV.
 *
 * Call convert_line() for each line in the opened file pointer.
 */
int
convert_from_fp(FILE *fp)
{
	int lineno = 1, retcode;
	char line[MAXLINELEN];

	for (;;) {
		if (fgets(line, sizeof(line), fp) == NULL) {
			if (ferror(fp) != 0) {
				return -1;
			} else if (feof(fp) != 0) {
				break;
			}
		}

		retcode = convert_line(line);
		if (retcode != 0) {
			errx(100, "error converting line %d", lineno);
		}

		lineno++;
	}

	flush_output();

	return 0;
}
