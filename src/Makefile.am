NULL =

INCLUDES =						\
	-DG_LOG_DOMAIN=\"Nautilus-Image-Converter\"	\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	-Werror                                         \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(NAUTILUS_CFLAGS)

# FIXME: get this from the .pc file
nautilus_extensiondir=$(libdir)/nautilus/extensions-1.0

nautilus_extension_LTLIBRARIES=libnautilus-image-converter.la

libnautilus_image_converter_la_SOURCES =	\
	image-converter.c			\
	nautilus-image-converter.c		\
	nautilus-image-converter.h		\
	nautilus-image-resizer.c		\
	nautilus-image-resizer.h		\
	$(NULL)

libnautilus_image_converter_la_LDFLAGS = -module -avoid-version
libnautilus_image_converter_la_LIBADD  = $(NAUTILUS_LIBS)
