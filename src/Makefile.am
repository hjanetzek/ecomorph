MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I. \
	   -I$(top_srcdir) \
	   -I$(includedir) \
	   @E_CFLAGS@

pkgdir = $(datadir)/$(MODULE_ARCH)
pkg_LTLIBRARIES = module.la
module_la_SOURCES      = e_mod_main.h \
                         eco_main.c \
                         eco_config.c \
                         eco_config.h \
                         eco_actions.c \
                         eco_actions.h \
                         eco_event.c \
                         eco_event.h \
                         eco_config_water.c \
                         eco_config_blur.c \
                         eco_config_mblur.c \
                         eco_config_expo.c \
                         eco_config_switcher.c \
                         eco_config_wobbly.c \
                         eco_config_wall.c \
                         eco_config_scale.c \
	                 eco_config_scaleaddon.c \
                         eco_config_ring.c \
                         eco_config_shift.c \
                         eco_config_animation.c \
                         eco_config_animation2.c \
                         eco_config_animation3.c \
                         eco_config_animation4.c \
                         eco_config_animation5.c \
                         eco_config_opacity.c \
                         eco_config_move.c \
                         eco_config_cube.c \
                         eco_config_cubereflex.c \
                         eco_config_rotate.c \
                         eco_config_thumbnail.c \
                         eco_config_decoration.c \
			 eco_config_general.c


module_la_LIBADD = @E_LIBS@
module_la_LDFLAGS = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

clean-local:
	rm -rf *~
