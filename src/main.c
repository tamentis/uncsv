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

#define ERR_BAD_DELIMITER "error: delimiter should be one-byte only " \
			  "or one of: \\t, \\n, \\0"

char delimiter = '|';
bool quote_for_space = false;

void usage(void);
int convert_from_fp(FILE *);


int
main(int argc, char **argv)
{
	int opt, i, errcode;
	FILE *fp;
	extern char *optarg;
	extern int optind;

	while ((opt = getopt(argc, argv, "sd:Vh")) != -1) {
		switch (opt) {
		case 's':
			quote_for_space = true;
			break;
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
			return 100;
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
