# Copyright 2019 MXNavi, All Rights Reserved
### Standard Makefile template
### Copyright (C) Matthew Peddie <peddie@alum.mit.edu>
###
### This file is hereby placed in the public domain, or, if your legal
### system doesn't recognize this concept, you may consider it
### licensed under the WTFPL version 2.0 or any BSD license you
### choose.
###
### This file should be all you need to configure a basic project;
### obviously for more complex projects, you'll need to edit the other
### files as well.  It supports only one project at a time.  Type
### ``make help'' for usage help.

# What's the executable called?
PROJ = libmlvncclient

LOCAL_PATH:= $(shell pwd)

# TODO:
BUILD_FOR_HU ?= no

####################################################
vncclient_SRC_FILES := \
    libvncclient/cursor.c \
    libvncclient/listen.c \
    libvncclient/rfbproto.c \
    libvncclient/sockets.c \
    libvncclient/vncviewer.c \
    libvncclient/tls_none.c \
    common/minilzo.c \

LOCAL_SRC_FILES := \
    $(vncclient_SRC_FILES)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/build \
    $(LOCAL_PATH)/libvncclient \
    $(LOCAL_PATH)/common \

LOCAL_CFLAGS += -Wno-format
LOCAL_CFLAGS += -Wno-unused-result

LOCAL_CFLAGS += -Wno-unused-parameter -Wno-sign-compare -Wno-pointer-sign -Wno-deprecated-declarations -Wno-type-limits

LOCAL_CFLAGS += -DLIBVNCSERVER_HAVE_ML_EXT=1

LOCAL_CFLAGS += -ULIBVNCSERVER_HAVE_LIBSSL  -ULIBVNCSERVER_HAVE_TLS \
    -ULIBVNCSERVER_HAVE_LIBJPEG -ULIBVNCSERVER_HAVE_LIBNSL \
    -ULIBVNCSERVER_HAVE_LIBPNG -ULIBVNCSERVER_HAVE_X11 -ULIBVNCSERVER_IPv6 \
    -ULIBVNCSERVER_WITH_WEBSOCKETS \
    -UHAVE_LIBSSL -UWITH_WEBSOCKETS

LOCAL_CFLAGS += -fvisibility=hidden

LOCAL_LIBNAMES += 
LOCAL_LIBDIRS += 

#######################################################


include $(ROOT_DIR)/build/makefile-$(TARGET_PLATFORM)-$(TARGET_ARCH).mk

