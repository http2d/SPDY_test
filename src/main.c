/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* All files in http2d are Copyright (C) 2012 Alvaro Lopez Ortega.
 *
 *   Authors:
 *     * Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "server.h"
#include "common-internal.h"
#include "trace.h"
#include "init.h"

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else
# include "getopt/getopt.h"
#endif

#define EXIT_OK    0
#define EXIT_ERROR 1

/* Notices
 */
#define APP_NAME        \
	"HTTP2d"

#define APP_COPY_NOTICE \
	"Written by Alvaro Lopez Ortega <alvaro@alobbs.com>\n\n"                       \
	"Copyright (C) 2012 Alvaro Lopez Ortega.\n"                                    \
	"This is free software; see the source for copying conditions.  There is NO\n" \
	"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"

#define DEFAULT_CONFIG_FILE HTTP2D_CONFDIR "/http2d.conf"


static http2d_buffer_t  config_file   = HTTP2D_BUF_INIT;
static http2d_buffer_t  document_root = HTTP2D_BUF_INIT;
static bool             daemon_mode   = false;
static bool             just_test     = false;
static bool             print_modules = false;
static bool             port_set      = false;
static cuint_t          port          = 80;


static void
print_help (void)
{
	printf (APP_NAME "\n"
		"Usage: http2d [options]\n\n"
		"  -h,       --help                  Print this help\n"
		"  -V,       --version               Print version and exit\n"
		"  -t,       --test                  Just test configuration file\n"
		"  -d,       --detach                Detach from the console\n"
		"  -C<PATH>, --config=<PATH>         Configuration file\n"
		"  -p<NUM>,  --port=<NUM>            TCP port number\n"
		"  -r<PATH>, --documentroot=<PATH>   Server directory content\n"
		"  -i,       --print-server-info     Print server technical information\n"
		"  -v,       --valgrind              Execute the worker process under valgrind\n\n"
		"Report bugs to " PACKAGE_BUGREPORT "\n");
}


static ret_t
process_parameters (int argc, char **argv)
{
	int c;

	http2d_buffer_add_str (&config_file, DEFAULT_CONFIG_FILE);

	/* NOTE 1: If any of these parameters change, main.c may need
	 * to be updated.
	 *
	 * NOTE 2: The -v / --valgrind parameter is handled by main.c.
	 */
	struct option long_options[] = {
		{"help",              no_argument,       NULL, 'h'},
		{"version",           no_argument,       NULL, 'V'},
		{"detach",            no_argument,       NULL, 'd'},
		{"test",              no_argument,       NULL, 't'},
		{"print-server-info", no_argument,       NULL, 'i'},
		{"port",              required_argument, NULL, 'p'},
		{"documentroot",      required_argument, NULL, 'r'},
		{"config",            required_argument, NULL, 'C'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "hVdtiap:r:C:", long_options, NULL)) != -1) {
		switch(c) {
		case 'C':
			http2d_buffer_mrproper (&config_file);
			http2d_buffer_add      (&config_file, optarg, strlen(optarg));
			break;
		case 'd':
			daemon_mode = true;
			break;
		case 'r':
			http2d_buffer_mrproper (&document_root);
			http2d_buffer_add      (&document_root, optarg, strlen(optarg));
			break;
		case 'p':
			port = atoi(optarg);
			port_set = true;
			break;
		case 't':
			just_test = true;
			break;
		case 'i':
			print_modules = true;
			break;
		case 'V':
			printf (APP_NAME " " PACKAGE_VERSION "\n" APP_COPY_NOTICE);
			return ret_eof;
		case 'h':
		case '?':
		default:
			print_help();
			return ret_eof;
		}
	}

	/* Check for trailing parameters
	 */
	for (c = optind; c < argc; c++) {
		if ((argv[c] != NULL) && (strlen(argv[c]) > 0)) {
			print_help();
			return ret_eof;
		}
	}

	return ret_ok;
}


int
main (int argc, char *argv[])
{
	ret_t           ret;
	http2d_server_t srv;

	http2d_init();

	ret = process_parameters (argc, argv);
	if (ret != ret_ok)
		return EXIT_ERROR;

	http2d_trace_init();

	ret = http2d_server_init (&srv);
	if (ret != ret_ok)
		return EXIT_ERROR;

	ret = http2d_server_read_conf (&srv, &config_file);
	if (ret != ret_ok)
		return EXIT_ERROR;

	ret = http2d_server_initialize (&srv);
	if (ret != ret_ok)
		return EXIT_ERROR;

	http2d_server_run (&srv);

	SHOULDNT_HAPPEN;
	return EXIT_OK;
}

