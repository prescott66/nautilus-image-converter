SUBDIRS = data src po

DISTCHECK_CONFIGURE_FLAGS = --with-nautilusdir='$${libdir}/nautilus/extensions-2.0-distcheck'

INTLTOOL = 			\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in

EXTRA_DIST = 			\
	ChangeLog               \
	AUTHORS			\
	NEWS			\
	$(INTLTOOL)

DISTCLEANFILES = 		\
	intltool-extract	\
	intltool-merge		\
	intltool-update

