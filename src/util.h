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

#ifndef HTTP2D_UTIL_H
#define HTTP2D_UTIL_H

#include "common.h"
#include "avl.h"
#include "trace.h"

/* #include <http2d/trace.h> */
/* #include <http2d/plugin_loader.h> */

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_GRP_H
# include <grp.h>
#endif

#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <time.h>
#include <dirent.h>
#include <errno.h>

#include "iocache.h"


/* #include <http2d/buffer.h> */
/* #include <http2d/iocache.h> */


/* Error buffer size for http2d_strerror_r().
 */
#define ERROR_MIN_BUFSIZE	64	/* min. buffer size */
#define ERROR_MAX_BUFSIZE	512	/* max. buffer size */

#ifdef _WIN32
# define http2d_stat(path,buf)   http2d_win32_stat(path,buf)
# define http2d_lstat(path,buf)  http2d_win32_stat(path,buf)
# define http2d_error            GetLastError()
#else
# define http2d_error            errno
#endif

/* Missing functions
 */
#ifndef HAVE_STRNSTR
char *strnstr (const char *s, const char *find, size_t slen);
#endif
#ifndef HAVE_STRCASESTR
char *strcasestr (register char *s, register char *find);
#endif
#ifndef HAVE_STRLCAT
size_t strlcat (char *dst, const char *src, size_t siz);
#endif
#ifndef HAVE_MALLOC
void *rpl_malloc (size_t n);
#endif

char *strncasestr  (const char *s, const char *find, size_t slen);
char *strncasestrn (const char *s, size_t slen, const char *find, size_t findlen);

#define strncasestrn_s(s,s_len,lit) strncasestrn(s, s_len, lit, sizeof(lit)-1)
#define strcasestrn_s(s,lit)        strncasestrn(s, lit, sizeof(lit)-1)

/* Constants
 */
extern const char hex2dec_tab[256];
extern const char *month[13];

/* String management functions
 */
char   *http2d_strerror_r         (int err, char *buf, size_t bufsize);
int     http2d_isbigendian        (void);
char   *http2d_min_str            (char *s1, char *s2);
char   *http2d_max_str            (char *s1, char *s2);
int     http2d_estimate_va_length (const char *format, va_list ap);
long    http2d_eval_formated_time (http2d_buffer_t *buf);
ret_t   http2d_fix_dirpath        (http2d_buffer_t *buf);

ret_t   http2d_find_header_end    (http2d_buffer_t  *buf,
				   char              **end,
				   cuint_t            *sep_len);

ret_t   http2d_find_header_end_cstr (char      *c_str,
				     cint_t     c_len,
				     char     **end,
				     cuint_t   *sep_len);

ret_t   http2d_parse_host         (http2d_buffer_t *buf,
				   http2d_buffer_t *host,
				   cuint_t           *port);

int     http2d_string_is_ipv6     (http2d_buffer_t *ip);
ret_t   http2d_copy_local_address (void              *socket, http2d_buffer_t *buf);

ret_t   http2d_buf_add_bogonow    (http2d_buffer_t *buf, bool update);
ret_t   http2d_buf_add_backtrace  (http2d_buffer_t *buf, int n_skip, const char *new_line, const char *line_pre);

ret_t   http2d_find_exec_in_path  (const char        *bin_name,
				   http2d_buffer_t *fullpath);

ret_t   http2d_atoi               (const char *str, int *ret_value);
ret_t   http2d_atob               (const char *str, bool *ret_value);

/* Time management functions
 */
struct tm *http2d_gmtime           (const time_t *timep, struct tm *result);
struct tm *http2d_localtime        (const time_t *timep, struct tm *result);
long      *http2d_get_timezone_ref (void);

/* Thread safe functions
 */
DIR * http2d_opendir       (const char *dirname);
int   http2d_readdir       (DIR *dirstream, struct dirent *entry, struct dirent **result);
int   http2d_closedir      (DIR *dirstream);

int   http2d_stat          (const char *restrict path, struct stat *buf);
int   http2d_lstat         (const char *restrict path, struct stat *buf);
int   http2d_fstat         (int filedes, struct stat *buf);
int   http2d_access        (const char *pathname, int mode);
int   http2d_open          (const char *path, int oflag, int mode);
int   http2d_unlink        (const char *path);
int   http2d_pipe          (int fildes[2]);

