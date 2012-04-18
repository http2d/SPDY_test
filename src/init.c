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

#include "common-internal.h"
#include "init.h"
#include "error_log.h"

#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/engine.h>

static bool             _initialized = false;
static pthread_mutex_t *locks        = NULL;
static size_t           locks_num    = 0;


/* Private low-level functions for OpenSSL
 */

static unsigned long
__get_thread_id (void)
{
        return (unsigned long) pthread_self();
}


static void
__lock_thread (int mode, int n, const char *file, int line)
{
        UNUSED (file);
        UNUSED (line);

        if (mode & CRYPTO_LOCK) {
                pthread_mutex_lock (&locks[n]);
        } else {
                pthread_mutex_unlock (&locks[n]);
        }
}


ret_t
http2d_init (void)
{
	if (_initialized)
		return ret_ok;

        /* Init OpenSSL
         */
        OPENSSL_config (NULL);
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        /* Ensure PRNG has been seeded with enough data
         */
        if (RAND_status() == 0) {
                LOG_WARNING_S (HTTP2D_ERROR_SSL_NO_ENTROPY);
        }

        /* Init concurrency related stuff
         */
        if ((CRYPTO_get_id_callback()      == NULL) &&
            (CRYPTO_get_locking_callback() == NULL))
        {
                cuint_t n;

                CRYPTO_set_id_callback (__get_thread_id);
                CRYPTO_set_locking_callback (__lock_thread);

                locks_num = CRYPTO_num_locks();
                locks     = malloc (locks_num * sizeof(*locks));

                for (n = 0; n < locks_num; n++) {
                        HTTP2D_MUTEX_INIT (&locks[n], NULL);
                }
        }

	/* Engines
	 */
        ENGINE_load_builtin_engines();
        OpenSSL_add_all_algorithms();

        ENGINE *e = ENGINE_by_id("pkcs11");
        while (e != NULL) {
                if(! ENGINE_init(e)) {
                        ENGINE_free (e);
                        LOG_CRITICAL_S (HTTP2D_ERROR_SSL_PKCS11);
                        break;
                }

                if(! ENGINE_set_default(e, ENGINE_METHOD_ALL)) {
                        ENGINE_free (e);
                        LOG_CRITICAL_S (HTTP2D_ERROR_SSL_DEFAULTS);
                        break;
                }

                ENGINE_finish(e);
                ENGINE_free(e);
                break;
        }

	return ret_ok;
}
