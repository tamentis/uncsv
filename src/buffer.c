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

/*
 * Implement a simple write buffer to avoid constant calls to putchar/printf
 * with relatively small amounts of data.
 */

#include <stdio.h>
#include <string.h>
#include <err.h>

#include "uncsv.h"


#define WRITE_BUFFER_SIZE 4096


char buffer[WRITE_BUFFER_SIZE];
int offset = 0;


/*
 * Flush the buffer to stdout.
 */
void
flush_output()
{
	int retcode;

	if (offset == 0)
		return;

	retcode = fwrite(buffer, sizeof(char), offset, stdout);
	if (retcode < offset) {
		err(100, "flush_output:write error");
	}
	offset = 0;
}


/*
 * Write a single character to the output buffer.
 *
 * If the buffer reach its maximum size upon adding this character, the buffer
 * is flushed.
 */
void
write_character(char c)
{
	buffer[offset++] = c;

	if (offset == WRITE_BUFFER_SIZE) {
		flush_output();
	}
}


/*
 * Write up to 'len' characters from 's' to the buffer, regardless of
 * NUL-bytes.
 *
 * If the input would make the buffer overflow, the buffer is flushed, if the
 * input is larger than the write buffer, it is written directly to stdout
 * after the buffer is flushed.
 */
void
write_string(char *s, size_t len)
{
	int retcode;

	if (offset + len > WRITE_BUFFER_SIZE) {
		flush_output();

		if (len > WRITE_BUFFER_SIZE) {
			retcode = fwrite(s, sizeof(char), len, stdout);
			if (retcode < offset) {
				err(100, "write_string:fwrite error");
			}
			return;
		}
	}

	memcpy(buffer + offset, s, len);
	offset += len;
}