ret_t http2d_gethostbyname (http2d_buffer_t *hostname, struct addrinfo **addr);

ret_t http2d_gethostname   (http2d_buffer_t *buf);
ret_t http2d_syslog        (int priority, http2d_buffer_t *buf);

ret_t http2d_getpwnam      (const char *name, struct passwd *pwbuf, char *buf, size_t buflen);
ret_t http2d_getpwuid      (uid_t uid, struct passwd *pwbuf, char *buf, size_t buflen);
ret_t http2d_getpwnam_uid  (const char *name, struct passwd *pwbuf, char *buf, size_t buflen);
ret_t http2d_getgrnam      (const char *name, struct group *pwbuf, char *buf, size_t buflen);
ret_t http2d_getgrgid      (gid_t gid, struct group *pwbuf, char *buf, size_t buflen);
ret_t http2d_getgrnam_gid  (const char *name, struct group *pwbuf, char *buf, size_t buflen);

ret_t http2d_mkstemp       (http2d_buffer_t *buffer, int *fd);
ret_t http2d_mkdtemp       (char *template);

ret_t http2d_mkdir         (const char *path, int mode);
ret_t http2d_mkdir_p       (http2d_buffer_t *path, int mode);
ret_t http2d_mkdir_p_perm  (http2d_buffer_t *dir_path, int create_mode, int ensure_perm);
ret_t http2d_rm_rf         (http2d_buffer_t *path, uid_t uid);

void http2d_random_seed    (void);
long http2d_random         (void);

ret_t http2d_ntop          (int family, struct sockaddr *addr, char *dst, size_t cnt);
ret_t http2d_wait_pid      (int pid, int *retcode);
ret_t http2d_reset_signals (void);

ret_t http2d_io_stat       (http2d_iocache_t        *iocache,
			    http2d_buffer_t         *path,
			    bool                     useit,
			    struct stat             *info_space,
			    http2d_iocache_entry_t **io_entry,
			    struct stat            **info);

/* File descriptors
 */
ret_t http2d_fd_set_nonblocking (int fd, bool enable);
ret_t http2d_fd_set_nodelay     (int fd, bool enable);
ret_t http2d_fd_set_closexec    (int fd);
ret_t http2d_fd_set_reuseaddr   (int fd);
ret_t http2d_fd_close           (int fd);

/* Misc
 */
ret_t http2d_sys_fdlimit_get (cuint_t *limit);
ret_t http2d_sys_fdlimit_set (cuint_t  limit);
ret_t http2d_get_shell       (const char **shell, const char **binary);
ret_t http2d_tmp_dir_copy    (http2d_buffer_t *buffer);

/* IO vectors
 */
ret_t http2d_iovec_skip_sent (struct iovec orig[], uint16_t  orig_len,
				struct iovec dest[], uint16_t *dest_len,
				size_t sent);
ret_t http2d_iovec_was_sent  (struct iovec orig[], uint16_t orig_len, size_t sent);

/* Debug
 */
void  http2d_trace           (const char *entry, const char *file, int line, const char *func, const char *fmt, ...);

/* Path management
 */
ret_t http2d_path_short         (http2d_buffer_t *path);
ret_t http2d_path_arg_eval      (http2d_buffer_t *path);

/* Path walking
 */
ret_t http2d_split_pathinfo     (http2d_buffer_t  *path,
				 cuint_t             init_pos,
				 int                 allow_dirs,
				 char              **pathinfo,
				 int                *pathinfo_len);

ret_t http2d_split_arguments    (http2d_buffer_t *request,
				 int                init_pos,
				 char             **arguments,
				 int               *arguments_len);

ret_t http2d_parse_query_string (http2d_buffer_t *qstring,
				 http2d_avl_t  *arguments);

char  *http2d_header_get_next_line (char *line);
ret_t  http2d_header_del_entry     (http2d_buffer_t *header,
				    const char        *header_name,
				    int                header_name_len);



#endif /* HTTP2D_UTIL_H */
