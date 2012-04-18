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
#include "http.h"
#include "error_log.h"

#define entry(name,name_str)                           \
	    case name:	                               \
	         if (len) *len = sizeof(name_str) - 1; \
	         *str = name_str;                      \
 		 return ret_ok;


ret_t
http2d_http_method_to_string (http2d_http_method_t method, const char **str, cuint_t *len)
{
	switch (method) {
		/* HTTP 1.1 methods
		 */
		entry (http_get, "GET");
		entry (http_post, "POST");
		entry (http_head, "HEAD");
		entry (http_put, "PUT");
		entry (http_options, "OPTIONS");
		entry (http_delete, "DELETE");
		entry (http_trace, "TRACE");
		entry (http_connect, "CONNECT");

		/* WebDAV methods
		 */
		entry (http_copy, "COPY");
		entry (http_lock, "LOCK");
		entry (http_mkcol, "MKCOL");
		entry (http_move, "MOVE");
		entry (http_notify, "NOTIFY");
		entry (http_poll, "POLL");
		entry (http_propfind, "PROPFIND");
		entry (http_proppatch, "PROPPATCH");
		entry (http_search, "SEARCH");
		entry (http_subscribe, "SUBSCRIBE");
		entry (http_unlock, "UNLOCK");
		entry (http_unsubscribe, "UNSUBSCRIBE");
		entry (http_report, "REPORT");
		entry (http_patch, "PATCH");
		entry (http_version_control, "VERSION_CONTROL");
		entry (http_checkout, "CHECKOUT");
		entry (http_uncheckout, "UNCHECKOUT");
		entry (http_checkin, "CHECKIN");
		entry (http_update, "UPDATE");
		entry (http_label, "LABEL");
		entry (http_mkworkspace, "MKWORKSPACE");
		entry (http_mkactivity, "MKACTIVITY");
		entry (http_baseline_control, "BASELINE_CONTROL");
		entry (http_merge, "MERGE");
		entry (http_invalid, "INVALID");

	default:
		break;
	};

	if (len) *len = 7;
	*str = "UNKNOWN";
	return ret_error;
}


ret_t
http2d_http_string_to_method (http2d_buffer_t      *string,
			      http2d_http_method_t *method)
{
	if (http2d_buffer_case_cmp_str (string, "get") == 0)
		*method = http_get;
	else if (http2d_buffer_case_cmp_str (string, "post") == 0)
		*method = http_post;
	else if (http2d_buffer_case_cmp_str (string, "head") == 0)
		*method = http_head;
	else if (http2d_buffer_case_cmp_str (string, "put") == 0)
		*method = http_put;
	else if (http2d_buffer_case_cmp_str (string, "options") == 0)
		*method = http_options;
	else if (http2d_buffer_case_cmp_str (string, "delete") == 0)
		*method = http_delete;
	else if (http2d_buffer_case_cmp_str (string, "trace") == 0)
		*method = http_trace;
	else if (http2d_buffer_case_cmp_str (string, "connect") == 0)
		*method = http_connect;
	else if (http2d_buffer_case_cmp_str (string, "copy") == 0)
		*method = http_copy;
	else if (http2d_buffer_case_cmp_str (string, "lock") == 0)
		*method = http_lock;
	else if (http2d_buffer_case_cmp_str (string, "mkcol") == 0)
		*method = http_mkcol;
	else if (http2d_buffer_case_cmp_str (string, "move") == 0)
		*method = http_move;
	else if (http2d_buffer_case_cmp_str (string, "notify") == 0)
		*method = http_notify;
	else if (http2d_buffer_case_cmp_str (string, "poll") == 0)
		*method = http_poll;
	else if (http2d_buffer_case_cmp_str (string, "propfind") == 0)
		*method = http_propfind;
	else if (http2d_buffer_case_cmp_str (string, "proppatch") == 0)
		*method = http_proppatch;
	else if (http2d_buffer_case_cmp_str (string, "search") == 0)
		*method = http_search;
	else if (http2d_buffer_case_cmp_str (string, "subscribe") == 0)
		*method = http_subscribe;
	else if (http2d_buffer_case_cmp_str (string, "unlock") == 0)
		*method = http_unlock;
	else if (http2d_buffer_case_cmp_str (string, "unsubscribe") == 0)
		*method = http_unsubscribe;
	else if (http2d_buffer_case_cmp_str (string, "report") == 0)
		*method = http_report;
	else if (http2d_buffer_case_cmp_str (string, "patch") == 0)
		*method = http_patch;
	else if (http2d_buffer_case_cmp_str (string, "version_control") == 0)
		*method = http_version_control;
	else if (http2d_buffer_case_cmp_str (string, "checkout") == 0)
		*method = http_checkout;
	else if (http2d_buffer_case_cmp_str (string, "uncheckout") == 0)
		*method = http_uncheckout;
	else if (http2d_buffer_case_cmp_str (string, "checkin") == 0)
		*method = http_checkin;
	else if (http2d_buffer_case_cmp_str (string, "update") == 0)
		*method = http_update;
	else if (http2d_buffer_case_cmp_str (string, "label") == 0)
		*method = http_label;
	else if (http2d_buffer_case_cmp_str (string, "mkworkspace") == 0)
		*method = http_mkworkspace;
	else if (http2d_buffer_case_cmp_str (string, "mkactivity") == 0)
		*method = http_mkactivity;
	else if (http2d_buffer_case_cmp_str (string, "baseline_control") == 0)
		*method = http_baseline_control;
	else if (http2d_buffer_case_cmp_str (string, "merge") == 0)
		*method = http_merge;
	else if (http2d_buffer_case_cmp_str (string, "invalid") == 0)
		*method = http_invalid;
	else if (http2d_buffer_case_cmp_str (string, "purge") == 0)
		*method = http_purge;
	else {
		*method = http_unknown;
		return ret_not_found;
	}

	return ret_ok;
}


