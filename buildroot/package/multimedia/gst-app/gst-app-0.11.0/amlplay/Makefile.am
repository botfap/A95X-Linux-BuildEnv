lib_LTLIBRARIES = libgplaycore.la

libgplaycore_la_SOURCES = mfw_gplay_core.c fsl_player_debug.c fsl_player_osal.c fsl_player_queue.c fsl_player_ui_message.c gst_snapshot.c
libgplaycore_la_CFLAGS  = $(GST_CFLAGS) 
libgplaycore_la_LIBADD = $(GST_LIBS) -lz

bin_PROGRAMS = gplay
gplay_SOURCES = mfw_gplay.c playlist.c
gplay_CFLAGS  = $(GST_CFLAGS) 
gplay_LDADD   = libgplaycore.la

noinst_HEADERS = \
    fsl_player_debug.h      \
    fsl_player_drm_types.h  \
    fsl_player_osal.h       \
    fsl_player_queue.h      \
    fsl_player_types.h      \
    fsl_player_ui_message.h \
    gst_snapshot.h          \
    mfw_gplay_core.h        \
    playlist.h
