# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

external_subdirs = @mpl_srcdir@
external_ldflags = -L$(top_builddir)/@mpl_srcdir@
external_libs = -lmpl

EXTRA_DIST = VERSION examples/binding.c

bin_PROGRAMS =

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/@mpl_srcdir@/include \
	-I$(top_builddir)/@mpl_srcdir@/include

noinst_LTLIBRARIES = libhydra.la
libhydra_la_SOURCES = \
	include/hydra.h \
	include/hydra_server.h

# The below directories contribute to libhydra
include utils/Makefile.mk
include tools/Makefile.mk
include pm/Makefile.mk
include ui/Makefile.mk

# External subdirs should be built first, as we might depend on them
SUBDIRS = ${external_subdirs} .

# Provide an alias for mpirun to support tools that assume that
# mpiexec is called mpirun
install-exec-hook:
	if [ -x ${DESTDIR}${bindir}/yod.hydra ] ; then \
	    cd ${DESTDIR}${bindir} && ln -f -s yod.hydra yod ; \
	fi

# A special alternate installation target when using multiple process
# managers; this is basically the install target, but without the
# install-exec-hook
install-alt: $(bin_PROGRAMS)
	@for foo in $? ; do $(MAKE) install $$foo ; done

uninstall-local:
	rm -f ${DESTDIR}${bindir}/mpirun ${DESTDIR}${bindir}/mpiexec

htmldoc:
	if test "@DOXYGEN@" != "" ; then @DOXYGEN@ hydra-doxygen.cfg ; fi
