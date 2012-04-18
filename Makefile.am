## http2d: Makefile.am  -*- makefile -*-

SUBDIRS = . m4 src
SUFFIXES = .sample.pre .sample .h.pre .h
ACLOCAL_AMFLAGS = -I m4

.h.pre.h:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%cgiroot%|${CGI_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@
.sample.pre.sample:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%cgiroot%|${CGI_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@


# Constants
CONSTANTS_PRE = \
constants.h.pre

CONSTANTS = \
$(CONSTANTS_PRE:.h.pre=.h)

# Configurations
CONFS_PRE = \
http2d.conf.sample.pre

CONFS = \
$(CONFS_PRE:.sample.pre=.sample)


CLEANFILES = \
$(CONFS)     \
$(CONSTANTS)

EXTRA_DIST =     \
$(CONFS_PRE)     \
$(CONSTANTS_PRE)

all-local: $(CONSTANTS) $(CONFS)

test:
	$(MAKE) -C src test

cocomo:
	sloccount src/*.{c,h,py} `find . -name Makefile.am` *.pre