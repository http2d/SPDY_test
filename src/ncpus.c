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
#include "ncpus.h"
#include "error_log.h"a
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>


#define FAILED -1

/**
 * Determine number of processors online.
 *
 * We will in the future use this to gauge how many concurrent tasks
 * should run on this machine.  Obviously this is only very rough: the
 * correct number needs to take into account disk buffers, IO
 * bandwidth, other tasks, etc.
**/

#if defined(__hpux__) || defined(__hpux)

#include <sys/param.h>
#include <sys/pstat.h>

int dcc_ncpus(int *ncpus)
{
	struct pst_dynamic psd;
	if (pstat_getdynamic(&psd, sizeof(psd), 1, 0) != -1) {
		*ncpus = psd.psd_proc_cnt;
		return 0;
	} else {
		*ncpus = -1;
		LOG_ERRNO (errno, http2d_err_error, HTTP2D_ERROR_NCPUS_PSTAT);
		return FAILED;
	}
}


#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)

/* http://www.FreeBSD.org/cgi/man.cgi?query=sysctl&sektion=3&manpath=FreeBSD+4.6-stable
   http://www.openbsd.org/cgi-bin/man.cgi?query=sysctl&sektion=3&manpath=OpenBSD+Current
   http://www.tac.eu.org/cgi-bin/man-cgi?sysctl+3+NetBSD-current
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
int dcc_ncpus(int *ncpus)
{
	int mib[2];
	size_t len = sizeof(*ncpus);
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	if (sysctl(mib, 2, ncpus, &len, NULL, 0) == 0)
         return 0;
	else {
         LOG_ERRNO (errno, http2d_err_error, HTTP2D_ERROR_NCPUS_HW_NCPU);
         return FAILED;
	}
}

#elif !defined (_WIN32) /* every other system but Windows */

/*
  http://www.opengroup.org/onlinepubs/007904975/functions/sysconf.html
  http://docs.sun.com/?p=/doc/816-0213/6m6ne38dd&a=view
  http://www.tru64unix.compaq.com/docs/base_doc/DOCUMENTATION/V40G_HTML/MAN/MAN3/0629____.HTM
  http://techpubs.sgi.com/library/tpl/cgi-bin/getdoc.cgi?coll=0650&db=man&fname=/usr/share/catman/p_man/cat3c/sysconf.z
*/

int dcc_ncpus(int *ncpus)
{
#if defined(_SC_NPROCESSORS_ONLN)
	/* Linux, Solaris, Tru64, UnixWare 7, and Open UNIX 8  */
	*ncpus = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
	/* IRIX */
	*ncpus = sysconf(_SC_NPROC_ONLN);
#else
#warning "Please port this function"
	*ncpus = -1;                /* unknown */
#endif

	if (*ncpus == -1) {
         LOG_ERRNO_S (errno, http2d_err_error, HTTP2D_ERROR_NCPUS_SYSCONF);
         return FAILED;
	} else if (*ncpus == 0) {
		/* If there are no cpus, what are we running on ?
		 * NOTE: it has apparently been observed to happen on ARM Linux
		 */
		*ncpus = 1;
	}

	return 0;
}
#endif


