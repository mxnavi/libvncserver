LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libvncserver
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \
                    $(LOCAL_PATH)/lib \
                    $(LOCAL_PATH)/libvncserver \
                    $(LOCAL_PATH) \
                    $(LOCAL_PATH)/common \
                    external/zlib \

                    #external/libpng \
                    external/openssl/include \
                    external/jpeg \

LOCAL_SRC_FILES := libvncserver/auth.c \
                   libvncserver/cargs.c \
                   libvncserver/corre.c \
                   libvncserver/cursor.c \
                   libvncserver/cutpaste.c \
                   libvncserver/draw.c \
                   libvncserver/font.c \
                   libvncserver/hextile.c \
                   libvncserver/httpd.c \
                   libvncserver/main.c \
                   libvncserver/rfbregion.c \
                   libvncserver/rfbserver.c \
                   libvncserver/rre.c \
                   libvncserver/scale.c \
                   libvncserver/selbox.c \
                   libvncserver/sockets.c \
                   libvncserver/stats.c \
                   libvncserver/translate.c \
                   libvncserver/ultra.c \
                   libvncserver/zlib.c \
                   libvncserver/zrle.c \
                   libvncserver/zrleoutstream.c \
                   libvncserver/zrlepalettehelper.c \
                   libvncserver/rfbssl_none.c \
                   common/d3des.c \
                   common/minilzo.c \
                   common/sha1.c \
                   common/vncauth.c \
                   test/bmp.c


                   #libvncserver/tight.c \
                   libvncserver/rfbcrypto_openssl.c \
                   libvncserver/rfbssl_openssl.c \
                   libvncserver/websockets.c \
                   common/turbojpeg.c \

#LOCAL_STATIC_LIBRARIES := libz libpng libjpeg_static
#LOCAL_SHARED_LIBRARIES := libssl

LOCAL_CFLAGS := \
    -ULIBVNCSERVER_HAVE_LIBSSL  -ULIBVNCSERVER_HAVE_TLS \
    -ULIBVNCSERVER_HAVE_LIBJPEG -ULIBVNCSERVER_HAVE_LIBNSL \
    -ULIBVNCSERVER_HAVE_LIBPNG -ULIBVNCSERVER_HAVE_X11 -ULIBVNCSERVER_IPv6 \
    -ULIBVNCSERVER_WITH_WEBSOCKETS \
    -DHAVE_LIBSSL -DWITH_WEBSOCKETS \
    -Wall

# -Werror \
    -Wno-missing-field-initializers \
    -Wno-sign-compare \
    -Wno-tautological-compare \
    -Wno-unused-parameter \
    -Wno-user-defined-warnings \

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays -fvisibility=hidden

#LOCAL_SDK_VERSION := 14

include $(BUILD_STATIC_LIBRARY)