ret_t
http2d_http_version_to_string (http2d_prot_application_t version, const char **str, cuint_t *len)
{
	switch (version) {
		entry (prot_app_http11, "HTTP/1.1");
		entry (prot_app_http10, "HTTP/1.0");
		entry (prot_app_http20, "HTTP/2.0");
	default:
		break;
	};

	if (len) *len = 12;
	*str = "HTTP/Unknown";
	return ret_error;
}


ret_t
http2d_http_code_to_string (http2d_http_t code, const char **str, cuint_t *len)
{
	switch (code) {
		/* 2xx
		 */
		entry (http_ok, http_ok_string);
		entry (http_created, http_created_string);
		entry (http_accepted, http_accepted_string);
		entry (http_non_authoritative_info, http_non_authoritative_info_string);
		entry (http_no_content, http_no_content_string);
		entry (http_reset_content, http_reset_content_string);
		entry (http_partial_content, http_partial_content_string);
		entry (http_multi_status, http_multi_status_string);
		entry (http_im_used, http_im_used_string);

		/* 3xx
		 */
		entry (http_moved_permanently, http_moved_permanently_string);
		entry (http_moved_temporarily, http_moved_temporarily_string);
		entry (http_see_other, http_see_other_string);
		entry (http_not_modified, http_not_modified_string);
		entry (http_temporary_redirect, http_temporary_redirect_string);
		entry (http_multiple_choices, http_multiple_choices_string);
		entry (http_use_proxy, http_use_proxy_string);

		/* 4xx
		 */
		entry (http_bad_request, http_bad_request_string);
		entry (http_unauthorized, http_unauthorized_string);
		entry (http_access_denied, http_access_denied_string);
		entry (http_not_found, http_not_found_string);
		entry (http_method_not_allowed, http_method_not_allowed_string);
		entry (http_not_acceptable, http_not_acceptable_string);
		entry (http_request_timeout, http_request_timeout_string);
		entry (http_gone, http_gone_string);
		entry (http_length_required, http_length_required_string);
		entry (http_request_entity_too_large, http_request_entity_too_large_string);
		entry (http_unsupported_media_type, http_unsupported_media_type_string);
		entry (http_request_uri_too_long, http_request_uri_too_long_string);
		entry (http_range_not_satisfiable, http_range_not_satisfiable_string);
		entry (http_upgrade_required, http_upgrade_required_string);
		entry (http_payment_required, http_payment_required_string);
		entry (http_proxy_auth_required, http_proxy_auth_required_string);
		entry (http_conflict, http_conflict_string);
		entry (http_precondition_failed, http_precondition_failed_string);
		entry (http_expectation_failed, http_expectation_failed_string);
		entry (http_unprocessable_entity, http_unprocessable_entity_string);
		entry (http_locked, http_locked_string);
		entry (http_failed_dependency, http_failed_dependency_string);
		entry (http_unordered_collection, http_unordered_collection_string);
		entry (http_retry_with, http_retry_with_string);

		/* 5xx
		 */
		entry (http_internal_error, http_internal_error_string);
		entry (http_not_implemented, http_not_implemented_string);
		entry (http_bad_gateway, http_bad_gateway_string);
		entry (http_service_unavailable, http_service_unavailable_string);
		entry (http_gateway_timeout, http_gateway_timeout_string);
		entry (http_version_not_supported, http_version_not_supported_string);
		entry (http_variant_also_negotiates, http_variant_also_negotiates_string);
		entry (http_insufficient_storage, http_insufficient_storage_string);
		entry (http_bandwidth_limit_exceeded, http_bandwidth_limit_exceeded_string);
		entry (http_not_extended, http_not_extended_string);

		/* 1xx
		 */
		entry (http_continue, http_continue_string);
		entry (http_processing, http_processing_string);

	default:
		*str = "500 Unknown error";
		return ret_error;
	}

	return ret_ok;
}


