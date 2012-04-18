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
#include "virtual_server.h"
#include "connection.h"
#include "request.h"
#include "server.h"
#include "error_log.h"
#include "util.h"
#include "socket-ssl.h"

#define ENTRIES             "vserver"
#define SSL_DEFAULT_CIPHERS "HIGH:!ADH:!MD5"

ret_t
http2d_virtual_server_new  (http2d_virtual_server_t **vsrv,
			    void                     *srv)
{
	HTTP2D_NEW_STRUCT (n, virtual_server);

	n->priority                     = 0;
	n->srv                          = srv;
	n->ssl_context                  = NULL;
	n->ssl_cipher_server_preference = true;

	http2d_buffer_init (&n->name);
	http2d_buffer_init (&n->ssl_certificate_file);
	http2d_buffer_init (&n->ssl_certificate_key_file);
	http2d_buffer_init (&n->ssl_ca_list_file);

	http2d_buffer_init    (&n->ssl_ciphers);
	http2d_buffer_add_str (&n->ssl_ciphers, SSL_DEFAULT_CIPHERS);

	*vsrv = n;
	return ret_ok;
}


ret_t
http2d_virtual_server_free (http2d_virtual_server_t *vsrv)
{
	http2d_buffer_mrproper (&vsrv->name);
	http2d_buffer_mrproper (&vsrv->ssl_certificate_file);
	http2d_buffer_mrproper (&vsrv->ssl_certificate_key_file);
	http2d_buffer_mrproper (&vsrv->ssl_ca_list_file);
	http2d_buffer_mrproper (&vsrv->ssl_ciphers);

	if (vsrv->ssl_context != NULL) {
		SSL_CTX_free (vsrv->ssl_context);
	}

	return ret_ok;
}


static ret_t
find_vserver (SSL                 *ssl,
	      http2d_server_t     *srv,
	      http2d_buffer_t     *servername,
	      http2d_connection_t *conn)
{
	ret_t                    ret;
	http2d_virtual_server_t *vsrv = NULL;
	SSL_CTX                 *ctx;

	/* Try to match the connection to a server
	 */
	ret = http2d_server_get_vserver (srv, servername, conn, &vsrv);
	if ((ret != ret_ok) || (vsrv == NULL)) {
		LOG_ERROR (HTTP2D_ERROR_SSL_SRV_MATCH, servername->buf);
		return ret_error;
	}

	printf ("vsrv=%p\n", vsrv);

	TRACE (ENTRIES, "Setting new TLS context. Virtual host='%s'\n",
	       vsrv->name.buf);

	/* Check whether the Virtual Server supports TLS
	 */
	if (vsrv->ssl_context == NULL) {
		TRACE (ENTRIES, "Virtual server '%s' does not support SSL\n", servername->buf);
		return ret_error;
	}

	/* Set the new SSL context
	 */
	ctx = SSL_set_SSL_CTX (ssl, vsrv->ssl_context);
	if (ctx != vsrv->ssl_context) {
		LOG_ERROR (HTTP2D_ERROR_SSL_CHANGE_CTX, servername->buf);
	}

	/* SSL_set_SSL_CTX() only change certificates. We need to
	 * changes more options by hand.
	 */
	SSL_set_options(ssl, SSL_CTX_get_options(ssl->ctx));

	if ((SSL_get_verify_mode(ssl) == SSL_VERIFY_NONE) ||
	    (SSL_num_renegotiations(ssl) == 0)) {

		SSL_set_verify(ssl, SSL_CTX_get_verify_mode(ssl->ctx),
		               SSL_CTX_get_verify_callback(ssl->ctx));
	}

	return ret_ok;
}


