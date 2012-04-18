## http2d: src/Makefile.am  -*- makefile -*-

INCLUDES =             \
	-I${top_srcdir}   \
	$(PTHREAD_CFLAGS) \
	$(OPENSSL_CFLAGS)

server_src =              \
	$(BUILT_SOURCES)     \
	common.h             \
	common-internal.h    \
	md5.h                \
	md5.c                \
	crc32.h              \
	crc32.c              \
	sha1.h               \
	sha1.c               \
	server.h             \
	server.c             \
	server-cfg.h         \
	server-cfg.c         \
	buffer.h             \
	buffer.c             \
	util.h               \
	util.c               \
	list.h               \
	list.c               \
	avl_generic.h        \
	avl_generic.c        \
	avl.h                \
	avl.c                \
	config_node.h        \
	config_node.c        \
	config_reader.h      \
	config_reader.c      \
	error_log.h          \
	error_log.c          \
	nullable.h           \
	threading.h          \
	threading.c          \
	cache.h              \
	cache.c              \
	iocache.h            \
	iocache.c            \
	connection.h         \
	connection.c         \
	connection-spdy.h    \
	connection-spdy.c    \
	connection-http.h    \
	connection-http.c    \
	resolv_cache.h       \
	resolv_cache.c       \
	socket_lowlevel.h    \
	socket.h             \
	socket.c             \
	socket-ssl.h         \
	socket-ssl.c         \
	bind.h               \
	bind.c               \
	version.h            \
	version.c            \
	protocol.h           \
	protocol.c           \
	http.h               \
	http.c               \
	ncpus.h              \
	ncpus.c              \
	thread.h             \
	thread.c             \
	virtual_server.h     \
	virtual_server.c     \
	virtual_server-cfg.h \
	virtual_server-cfg.c \
	access.h             \
	access.c             \
	trace.h              \
	trace.c              \
	request.h            \
	request.c            \
	request-spdy.h       \
	request-spdy.c       \
	request-http.h       \
	request-http.c       \
	init.h               \
	init.c               \
	spdy.h               \
	spdy.c               \
	spdy-zlib.h          \
	spdy-zlib.c          \
	header.h             \
	header.c

tests_src =         \
	buffer_test.c  \
	server_test.c

# Server
#
sbin_PROGRAMS = http2d

http2d_LDFLAGS = -export-dynamic
http2d_LDADD =       \
	-lev            \
	-lz             \
	$(OPENSSL_LIBS) \
	$(PTHREAD_LIBS)
http2d_SOURCES = \
	main.c      \
	$(server_src)

# Unit testing
#
noinst_PROGRAMS = tests

tests_LDFLAGS = $(http2d_LDFLAGS)
tests_LDADD   = $(http2d_LDADD)
tests_SOURCES =    \
	tests.c       \
	ctest.h       \
	$(tests_src)  \
	$(server_src)

test: tests
	./tests

#
# File generation rules
#
BUILT_SOURCES = \
errors.h \
errors_defs.h

errors.h: errors.py error_list.py
	python errors.py --errors $@

errors_defs.h: errors.py error_list.py
	python errors.py --defines $@

EXTRA_DIST =     \
errors.py        \
error_list.py    \
$(BUILT_SOURCES)

distclean-local:
	$(RM) -f $(BUILT_SOURCES)