ret_t
http2d_http_code_copy (http2d_http_t code, http2d_buffer_t *buf)
{
#define entry_code(c)    \
	case http_ ## c: \
	   return http2d_buffer_add (buf, http_ ## c ## _string, sizeof(http_ ## c ## _string)-1)

	switch (code) {

		/* 2xx
		 */
		entry_code (ok);
		entry_code (created);
		entry_code (accepted);
		entry_code (non_authoritative_info);
		entry_code (no_content);
		entry_code (reset_content);
		entry_code (partial_content);
		entry_code (multi_status);
		entry_code (im_used);

		/* 3xx
		 */
		entry_code (moved_permanently);
		entry_code (moved_temporarily);
		entry_code (see_other);
		entry_code (not_modified);
		entry_code (temporary_redirect);
		entry_code (multiple_choices);
		entry_code (use_proxy);

		/* 4xx
		 */
		entry_code (bad_request);
		entry_code (unauthorized);
		entry_code (access_denied);
		entry_code (not_found);
		entry_code (method_not_allowed);
		entry_code (not_acceptable);
		entry_code (request_timeout);
		entry_code (length_required);
		entry_code (gone);
		entry_code (request_entity_too_large);
		entry_code (request_uri_too_long);
		entry_code (unsupported_media_type);
		entry_code (range_not_satisfiable);
		entry_code (upgrade_required);
		entry_code (payment_required);
		entry_code (proxy_auth_required);
		entry_code (conflict);
		entry_code (precondition_failed);
		entry_code (expectation_failed);
		entry_code (unprocessable_entity);
		entry_code (locked);
		entry_code (failed_dependency);
		entry_code (unordered_collection);
		entry_code (retry_with);

		/* 5xx
		 */
		entry_code (internal_error);
		entry_code (not_implemented);
		entry_code (bad_gateway);
		entry_code (service_unavailable);
		entry_code (gateway_timeout);
		entry_code (version_not_supported);
		entry_code (insufficient_storage);
		entry_code (bandwidth_limit_exceeded);
		entry_code (not_extended);
		entry_code (variant_also_negotiates);

		/* 1xx
		 */
		entry_code (continue);
		entry_code (processing);

	default:
 		LOG_WARNING (HTTP2D_ERROR_HTTP_UNKNOWN_CODE, code);
 		http2d_buffer_add_str (buf, http_internal_error_string);
 		return ret_error;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}