static int
SNI_servername_cb (SSL *ssl, int *ad, void *arg)
{
	ret_t                    ret;
	int                      re;
	const char              *servername;
	http2d_connection_t     *conn;
//	http2d_request_t        *req;
	http2d_buffer_t          tmp;
	http2d_server_t         *srv          = SRV(arg);
	http2d_virtual_server_t *vsrv         = NULL;

	UNUSED(ad);

	/* Get the pointer to the socket
	 */
	conn = SSL_get_app_data (ssl);
	if (unlikely (conn == NULL)) {
		LOG_ERROR (HTTP2D_ERROR_SSL_SOCKET, ssl);
		return SSL_TLSEXT_ERR_ALERT_FATAL;
	}

	http2d_buffer_init(&tmp);
	http2d_buffer_ensure_size(&tmp, 40);

	/* Read the SNI server name
	 */
	servername = SSL_get_servername (ssl, TLSEXT_NAMETYPE_host_name);
	if (servername != NULL) {
		/* SNI FTW!
		 */
		http2d_buffer_add (&tmp, servername, strlen(servername));
		TRACE (ENTRIES, "SNI: Switching to servername='%s'\n", servername);
	} else {
		/* Plan-B: Try to use the server IP as hostname
		 */
		http2d_socket_ntop (&conn->socket, tmp.buf, tmp.size);
		TRACE (ENTRIES, "No SNI: Using IP='%s' as servername.\n", tmp.buf);
	}

	/* Look up and change the vserver
	 */
	ret = find_vserver (ssl, srv, &tmp, conn);
	if (ret != ret_ok) {
		re = SSL_TLSEXT_ERR_NOACK;
	} else {
		re = SSL_TLSEXT_ERR_OK;
	}

	http2d_buffer_mrproper (&tmp);
	return re;
}


static int
NPN_next_proto_cb (SSL *s,
		   const unsigned char **data,
		   unsigned int *len,
		   void *arg)
{
#define NEXT_PROTO_STRING "\x06spdy/3\x08http/1.1"

	*data = (const unsigned char *) NEXT_PROTO_STRING;
	*len = strlen(NEXT_PROTO_STRING);
	return SSL_TLSEXT_ERR_OK;
}


static ret_t
init_ssl (http2d_virtual_server_t *vsrv)
{
	ret_t       ret;
	int         rc;
	const char *error;
	long        options;
	int         verify_mode = SSL_VERIFY_NONE;

	/* Init the OpenSSL context
	 */
	vsrv->ssl_context = SSL_CTX_new (SSLv23_server_method());
	if (vsrv->ssl_context == NULL) {
		LOG_ERROR_S(HTTP2D_ERROR_SSL_ALLOCATE_CTX);
		goto error;
	}

	/* Callback to be used when a DH parameters are required
	 */
//	SSL_CTX_set_tmp_dh_callback (vsrv->ssl_context, tmp_dh_cb);

	/* Set the SSL context options:
	 */
	options  = SSL_OP_ALL;
	options |= SSL_OP_NO_SSLv2;
	options |= SSL_OP_SINGLE_DH_USE;
	options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

	if (vsrv->ssl_cipher_server_preference) {
		options |= SSL_OP_CIPHER_SERVER_PREFERENCE;
	}

	SSL_CTX_set_options (vsrv->ssl_context, options);

	/* Set cipher list that vserver will accept.
	 */
	if (! http2d_buffer_is_empty (&vsrv->ssl_ciphers)) {
		rc = SSL_CTX_set_cipher_list (vsrv->ssl_context, vsrv->ssl_ciphers.buf);
		if (rc != 1) {
			OPENSSL_LAST_ERROR(error);
			LOG_ERROR(HTTP2D_ERROR_SSL_CIPHER,
				  vsrv->ssl_ciphers.buf, error);
			goto error;
		}
	}

	OPENSSL_CLEAR_ERRORS;

	/* Certificate
	 */
	TRACE(ENTRIES, "Vserver '%s'. Reading certificate file '%s'\n",
	      vsrv->name.buf, vsrv->ssl_certificate_file.buf);

	rc = SSL_CTX_use_certificate_chain_file (vsrv->ssl_context, vsrv->ssl_certificate_file.buf);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (HTTP2D_ERROR_SSL_CERTIFICATE,
			   vsrv->ssl_certificate_file.buf, error);
		goto error;
	}

	/* Private key
	 */
	TRACE(ENTRIES, "Vserver '%s'. Reading key file '%s'\n",
	      vsrv->name.buf, vsrv->ssl_certificate_key_file.buf);

	rc = SSL_CTX_use_PrivateKey_file (vsrv->ssl_context, vsrv->ssl_certificate_key_file.buf, SSL_FILETYPE_PEM);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR(HTTP2D_ERROR_SSL_KEY, vsrv->ssl_certificate_key_file.buf, error);
		goto error;
	}

	/* Check private key
	 */
	rc = SSL_CTX_check_private_key (vsrv->ssl_context);
	if (rc != 1) {
		LOG_ERROR_S(HTTP2D_ERROR_SSL_KEY_MATCH);
		goto error;
	}

	/* if (! http2d_buffer_is_empty (&vsrv->req_client_certs)) { */
	/* 	STACK_OF(X509_NAME) *X509_clients; */

	/* 	verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE; */
	/* 	if (http2d_buffer_cmp_str (&vsrv->req_client_certs, "required") == 0) { */
	/* 		verify_mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT; */
	/* 	} */

	/* 	/\* Trusted CA certificates */
	/* 	 *\/ */
	/* 	if (! http2d_buffer_is_empty (&vsrv->certs_ca)) { */
	/* 		rc = SSL_CTX_load_verify_locations (vsrv->ssl_context, vsrv->certs_ca.buf, NULL); */
	/* 		if (rc != 1) { */
	/* 			OPENSSL_LAST_ERROR(error); */
	/* 			LOG_CRITICAL(HTTP2D_ERROR_SSL_CA_READ, */
	/* 				     vsrv->certs_ca.buf, error); */
	/* 			goto error; */
	/* 		} */

	/* 		X509_clients = SSL_load_client_CA_file (vsrv->certs_ca.buf); */
	/* 		if (X509_clients == NULL) { */
	/* 			OPENSSL_LAST_ERROR(error); */
	/* 			LOG_CRITICAL (HTTP2D_ERROR_SSL_CA_LOAD, */
	/* 				      vsrv->certs_ca.buf, error); */
	/* 			goto error; */
	/* 		} */

	/* 		CLEAR_LIBSSL_ERRORS; */

	/* 		SSL_CTX_set_client_CA_list (vsrv->ssl_context, X509_clients); */
	/* 		TRACE (ENTRIES, "Setting client CA list: %s on '%s'\n", vsrv->certs_ca.buf, vsrv->name.buf); */
	/* 	} else { */
	/* 		verify_mode = SSL_VERIFY_NONE; */
	/* 	} */
	/* } */

	/* SSL_CTX_set_verify (vsrv->ssl_context, verify_mode, NULL); */
	/* SSL_CTX_set_verify_depth (vsrv->ssl_context, vsrv->verify_depth); */

	// TODO: I do not whether this will be needed
	//
	/* SSL_CTX_set_mode (vsrv->ssl_context, */
	/* 		  SSL_CTX_get_mode(vsrv->ssl_context) | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER); */

	/* Read ahead
	 */
	SSL_CTX_set_read_ahead (vsrv->ssl_context, 1);

	/* Set the SSL context cache
	 */
	rc = SSL_CTX_set_session_id_context (vsrv->ssl_context,
					     (unsigned char *) vsrv->name.buf,
					     (unsigned int)    vsrv->name.len);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (HTTP2D_ERROR_SSL_SESSION_ID, vsrv->name.buf, error);
	}

	SSL_CTX_set_session_cache_mode (vsrv->ssl_context, SSL_SESS_CACHE_SERVER);

	/* Enable SNI
	 */
	rc = SSL_CTX_set_tlsext_servername_callback (vsrv->ssl_context, SNI_servername_cb);
	if (rc != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_WARNING (HTTP2D_ERROR_SSL_SNI, vsrv->name.buf, error);
	} else {
		rc = SSL_CTX_set_tlsext_servername_arg (vsrv->ssl_context, vsrv->srv);
		if (rc != 1) {
			OPENSSL_LAST_ERROR(error);
			LOG_WARNING (HTTP2D_ERROR_SSL_SNI, vsrv->name.buf, error);
		}
	}

	/* Enable NPN
	 */
	SSL_CTX_set_next_protos_advertised_cb (vsrv->ssl_context, NPN_next_proto_cb, NULL);


	return ret_ok;

error:
	if (vsrv->ssl_context != NULL) {
		SSL_CTX_free (vsrv->ssl_context);
		vsrv->ssl_context = NULL;
	}

	return ret_error;
}


ret_t
http2d_virtual_server_initialize (http2d_virtual_server_t *vsrv)
{
	ret_t ret;

	ret = init_ssl (vsrv);
	if (ret != ret_ok) {
		return ret_error;
	}

	return ret_ok;
}
