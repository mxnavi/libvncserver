#ifndef RFBPROTO_H
#define RFBPROTO_H

/**
 @mainpage
 @li @ref libvncserver_api
 @li @ref libvncserver_doc


 @li @ref libvncclient_api
 @li @ref libvncclient_doc

*/

/*
 *  Copyright (C) 2009-2010 D. R. Commander. All Rights Reserved.
 *  Copyright (C) 2005 Rohit Kumar, Johannes E. Schindelin
 *  Copyright (C) 2004-2008 Sun Microsystems, Inc. All Rights Reserved.
 *  Copyright (C) 2000-2002 Constantin Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * rfbproto.h - header file for the RFB protocol version 3.3
 *
 * Uses types CARD<n> for an n-bit unsigned integer, INT<n> for an n-bit signed
 * integer (for n = 8, 16 and 32).
 *
 * All multiple byte integers are in big endian (network) order (most
 * significant byte first).  Unless noted otherwise there is no special
 * alignment of protocol structures.
 *
 *
 * Once the initial handshaking is done, all messages start with a type byte,
 * (usually) followed by message-specific data.  The order of definitions in
 * this file is as follows:
 *
 *  (1) Structures used in several types of message.
 *  (2) Structures used in the initial handshaking.
 *  (3) Message types.
 *  (4) Encoding types.
 *  (5) For each message type, the form of the data following the type byte.
 *      Sometimes this is defined by a single structure but the more complex
 *      messages have to be explained by comments.
 */


#if defined(WIN32) && !defined(__MINGW32__)
#define LIBVNCSERVER_WORDS_BIGENDIAN
#define rfbBool int
#include <sys/timeb.h>
#include <winsock2.h>
#undef SOCKET
#define SOCKET int
#else
#include <rfb/rfbconfig.h>
#include <rfb/rfbint.h>
#endif

#ifdef LIBVNCSERVER_HAVE_LIBZ
#include <zlib.h>
#ifdef __CHECKER__
#undef Z_NULL
#define Z_NULL NULL
#endif
#endif

/* some autotool versions do not properly prefix
   WORDS_BIGENDIAN, so do that manually */
#ifdef WORDS_BIGENDIAN
#define LIBVNCSERVER_WORDS_BIGENDIAN
#endif

/* MS compilers don't have strncasecmp */
#ifdef _MSC_VER
#define strncasecmp _strnicmp
#endif

#if !defined(WIN32) || defined(__MINGW32__)
#define max(a,b) (((a)>(b))?(a):(b))
#ifdef LIBVNCSERVER_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef LIBVNCSERVER_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#define SOCKET int
typedef int8_t rfbBool;
#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE -1
#endif

#ifdef _MSC_VER
#include <stdint.h>
#endif

typedef uint32_t rfbKeySym;
typedef uint32_t rfbPixel;

#ifdef LIBVNCSERVER_NEED_INADDR_T
typedef uint32_t in_addr_t;
#endif

#ifndef INADDR_NONE
#define                INADDR_NONE     ((in_addr_t) 0xffffffff)
#endif

#define MAX_ENCODINGS 21

/*****************************************************************************
 *
 * Structures used in several messages
 *
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Structure used to specify a rectangle.  This structure is a multiple of 4
 * bytes so that it can be interspersed with 32-bit pixel data without
 * affecting alignment.
 */

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} rfbRectangle;

#define sz_rfbRectangle 8


/*-----------------------------------------------------------------------------
 * Structure used to specify pixel format.
 */

typedef struct {

    uint8_t bitsPerPixel;		/* 8,16,32 only */

    uint8_t depth;		/* 8 to 32 */

    uint8_t bigEndian;		/* True if multi-byte pixels are interpreted
				   as big endian, or if single-bit-per-pixel
				   has most significant bit of the byte
				   corresponding to first (leftmost) pixel. Of
				   course this is meaningless for 8 bits/pix */

    uint8_t trueColour;		/* If false then we need a "colour map" to
				   convert pixels to RGB.  If true, xxxMax and
				   xxxShift specify bits used for red, green
				   and blue */

    /* the following fields are only meaningful if trueColour is true */

    uint16_t redMax;		/* maximum red value (= 2^n - 1 where n is the
				   number of bits used for red). Note this
				   value is always in big endian order. */

    uint16_t greenMax;		/* similar for green */

    uint16_t blueMax;		/* and blue */

    uint8_t redShift;		/* number of shifts needed to get the red
				   value in a pixel to the least significant
				   bit. To find the red value from a given
				   pixel, do the following:
				   1) Swap pixel value according to bigEndian
				      (e.g. if bigEndian is false and host byte
				      order is big endian, then swap).
				   2) Shift right by redShift.
				   3) AND with redMax (in host byte order).
				   4) You now have the red value between 0 and
				      redMax. */

    uint8_t greenShift;		/* similar for green */

    uint8_t blueShift;		/* and blue */

    uint8_t pad1;
    uint16_t pad2;

} rfbPixelFormat;

#define sz_rfbPixelFormat 16

/* UltraVNC: Color settings values */
#define rfbPFFullColors		0
#define rfbPF256Colors		1
#define rfbPF64Colors		2
#define rfbPF8Colors		3
#define rfbPF8GreyColors	4
#define rfbPF4GreyColors	5
#define rfbPF2GreyColors	6


/*****************************************************************************
 *
 * Initial handshaking messages
 *
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Protocol Version
 *
 * The server always sends 12 bytes to start which identifies the latest RFB
 * protocol version number which it supports.  These bytes are interpreted
 * as a string of 12 ASCII characters in the format "RFB xxx.yyy\n" where
 * xxx and yyy are the major and minor version numbers (for version 3.3
 * this is "RFB 003.003\n").
 *
 * The client then replies with a similar 12-byte message giving the version
 * number of the protocol which should actually be used (which may be different
 * to that quoted by the server).
 *
 * It is intended that both clients and servers may provide some level of
 * backwards compatibility by this mechanism.  Servers in particular should
 * attempt to provide backwards compatibility, and even forwards compatibility
 * to some extent.  For example if a client demands version 3.1 of the
 * protocol, a 3.0 server can probably assume that by ignoring requests for
 * encoding types it doesn't understand, everything will still work OK.  This
 * will probably not be the case for changes in the major version number.
 *
 * The format string below can be used in sprintf or sscanf to generate or
 * decode the version string respectively.
 */

#define rfbProtocolVersionFormat "RFB %03d.%03d\n"
#define rfbProtocolMajorVersion 3
#define rfbProtocolMinorVersion 8
/* UltraVNC Viewer examines rfbProtocolMinorVersion number (4, and 6)
 * to identify if the server supports File Transfer
 */

typedef char rfbProtocolVersionMsg[13];	/* allow extra byte for null */

#define sz_rfbProtocolVersionMsg 12

/*
 * Negotiation of the security type (protocol version 3.7)
 *
 * Once the protocol version has been decided, the server either sends a list
 * of supported security types, or informs the client about an error (when the
 * number of security types is 0).  Security type rfbSecTypeTight is used to
 * enable TightVNC-specific protocol extensions.  The value rfbSecTypeVncAuth
 * stands for classic VNC authentication.
 *
 * The client selects a particular security type from the list provided by the
 * server.
 */

#define rfbSecTypeInvalid 0
#define rfbSecTypeNone 1
#define rfbSecTypeVncAuth 2


/*-----------------------------------------------------------------------------
 * Authentication
 *
 * Once the protocol version has been decided, the server then sends a 32-bit
 * word indicating whether any authentication is needed on the connection.
 * The value of this word determines the authentication scheme in use.  For
 * version 3.0 of the protocol this may have one of the following values:
 */

#define rfbConnFailed 0
#define rfbNoAuth 1
#define rfbVncAuth 2

#define rfbRA2 5
#define rfbRA2ne 6
#define rfbSSPI 7
#define rfbSSPIne 8
#define rfbTight 16
#define rfbUltra 17
#define rfbTLS 18
#define rfbVeNCrypt 19
#define rfbARD 30
#define rfbMSLogon 0xfffffffa

#define rfbVeNCryptPlain 256
#define rfbVeNCryptTLSNone 257
#define rfbVeNCryptTLSVNC 258
#define rfbVeNCryptTLSPlain 259
#define rfbVeNCryptX509None 260
#define rfbVeNCryptX509VNC 261
#define rfbVeNCryptX509Plain 262
#define rfbVeNCryptX509SASL 263
#define rfbVeNCryptTLSSASL 264

/*
 * rfbConnFailed:	For some reason the connection failed (e.g. the server
 *			cannot support the desired protocol version).  This is
 *			followed by a string describing the reason (where a
 *			string is specified as a 32-bit length followed by that
 *			many ASCII characters).
 *
 * rfbNoAuth:		No authentication is needed.
 *
 * rfbVncAuth:		The VNC authentication scheme is to be used.  A 16-byte
 *			challenge follows, which the client encrypts as
 *			appropriate using the password and sends the resulting
 *			16-byte response.  If the response is correct, the
 *			server sends the 32-bit word rfbVncAuthOK.  If a simple
 *			failure happens, the server sends rfbVncAuthFailed and
 *			closes the connection. If the server decides that too
 *			many failures have occurred, it sends rfbVncAuthTooMany
 *			and closes the connection.  In the latter case, the
 *			server should not allow an immediate reconnection by
 *			the client.
 */

#define rfbVncAuthOK 0
#define rfbVncAuthFailed 1
#define rfbVncAuthTooMany 2


/*-----------------------------------------------------------------------------
 * Client Initialisation Message
 *
 * Once the client and server are sure that they're happy to talk to one
 * another, the client sends an initialisation message.  At present this
 * message only consists of a boolean indicating whether the server should try
 * to share the desktop by leaving other clients connected, or give exclusive
 * access to this client by disconnecting all other clients.
 */

typedef struct {
    uint8_t shared;
} rfbClientInitMsg;

#define sz_rfbClientInitMsg 1


/*-----------------------------------------------------------------------------
 * Server Initialisation Message
 *
 * After the client initialisation message, the server sends one of its own.
 * This tells the client the width and height of the server's framebuffer,
 * its pixel format and the name associated with the desktop.
 */

typedef struct {
    uint16_t framebufferWidth;
    uint16_t framebufferHeight;
    rfbPixelFormat format;	/* the server's preferred pixel format */
    uint32_t nameLength;
    /* followed by char name[nameLength] */
} rfbServerInitMsg;

#define sz_rfbServerInitMsg (8 + sz_rfbPixelFormat)


/*
 * Following the server initialisation message it's up to the client to send
 * whichever protocol messages it wants.  Typically it will send a
 * SetPixelFormat message and a SetEncodings message, followed by a
 * FramebufferUpdateRequest.  From then on the server will send
 * FramebufferUpdate messages in response to the client's
 * FramebufferUpdateRequest messages.  The client should send
 * FramebufferUpdateRequest messages with incremental set to true when it has
 * finished processing one FramebufferUpdate and is ready to process another.
 * With a fast client, the rate at which FramebufferUpdateRequests are sent
 * should be regulated to avoid hogging the network.
 */



/*****************************************************************************
 *
 * Message types
 *
 *****************************************************************************/

/* server -> client */

#define rfbFramebufferUpdate 0
#define rfbSetColourMapEntries 1
#define rfbBell 2
#define rfbServerCutText 3
/* Modif sf@2002 */
#define rfbResizeFrameBuffer 4
#define rfbPalmVNCReSizeFrameBuffer 0xF

#ifdef LIBVNCSERVER_HAVE_ML_EXT
/* MirrorLink message type used by both server and client */
#define rfbMLExt 128
#endif

/* client -> server */

#define rfbSetPixelFormat 0
#define rfbFixColourMapEntries 1	/* not currently supported */
#define rfbSetEncodings 2
#define rfbFramebufferUpdateRequest 3
#define rfbKeyEvent 4
#define rfbPointerEvent 5
#define rfbClientCutText 6
/* Modif sf@2002 - actually bidirectionnal */
#define rfbFileTransfer 7
/* Modif sf@2002 */
#define rfbSetScale 8
/* Modif rdv@2002 */
#define rfbSetServerInput	9
/* Modif rdv@2002 */
#define rfbSetSW	10
/* Modif sf@2002 - TextChat - Bidirectionnal */
#define rfbTextChat	11
/* Modif cs@2005 */
/* PalmVNC 1.4 & 2.0 SetScale Factor message */
#define rfbPalmVNCSetScaleFactor 0xF
/* Xvp message - bidirectional */
#define rfbXvp 250




/*****************************************************************************
 *
 * Encoding types
 *
 *****************************************************************************/

#define rfbEncodingRaw 0
#define rfbEncodingCopyRect 1
#define rfbEncodingRRE 2
#define rfbEncodingCoRRE 4
#define rfbEncodingHextile 5
#define rfbEncodingZlib 6
#define rfbEncodingTight 7
#define rfbEncodingTightPng 0xFFFFFEFC /* -260 */
#define rfbEncodingZlibHex 8
#define rfbEncodingUltra 9
#define rfbEncodingZRLE 16
#define rfbEncodingZYWRLE 17

#define rfbEncodingH264               0x48323634

/* Cache & XOR-Zlib - rdv@2002 */
#define rfbEncodingCache                 0xFFFF0000
#define rfbEncodingCacheEnable           0xFFFF0001
#define rfbEncodingXOR_Zlib              0xFFFF0002
#define rfbEncodingXORMonoColor_Zlib     0xFFFF0003
#define rfbEncodingXORMultiColor_Zlib    0xFFFF0004
#define rfbEncodingSolidColor            0xFFFF0005
#define rfbEncodingXOREnable             0xFFFF0006
#define rfbEncodingCacheZip              0xFFFF0007
#define rfbEncodingSolMonoZip            0xFFFF0008
#define rfbEncodingUltraZip              0xFFFF0009

/* Xvp pseudo-encoding */
#define rfbEncodingXvp 			 0xFFFFFECB

/*
 * Special encoding numbers:
 *   0xFFFFFD00 .. 0xFFFFFD05 -- subsampling level
 *   0xFFFFFE00 .. 0xFFFFFE64 -- fine-grained quality level (0-100 scale)
 *   0xFFFFFF00 .. 0xFFFFFF0F -- encoding-specific compression levels;
 *   0xFFFFFF10 .. 0xFFFFFF1F -- mouse cursor shape data;
 *   0xFFFFFF20 .. 0xFFFFFF2F -- various protocol extensions;
 *   0xFFFFFF30 .. 0xFFFFFFDF -- not allocated yet;
 *   0xFFFFFFE0 .. 0xFFFFFFEF -- quality level for JPEG compressor;
 *   0xFFFFFFF0 .. 0xFFFFFFFF -- cross-encoding compression levels.
 */

#define rfbEncodingFineQualityLevel0   0xFFFFFE00
#define rfbEncodingFineQualityLevel100 0xFFFFFE64
#define rfbEncodingSubsamp1X           0xFFFFFD00
#define rfbEncodingSubsamp4X           0xFFFFFD01
#define rfbEncodingSubsamp2X           0xFFFFFD02
#define rfbEncodingSubsampGray         0xFFFFFD03
#define rfbEncodingSubsamp8X           0xFFFFFD04
#define rfbEncodingSubsamp16X          0xFFFFFD05

#define rfbEncodingCompressLevel0  0xFFFFFF00
#define rfbEncodingCompressLevel1  0xFFFFFF01
#define rfbEncodingCompressLevel2  0xFFFFFF02
#define rfbEncodingCompressLevel3  0xFFFFFF03
#define rfbEncodingCompressLevel4  0xFFFFFF04
#define rfbEncodingCompressLevel5  0xFFFFFF05
#define rfbEncodingCompressLevel6  0xFFFFFF06
#define rfbEncodingCompressLevel7  0xFFFFFF07
#define rfbEncodingCompressLevel8  0xFFFFFF08
#define rfbEncodingCompressLevel9  0xFFFFFF09

#define rfbEncodingXCursor         0xFFFFFF10
#define rfbEncodingRichCursor      0xFFFFFF11
#define rfbEncodingPointerPos      0xFFFFFF18

#define rfbEncodingLastRect           0xFFFFFF20
#define rfbEncodingNewFBSize          0xFFFFFF21

#define rfbEncodingQualityLevel0   0xFFFFFFE0
#define rfbEncodingQualityLevel1   0xFFFFFFE1
#define rfbEncodingQualityLevel2   0xFFFFFFE2
#define rfbEncodingQualityLevel3   0xFFFFFFE3
#define rfbEncodingQualityLevel4   0xFFFFFFE4
#define rfbEncodingQualityLevel5   0xFFFFFFE5
#define rfbEncodingQualityLevel6   0xFFFFFFE6
#define rfbEncodingQualityLevel7   0xFFFFFFE7
#define rfbEncodingQualityLevel8   0xFFFFFFE8
#define rfbEncodingQualityLevel9   0xFFFFFFE9


/* LibVNCServer additions.   We claim 0xFFFE0000 - 0xFFFE00FF */
#define rfbEncodingKeyboardLedState   0xFFFE0000
#define rfbEncodingSupportedMessages  0xFFFE0001
#define rfbEncodingSupportedEncodings 0xFFFE0002
#define rfbEncodingServerIdentity     0xFFFE0003


/*****************************************************************************
 *
 * Server -> client message definitions
 *
 *****************************************************************************/


/*-----------------------------------------------------------------------------
 * FramebufferUpdate - a block of rectangles to be copied to the framebuffer.
 *
 * This message consists of a header giving the number of rectangles of pixel
 * data followed by the rectangles themselves.  The header is padded so that
 * together with the type byte it is an exact multiple of 4 bytes (to help
 * with alignment of 32-bit pixels):
 */

typedef struct {
    uint8_t type;			/* always rfbFramebufferUpdate */
    uint8_t pad;
    uint16_t nRects;
    /* followed by nRects rectangles */
} rfbFramebufferUpdateMsg;

#define sz_rfbFramebufferUpdateMsg 4

/*
 * Each rectangle of pixel data consists of a header describing the position
 * and size of the rectangle and a type word describing the encoding of the
 * pixel data, followed finally by the pixel data.  Note that if the client has
 * not sent a SetEncodings message then it will only receive raw pixel data.
 * Also note again that this structure is a multiple of 4 bytes.
 */

typedef struct {
    rfbRectangle r;
    uint32_t encoding;	/* one of the encoding types rfbEncoding... */
} rfbFramebufferUpdateRectHeader;

#define sz_rfbFramebufferUpdateRectHeader (sz_rfbRectangle + 4)

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Supported Messages Encoding.  This encoding does not contain any pixel data.
 * Instead, it contains 2 sets of bitflags.  These bitflags indicate what messages
 * are supported by the server.
 * rect->w contains byte count
 */

typedef struct {
  uint8_t client2server[32]; /* maximum of 256 message types (256/8)=32 */
  uint8_t server2client[32]; /* maximum of 256 message types (256/8)=32 */
} rfbSupportedMessages;

#define sz_rfbSupportedMessages 64

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Supported Encodings Encoding.  This encoding does not contain any pixel data.
 * Instead, it contains a list of (uint32_t) Encodings supported by this server.
 * rect->w contains byte count
 * rect->h contains encoding count
 */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Server Identity Encoding.  This encoding does not contain any pixel data.
 * Instead, it contains a text string containing information about the server.
 * ie: "x11vnc: 0.8.1 lastmod: 2006-04-25 (libvncserver 0.9pre)\0"
 * rect->w contains byte count
 */


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Raw Encoding.  Pixels are sent in top-to-bottom scanline order,
 * left-to-right within a scanline with no padding in between.
 */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * KeyboardLedState Encoding.  The X coordinate contains the Locked Modifiers
 * so that a remote troubleshooter can identify that the users 'Caps Lock' 
 * is set...   (It helps a *lot* when the users are untrained)
 */
#define rfbKeyboardMaskShift        1
#define rfbKeyboardMaskCapsLock     2
#define rfbKeyboardMaskControl      4
#define rfbKeyboardMaskAlt          8
#define rfbKeyboardMaskMeta        16
#define rfbKeyboardMaskSuper       32
#define rfbKeyboardMaskHyper       64
#define rfbKeyboardMaskNumLock    128
#define rfbKeyboardMaskScrollLock 256
#define rfbKeyboardMaskAltGraph   512

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * CopyRect Encoding.  The pixels are specified simply by the x and y position
 * of the source rectangle.
 */

typedef struct {
    uint16_t srcX;
    uint16_t srcY;
} rfbCopyRect;

#define sz_rfbCopyRect 4


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RRE - Rise-and-Run-length Encoding.  We have an rfbRREHeader structure
 * giving the number of subrectangles following.  Finally the data follows in
 * the form [<bgpixel><subrect><subrect>...] where each <subrect> is
 * [<pixel><rfbRectangle>].
 */

typedef struct {
    uint32_t nSubrects;
} rfbRREHeader;

#define sz_rfbRREHeader 4


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * CoRRE - Compact RRE Encoding.  We have an rfbRREHeader structure giving
 * the number of subrectangles following.  Finally the data follows in the form
 * [<bgpixel><subrect><subrect>...] where each <subrect> is
 * [<pixel><rfbCoRRERectangle>].  This means that
 * the whole rectangle must be at most 255x255 pixels.
 */

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
} rfbCoRRERectangle;

#define sz_rfbCoRRERectangle 4


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Hextile Encoding.  The rectangle is divided up into "tiles" of 16x16 pixels,
 * starting at the top left going in left-to-right, top-to-bottom order.  If
 * the width of the rectangle is not an exact multiple of 16 then the width of
 * the last tile in each row will be correspondingly smaller.  Similarly if the
 * height is not an exact multiple of 16 then the height of each tile in the
 * final row will also be smaller.  Each tile begins with a "subencoding" type
 * byte, which is a mask made up of a number of bits.  If the Raw bit is set
 * then the other bits are irrelevant; w*h pixel values follow (where w and h
 * are the width and height of the tile).  Otherwise the tile is encoded in a
 * similar way to RRE, except that the position and size of each subrectangle
 * can be specified in just two bytes.  The other bits in the mask are as
 * follows:
 *
 * BackgroundSpecified - if set, a pixel value follows which specifies
 *    the background colour for this tile.  The first non-raw tile in a
 *    rectangle must have this bit set.  If this bit isn't set then the
 *    background is the same as the last tile.
 *
 * ForegroundSpecified - if set, a pixel value follows which specifies
 *    the foreground colour to be used for all subrectangles in this tile.
 *    If this bit is set then the SubrectsColoured bit must be zero.
 *
 * AnySubrects - if set, a single byte follows giving the number of
 *    subrectangles following.  If not set, there are no subrectangles (i.e.
 *    the whole tile is just solid background colour).
 *
 * SubrectsColoured - if set then each subrectangle is preceded by a pixel
 *    value giving the colour of that subrectangle.  If not set, all
 *    subrectangles are the same colour, the foreground colour;  if the
 *    ForegroundSpecified bit wasn't set then the foreground is the same as
 *    the last tile.
 *
 * The position and size of each subrectangle is specified in two bytes.  The
 * Pack macros below can be used to generate the two bytes from x, y, w, h,
 * and the Extract macros can be used to extract the x, y, w, h values from
 * the two bytes.
 */

#define rfbHextileRaw			(1 << 0)
#define rfbHextileBackgroundSpecified	(1 << 1)
#define rfbHextileForegroundSpecified	(1 << 2)
#define rfbHextileAnySubrects		(1 << 3)
#define rfbHextileSubrectsColoured	(1 << 4)

#define rfbHextilePackXY(x,y) (((x) << 4) | (y))
#define rfbHextilePackWH(w,h) ((((w)-1) << 4) | ((h)-1))
#define rfbHextileExtractX(byte) ((byte) >> 4)
#define rfbHextileExtractY(byte) ((byte) & 0xf)
#define rfbHextileExtractW(byte) (((byte) >> 4) + 1)
#define rfbHextileExtractH(byte) (((byte) & 0xf) + 1)

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * zlib - zlib compressed Encoding.  We have an rfbZlibHeader structure
 * giving the number of bytes following.  Finally the data follows is
 * zlib compressed version of the raw pixel data as negotiated.
 * (NOTE: also used by Ultra Encoding)
 */

typedef struct {
    uint32_t nBytes;
} rfbZlibHeader;

#define sz_rfbZlibHeader 4

#ifdef LIBVNCSERVER_HAVE_LIBZ

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Tight and TightPng Encoding.
 *
 *-- TightPng is like Tight but basic compression is not used, instead PNG
 *   data is sent.
 *
 *-- The first byte of each Tight-encoded rectangle is a "compression control
 *   byte". Its format is as follows (bit 0 is the least significant one):
 *
 *   bit 0:    if 1, then compression stream 0 should be reset;
 *   bit 1:    if 1, then compression stream 1 should be reset;
 *   bit 2:    if 1, then compression stream 2 should be reset;
 *   bit 3:    if 1, then compression stream 3 should be reset;
 *   bits 7-4: if 1000 (0x08), then the compression type is "fill",
 *             if 1001 (0x09), then the compression type is "jpeg",
 *             (Tight only) if 1010 (0x0A), then the compression type is
 *               "basic" and no Zlib compression was used,
 *             (Tight only) if 1110 (0x0E), then the compression type is
 *               "basic", no Zlib compression was used, and a "filter id" byte
 *               follows this byte,
 *             (TightPng only) if 1010 (0x0A), then the compression type is
 *               "png",
 *             if 0xxx, then the compression type is "basic" and Zlib
 *               compression was used,
 *             values greater than 1010 are not valid.
 *
 * If the compression type is "basic" and Zlib compression was used, then bits
 * 6..4 of the compression control byte (those xxx in 0xxx) specify the
 * following:
 *
 *   bits 5-4:  decimal representation is the index of a particular zlib
 *              stream which should be used for decompressing the data;
 *   bit 6:     if 1, then a "filter id" byte is following this byte.
 *
 *-- The data that follows after the compression control byte described
 * above depends on the compression type ("fill", "jpeg", "png" or "basic").
 *
 *-- If the compression type is "fill", then the only pixel value follows, in
 * client pixel format (see NOTE 1). This value applies to all pixels of the
 * rectangle.
 *
 *-- If the compression type is "jpeg" or "png", the following data stream
 * looks like this:
 *
 *   1..3 bytes:  data size (N) in compact representation;
 *   N bytes:     JPEG or PNG image.
 *
 * Data size is compactly represented in one, two or three bytes, according
 * to the following scheme:
 *
 *  0xxxxxxx                    (for values 0..127)
 *  1xxxxxxx 0yyyyyyy           (for values 128..16383)
 *  1xxxxxxx 1yyyyyyy zzzzzzzz  (for values 16384..4194303)
 *
 * Here each character denotes one bit, xxxxxxx are the least significant 7
 * bits of the value (bits 0-6), yyyyyyy are bits 7-13, and zzzzzzzz are the
 * most significant 8 bits (bits 14-21). For example, decimal value 10000
 * should be represented as two bytes: binary 10010000 01001110, or
 * hexadecimal 90 4E.
 *
 *-- If the compression type is "basic" and bit 6 of the compression control
 * byte was set to 1, then the next (second) byte specifies "filter id" which
 * tells the decoder what filter type was used by the encoder to pre-process
 * pixel data before the compression. The "filter id" byte can be one of the
 * following:
 *
 *   0:  no filter ("copy" filter);
 *   1:  "palette" filter;
 *   2:  "gradient" filter.
 *
 *-- If bit 6 of the compression control byte is set to 0 (no "filter id"
 * byte), or if the filter id is 0, then raw pixel values in the client
 * format (see NOTE 1) will be compressed. See below details on the
 * compression.
 *
 *-- The "gradient" filter pre-processes pixel data with a simple algorithm
 * which converts each color component to a difference between a "predicted"
 * intensity and the actual intensity. Such a technique does not affect
 * uncompressed data size, but helps to compress photo-like images better. 
 * Pseudo-code for converting intensities to differences is the following:
 *
 *   P[i,j] := V[i-1,j] + V[i,j-1] - V[i-1,j-1];
 *   if (P[i,j] < 0) then P[i,j] := 0;
 *   if (P[i,j] > MAX) then P[i,j] := MAX;
 *   D[i,j] := V[i,j] - P[i,j];
 *
 * Here V[i,j] is the intensity of a color component for a pixel at
 * coordinates (i,j). MAX is the maximum value of intensity for a color
 * component.
 *
 *-- The "palette" filter converts true-color pixel data to indexed colors
 * and a palette which can consist of 2..256 colors. If the number of colors
 * is 2, then each pixel is encoded in 1 bit, otherwise 8 bits is used to
 * encode one pixel. 1-bit encoding is performed such way that the most
 * significant bits correspond to the leftmost pixels, and each raw of pixels
 * is aligned to the byte boundary. When "palette" filter is used, the
 * palette is sent before the pixel data. The palette begins with an unsigned
 * byte which value is the number of colors in the palette minus 1 (i.e. 1
 * means 2 colors, 255 means 256 colors in the palette). Then follows the
 * palette itself which consist of pixel values in client pixel format (see
 * NOTE 1).
 *
 *-- The pixel data is compressed using the zlib library. But if the data
 * size after applying the filter but before the compression is less then 12,
 * then the data is sent as is, uncompressed. Four separate zlib streams
 * (0..3) can be used and the decoder should read the actual stream id from
 * the compression control byte (see NOTE 2).
 *
 * If the compression is not used, then the pixel data is sent as is,
 * otherwise the data stream looks like this:
 *
 *   1..3 bytes:  data size (N) in compact representation;
 *   N bytes:     zlib-compressed data.
 *
 * Data size is compactly represented in one, two or three bytes, just like
 * in the "jpeg" compression method (see above).
 *
 *-- NOTE 1. If the color depth is 24, and all three color components are
 * 8-bit wide, then one pixel in Tight encoding is always represented by
 * three bytes, where the first byte is red component, the second byte is
 * green component, and the third byte is blue component of the pixel color
 * value. This applies to colors in palettes as well.
 *
 *-- NOTE 2. The decoder must reset compression streams' states before
 * decoding the rectangle, if some of bits 0,1,2,3 in the compression control
 * byte are set to 1. Note that the decoder must reset zlib streams even if
 * the compression type is "fill", "jpeg" or "png".
 *
 *-- NOTE 3. The "gradient" filter and "jpeg" compression may be used only
 * when bits-per-pixel value is either 16 or 32, not 8.
 *
 *-- NOTE 4. The width of any Tight-encoded rectangle cannot exceed 2048
 * pixels. If a rectangle is wider, it must be split into several rectangles
 * and each one should be encoded separately.
 *
 */

#define rfbTightExplicitFilter         0x04
#define rfbTightFill                   0x08
#define rfbTightJpeg                   0x09
#define rfbTightNoZlib                 0x0A
#define rfbTightPng                    0x0A
#define rfbTightMaxSubencoding         0x0A

/* Filters to improve compression efficiency */
#define rfbTightFilterCopy             0x00
#define rfbTightFilterPalette          0x01
#define rfbTightFilterGradient         0x02

#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * h264 - h264 encoding.  We have an rfbH264Header structure
 * giving the number of bytes following.  Finally the data follows is
 * h264 encoded frame.
 */

typedef struct {
    uint32_t nBytes;
	uint32_t slice_type;
	uint32_t width;
	uint32_t height;
} rfbH264Header;

#define sz_rfbH264Header 16

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * XCursor encoding. This is a special encoding used to transmit X-style
 * cursor shapes from server to clients. Note that for this encoding,
 * coordinates in rfbFramebufferUpdateRectHeader structure hold hotspot
 * position (r.x, r.y) and cursor size (r.w, r.h). If (w * h != 0), two RGB
 * samples are sent after header in the rfbXCursorColors structure. They
 * denote foreground and background colors of the cursor. If a client
 * supports only black-and-white cursors, it should ignore these colors and
 * assume that foreground is black and background is white. Next, two bitmaps
 * (1 bits per pixel) follow: first one with actual data (value 0 denotes
 * background color, value 1 denotes foreground color), second one with
 * transparency data (bits with zero value mean that these pixels are
 * transparent). Both bitmaps represent cursor data in a byte stream, from
 * left to right, from top to bottom, and each row is byte-aligned. Most
 * significant bits correspond to leftmost pixels. The number of bytes in
 * each row can be calculated as ((w + 7) / 8). If (w * h == 0), cursor
 * should be hidden (or default local cursor should be set by the client).
 */

typedef struct {
    uint8_t foreRed;
    uint8_t foreGreen;
    uint8_t foreBlue;
    uint8_t backRed;
    uint8_t backGreen;
    uint8_t backBlue;
} rfbXCursorColors;

#define sz_rfbXCursorColors 6


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RichCursor encoding. This is a special encoding used to transmit cursor
 * shapes from server to clients. It is similar to the XCursor encoding but
 * uses client pixel format instead of two RGB colors to represent cursor
 * image. For this encoding, coordinates in rfbFramebufferUpdateRectHeader
 * structure hold hotspot position (r.x, r.y) and cursor size (r.w, r.h).
 * After header, two pixmaps follow: first one with cursor image in current
 * client pixel format (like in raw encoding), second with transparency data
 * (1 bit per pixel, exactly the same format as used for transparency bitmap
 * in the XCursor encoding). If (w * h == 0), cursor should be hidden (or
 * default local cursor should be set by the client).
 */


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ZRLE - encoding combining Zlib compression, tiling, palettisation and
 * run-length encoding.
 */

typedef struct {
    uint32_t length;
} rfbZRLEHeader;

#define sz_rfbZRLEHeader 4

#define rfbZRLETileWidth 64
#define rfbZRLETileHeight 64


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ZLIBHEX - zlib compressed Hextile Encoding.  Essentially, this is the
 * hextile encoding with zlib compression on the tiles that can not be
 * efficiently encoded with one of the other hextile subencodings.  The
 * new zlib subencoding uses two bytes to specify the length of the
 * compressed tile and then the compressed data follows.  As with the
 * raw sub-encoding, the zlib subencoding invalidates the other
 * values, if they are also set.
 */

#define rfbHextileZlibRaw		(1 << 5)
#define rfbHextileZlibHex		(1 << 6)
#define rfbHextileZlibMono		(1 << 7)


/*-----------------------------------------------------------------------------
 * SetColourMapEntries - these messages are only sent if the pixel
 * format uses a "colour map" (i.e. trueColour false) and the client has not
 * fixed the entire colour map using FixColourMapEntries.  In addition they
 * will only start being sent after the client has sent its first
 * FramebufferUpdateRequest.  So if the client always tells the server to use
 * trueColour then it never needs to process this type of message.
 */

typedef struct {
    uint8_t type;			/* always rfbSetColourMapEntries */
    uint8_t pad;
    uint16_t firstColour;
    uint16_t nColours;

    /* Followed by nColours * 3 * uint16_t
       r1, g1, b1, r2, g2, b2, r3, g3, b3, ..., rn, bn, gn */

} rfbSetColourMapEntriesMsg;

#define sz_rfbSetColourMapEntriesMsg 6



/*-----------------------------------------------------------------------------
 * Bell - ring a bell on the client if it has one.
 */

typedef struct {
    uint8_t type;			/* always rfbBell */
} rfbBellMsg;

#define sz_rfbBellMsg 1



/*-----------------------------------------------------------------------------
 * ServerCutText - the server has new text in its cut buffer.
 */

typedef struct {
    uint8_t type;			/* always rfbServerCutText */
    uint8_t pad1;
    uint16_t pad2;
    uint32_t length;
    /* followed by char text[length] */
} rfbServerCutTextMsg;

#define sz_rfbServerCutTextMsg 8


/*-----------------------------------------------------------------------------
 * //  Modif sf@2002
 * FileTransferMsg - The client sends FileTransfer message.
 * Bidirectional message - Files can be sent from client to server & vice versa
 */

typedef struct _rfbFileTransferMsg {
    uint8_t type;			/* always rfbFileTransfer */
    uint8_t contentType;  /*  See defines below */
    uint8_t contentParam;/*  Other possible content classification (Dir or File name, etc..) */
    uint8_t pad;         /* It appears that UltraVNC *forgot* to Swap16IfLE(contentParam) */
    uint32_t size;		/*  FileSize or packet index or error or other  */
/*  uint32_t sizeH;		 Additional 32Bits params to handle big values. Only for V2 (we want backward compatibility between all V1 versions) */
    uint32_t length;
    /* followed by data char text[length] */
} rfbFileTransferMsg;

#define sz_rfbFileTransferMsg	12

#define rfbFileTransferVersion  2 /*  v1 is the old FT version ( <= 1.0.0 RC18 versions) */

/*  FileTransfer Content types and Params defines */
#define rfbDirContentRequest	1 /*  Client asks for the content of a given Server directory */
#define rfbDirPacket			2 /*  Full directory name or full file name. */
								  /*  Null content means end of Directory */
#define rfbFileTransferRequest	3 /*  Client asks the server for the transfer of a given file */
#define rfbFileHeader			4 /*  First packet of a file transfer, containing file's features */
#define rfbFilePacket			5 /*  One chunk of the file */
#define rfbEndOfFile			6 /*  End of file transfer (the file has been received or error) */
#define rfbAbortFileTransfer	7 /*  The file transfer must be aborted, whatever the state */
#define rfbFileTransferOffer	8 /*  The client offers to send a file to the server */
#define rfbFileAcceptHeader		9 /*  The server accepts or rejects the file */
#define rfbCommand				10 /*  The Client sends a simple command (File Delete, Dir create etc...) */
#define rfbCommandReturn		11 /*  The Client receives the server's answer about a simple command */
#define rfbFileChecksums		12 /*  The zipped checksums of the destination file (Delta Transfer) */
#define rfbFileTransferAccess	14 /*  Request FileTransfer authorization */

								/*  rfbDirContentRequest client Request - content params  */
#define rfbRDirContent			1 /*  Request a Server Directory contents */
#define rfbRDrivesList			2 /*  Request the server's drives list */
#define rfbRDirRecursiveList	3 /*  Request a server directory content recursive sorted list */
#define rfbRDirRecursiveSize	4 /*  Request a server directory content recursive size */

								/*  rfbDirPacket & rfbCommandReturn  server Answer - content params */
#define rfbADirectory			1 /*  Reception of a directory name */
#define rfbAFile				2 /*  Reception of a file name  */
#define rfbADrivesList			3 /*  Reception of a list of drives */
#define rfbADirCreate			4 /*  Response to a create dir command  */
#define rfbADirDelete			5 /*  Response to a delete dir command  */
#define rfbAFileCreate			6 /*  Response to a create file command  */
#define rfbAFileDelete			7 /*  Response to a delete file command  */
#define rfbAFileRename			8 /*  Response to a rename file command  */
#define rfbADirRename			9 /*  Response to a rename dir command  */
#define rfbADirRecursiveListItem	10 
#define rfbADirRecursiveSize		11 

								/*  rfbCommand Command - content params */
#define rfbCDirCreate			1 /*  Request the server to create the given directory */
#define rfbCDirDelete			2 /*  Request the server to delete the given directory */
#define rfbCFileCreate			3 /*  Request the server to create the given file */
#define rfbCFileDelete			4 /*  Request the server to delete the given file */
#define rfbCFileRename			5 /*  Request the server to rename the given file  */
#define rfbCDirRename			6 /*  Request the server to rename the given directory */

								/*  Errors - content params or "size" field */
#define rfbRErrorUnknownCmd     1  /*  Unknown FileTransfer command. */
#define rfbRErrorCmd			0xFFFFFFFF/*  Error when a command fails on remote side (ret in "size" field) */

#define sz_rfbBlockSize			8192  /*  Size of a File Transfer packet (before compression) */
#define rfbZipDirectoryPrefix   "!UVNCDIR-\0" /*  Transfered directory are zipped in a file with this prefix. Must end with "-" */
#define sz_rfbZipDirectoryPrefix 9 
#define rfbDirPrefix			"[ "
#define rfbDirSuffix			" ]"		



/*-----------------------------------------------------------------------------
 * Modif sf@2002
 * TextChatMsg - Utilized to order the TextChat mode on server or client
 * Bidirectional message
 */

typedef struct _rfbTextChatMsg {
    uint8_t type;			/* always rfbTextChat */
    uint8_t pad1;         /*  Could be used later as an additionnal param */
    uint16_t pad2;		/*  Could be used later as text offset, for instance */
    uint32_t length;      /*  Specific values for Open, close, finished (-1, -2, -3) */
    /* followed by char text[length] */
} rfbTextChatMsg;

#define sz_rfbTextChatMsg 8

#define rfbTextMaxSize		4096
#define rfbTextChatOpen		0xFFFFFFFF 
#define rfbTextChatClose	0xFFFFFFFE  
#define rfbTextChatFinished 0xFFFFFFFD  


/*-----------------------------------------------------------------------------
 * Xvp Message
 * Bidirectional message
 * A server which supports the xvp extension declares this by sending a message
 * with an Xvp_INIT xvp-message-code when it receives a request from the client
 * to use the xvp Pseudo-encoding. The server must specify in this message the
 * highest xvp-extension-version it supports: the client may assume that the
 * server supports all versions from 1 up to this value. The client is then
 * free to use any supported version. Currently, only version 1 is defined.
 *
 * A server which subsequently receives an xvp Client Message requesting an
 * operation which it is unable to perform, informs the client of this by
 * sending a message with an Xvp_FAIL xvp-message-code, and the same
 * xvp-extension-version as included in the client's operation request.
 *
 * A client supporting the xvp extension sends this to request that the server
 * initiate a clean shutdown, clean reboot or abrupt reset of the system whose
 * framebuffer the client is displaying.
 */


typedef struct {
    uint8_t type;			/* always rfbXvp */
	uint8_t pad;
	uint8_t version;	/* xvp extension version */
	uint8_t code;      	/* xvp message code */
} rfbXvpMsg;

#define sz_rfbXvpMsg (4)

/* server message codes */
#define rfbXvp_Fail 0
#define rfbXvp_Init 1
/* client message codes */
#define rfbXvp_Shutdown 2
#define rfbXvp_Reboot 3
#define rfbXvp_Reset 4


/*-----------------------------------------------------------------------------
 * Modif sf@2002
 * ResizeFrameBuffer - The Client must change the size of its framebuffer  
 */

typedef struct _rfbResizeFrameBufferMsg {
    uint8_t type;			/* always rfbResizeFrameBuffer */
	uint8_t pad1;
	uint16_t framebufferWidth;	/*  FrameBuffer width */
	uint16_t framebufferHeigth;	/*  FrameBuffer height */
} rfbResizeFrameBufferMsg;

#define sz_rfbResizeFrameBufferMsg 6


/*-----------------------------------------------------------------------------
 * Copyright (C) 2001 Harakan Software
 * PalmVNC 1.4 & 2.? ResizeFrameBuffer message
 * ReSizeFrameBuffer - tell the RFB client to alter its framebuffer, either
 * due to a resize of the server desktop or a client-requested scaling factor.
 * The pixel format remains unchanged.
 */

typedef struct {
    uint8_t type;			/* always rfbReSizeFrameBuffer */
	uint8_t pad1;
	uint16_t desktop_w;	/* Desktop width */
	uint16_t desktop_h;	/* Desktop height */
	uint16_t buffer_w;	/* FrameBuffer width */
	uint16_t buffer_h;	/* Framebuffer height */
    uint16_t pad2;

} rfbPalmVNCReSizeFrameBufferMsg;

#define sz_rfbPalmVNCReSizeFrameBufferMsg (12)


#ifdef LIBVNCSERVER_HAVE_ML_EXT


#define ML_EXT_MSG_PACKED  __attribute__ ((packed))
/**
 * 5 VNC MIRROR LINK EXTENSION MESSAGES
 */
/*-----------------------------------------------------------------------------
 * vnc mirror link extension messages
 * |--------|--------|--------|--------
 * | bytes  | Type   |  Value | Description
 * |--------|--------|--------|--------
 * | 1      | U8     |  128   | MirrorLink Message-type
 * |--------|--------|--------|--------
 * | 1      | U8     |        | Extension-type
 * |--------|--------|--------|--------
 * | 2      | U16    |        | Payload length
 * |--------|--------|--------|--------
 * | N      |U8 array|        | Message specific payload data
 * |--------|--------|--------|--------
 */

typedef struct {
    uint8_t type; /* always rfbMLExt (128) */
    uint8_t ext_type;
    uint16_t length;
    /* followed by uint8[length] which is Message specific payload data */
} ML_EXT_MSG_PACKED rfbMLExtMsg;

#define sz_rfbMLExtMsg 4

/**
 * |----------------|----------------|--------|----------------|--------------
 * | Extension-Type | Message Name   | Origin | Server Support | Client Support
 * |----------------|----------------|--------|----------------|--------------
 * | 0              | ByeBye         | Server | MUST           | MUST
 * | 0              | ByeBye         | Client | MUST           | MUST
 * | 1              | Server Display Configuration      | Server | MUST MUST
 * | 2              | Client Display Configuration      | Client | MUST MUST
 * | 3              | Server Event Configuration        | Server | MUST MUST
 * | 4              | Client Event Configuration        | Client | MUST MUST
 * | 5              | Event  Mapping                    | Server | MUST  MAY
 * | 6              | Event Mapping Request             | Client | MUST MAY
 * | 7              | Key Event Listing                 | Server | SHOULD MAY
 * | 8              | Key Event Listing Request         | Client | SHOULD MAY
 * | 9              | Virtual Keyboard Trigger          | Server | SHOULD MAY
 * | 10             | Virtual Keyboard Trigger Request  | Client | SHOULD MAY
 * | 11             | Device Status                     | Server | MUST MUST
 * | 12             | Device Status Request             | Client | MUST MUST
 * | 13             | Content Attestation Response      | Server | SHOULD SHOULD
 * | 14             | Content Attestation Request       | Client | SHOULD SHOULD
 * | 16             | Framebuffer Blocking Notification | Client | MUST MUST
 * | 18             | Audio Blocking Notification       | Client | MUST MUST
 * | 20             | Touch Event                       | Client | SHOULD SHOULD
 * | 21             | Framebuffer Alternative Text      | Server | MAY MAY
 * | 22             | Framebuffer Alternative Text Request | Client | MAY MAY
 */
#define rfbMLExt_ByeBye             0
#define rfbMLExt_ServerDispCfg      1
#define rfbMLExt_ClientDispCfg      2
#define rfbMLExt_ServerEvtCfg       3
#define rfbMLExt_ClientEvtCfg       4
#define rfbMLExt_EvtMapping         5
#define rfbMLExt_EvtMappingiReq     6
#define rfbMLExt_KeyEvtListing      7
#define rfbMLExt_KeyEvtListingReq   8
#define rfbMLExt_VKBTrigger         9
#define rfbMLExt_VKBTriggerReq      10
#define rfbMLExt_DeviceStatus       11
#define rfbMLExt_DeviceStatusReq    12
#define rfbMLExt_ContentAttestationRes  13
#define rfbMLExt_ContentAttestationReq  14
#define rfbMLExt_FBBlockingNotify       16
#define rfbMLExt_AudioBlockingNotify    18
#define rfbMLExt_TouchEvt               20
#define rfbMLExt_FBAlternativeText      21
#define rfbMLExt_FBAlternativeTextReq   22

/**
 * Server Display Configuration
 * Payload length: 12
 */
typedef struct {
    uint8_t major; /* MirrorLink Server Major Version */
    uint8_t minor; /* MirrorLink Server Minor Version */
    /**
     * Framebuffer configuration (1 = yes, 0 = no)
     * [0] : Server-side framebuffer orientation switch available
     *  The VNC Server MUST start in default orientation,
     *  given in the Server Init message.
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [1] : Server-side framebuffer rotation available
     *  The VNC Server MUST start with no rotation.
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [2] : Server-side framebuffer up-scaling available
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [3] : Server-side framebuffer down-scaling available
     * [5] : Server supports Framebuffer Alternative Text messages.
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     */
    uint16_t fb_cfg;
    /**
     * Relative pixel width (set to zero, if relative width not known)
     * <<1.3>>
     *       Deprecated. Shall be set to "1".
     */
    uint16_t relative_w;
    /**
     * Relative pixel Height (set to zero, if relative width not known)
     *
     * <<1.3>>
     *       Deprecated. Shall be set to "1".
     */
    uint16_t relative_h;
    /**
     * Pixel format support (1 = yes, 0 = no)
     * [0] : 32-bit ARGB 888 (mandatory support for VNC Server)
     * [7] : Any other 32-bit format
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [8] : 24-bit RGB 888
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [15] : Any other 24-bit format
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [16] : 16-bit RGB 565 (mandatory support for VNC Server)
     * [17] : 16-bit RGB 555 (15 bit color depth)
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [18] : 16-bit RGB 444 (12 bit color depth)
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [19] : 16-bit RGB 343 (10 bit color depth)
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [23] : Any other 16-bit format
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     * [24] : 16-bit single color (grayscale)
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     *   Client MUST use red_shift and red_mask to set gray range
     * [25] : 8-bit single color (grayscale)
     *   Client MUST use red_shift and red_mask to set gray range
     * <<1.3>>
     *       Deprecated. Shall be set to "0" (no support).
     */
    uint32_t format_support;
} ML_EXT_MSG_PACKED rfbMLExt_ServerDispCfg_t;

/**
 * Client Display Configuration
 * Payload length: 22
 */
typedef struct {
    uint8_t major; /* MirrorLink Client Major Version */
    uint8_t minor; /* MirrorLink Client Minor Version */
    /**
     * Framebuffer configuration (1 = yes, 0 = no)
     * [0] : Server-side framebuffer orientation switch used
     *   If enabled, the VNC Client MUST use the Device Status
     *   Request message (section 5.7)
     * [1] : Server-side framebuffer rotation used
     *   If enabled, the VNC Client MUST use the Device Status
     *   Request message (section 5.7)
     * [2] : Client-side framebuffer up-scaling available
     * [3] : Client-side framebuffer down-scaling available
     * [5] : Client supports Framebuffer Alternative Text messages
     */
    uint16_t fb_cfg;
    uint16_t c_disp_w; /* Client display width [pixel] */
    uint16_t c_disp_h; /* Client display height [pixel] */
    uint16_t c_disp_w_mm; /* Client display width [mm] */
    uint16_t c_disp_h_mm; /* Client display height [mm] */
    uint16_t distance_user; /* Distance user – Client display [mm] */

    /**
     * Pixel format support (1 = yes, 0 = no)
     * [0] : 32-bit ARGB 888
     * [8] : 24-bit RGB 888
     * [16]: 16-bit RGB 565
     * [17]: 16-bit RGB 555
     * [18]: 16-bit RGB 444
     * [19]: 16-bit RGB 343
     * [24]: 16-bit single color
     * [25]: 8-bit single color
     */
    uint32_t format_support;
    /**
     * Supported resize factors (1 = support, 0 = not support)
     * [0] : Resizing by factor of 1/1 (MUST BE ‘1’)
     * [1] : Resizing by factor of 1/2
     * [2] : Resizing by factor of 1/3
     * [3] : Resizing by factor of 1/4
     * [4] : Resizing by factor of 1/5
     * [5] : Resizing by factor of 1/6
     * [6] : Resizing by factor of 1/8
     * [7] : Resizing by factor of 1/10
     * [8] : Resizing by factor of 1/16
     * [9] : Resizing by factor of 1/32
     * [10]: Resizing by factor of 2/3
     * [11]: Resizing by factor of 3/4
     */
    uint32_t resize_factors;
} ML_EXT_MSG_PACKED rfbMLExt_ClientDispCfg_t;

/**
 * Server/Client Event Configuration
 * Payload length: 28
 */
typedef struct {
    /* Keyboard layout – Language code (according ISO 639-1) */
    uint16_t language;
    /* Keyboard layout – Country code (according ISO 3166-1 alpha-2) */
    uint16_t country_code;
    /* UI Language – Language code (according ISO 639-1) */
    uint16_t ui_language;
    /* UI Language – Country code (according ISO 3166-1 alpha-2) */
    uint16_t ui_country_code;

    /**
     * Knob keys (Bit mask according Table 40)
     * ‘1’: Server supports knob key events
     * ‘0’: Server does not support knob key events
     */
    uint32_t knob_keys_bits;

    /**
     * Device keys (Bit mask according Table 42)
     * ‘1’: Server supports device key events
     * ‘0’: Server does not support device key events
     */
    uint32_t device_keys_bits;
    /**
     * Multimedia keys (Bit mask according Table 42)
     * ‘1’: Server supports multimedia key events
     * ‘0’: Server does not support multimedia key events
     */
    uint32_t multimedia_keys_bits;
    /**
     * Key related (1 = support, 0 = no support)
     * [0] : ITU keypad (T9) events (‘0’, ... ,’9’, ‘#’, ‘*”)
     * <<1.3>>
     * Deprecated. Shall be set to "0" (no support).
     *
     * [1] : Virtual keyboard trigger support
     * <<1.3>>
     * Deprecated. Shall be set to "0" (no support).
     *
     * [2] : Key event listing support
     * <<1.3>>
     * Deprecated. Shall be set to "0" (no support).
     *
     * [3] : Event mapping support (MUST be ‘1’)
     * [15:8] : # additional Function keys, the server supports
     *   Key events start with Function_Key 0, no subsequent gaps
     */
    uint32_t key_related;
    /**
     * Pointer related (1 = support, 0 = no support)
     * [0] : Pointer events
     * [1] : Touch events
     * [15:8] : Pointer event button mask (according RFB spec)
     *   Must be 0x00 if the VNC Server does not support pointer
     *   events.
     * [23:16] : (Number of supported simultaneous events minus one)
     *   within a touch event, e.g. a value of 2 indicates a support of
     *   3 parallel events. Must be 0x00, if the VNC Server does not
     *   support touch events.
     * [31:24] : Touch event pressure mask, supported from the VNC
     *   Server. Must be 0x00, if the VNC Server does not support
     *   touch events.
     */
    uint32_t pointer_related;
} ML_EXT_MSG_PACKED rfbMLExt_EvtCft_t;

/**
 * Event Mapping / Mapping Request
 * Payload length: 8
 */
typedef struct {
    uint32_t c_key; /* Client Key Symbol Value */
    /**
     * Server Key Symbol Value
     *  (0 = client key value not mapped from server)
     *  (0 = request value from server)
     */
    uint32_t s_key;
} ML_EXT_MSG_PACKED rfbMLExt_EvtMapping_t;

/**
 * Key Event Listing
 * Payload length: 4 + 4*n
 */
typedef struct {
    /**
     * Configuration
     * [0] : Incremental flag (0 = non-incremental, 1 = incremental)
     * [1] : Listing flag (0 = black list, 1 = white list)
     * [2] : Default key event list flag
     *   ‘1’ – the key event list contains the default key event list.
     *   ‘0’ – the list contains a key event list update, either with
     *         respect to the previous update (incremental flag = 1) or
     *         with respect to the default list (incremental flag = 0).
     * [3] : Key event list follows flag (0 = last list, 1 = event list follows)
     */
    uint8_t cfg_bits;
    /* # key events in list */
    uint8_t key_evt_cnt;
    /* Key event counter */
    uint16_t key_evt_counter;

    /**
     * U32 array KeySymValue list used to define the next valid character
     */
    uint32_t key_sym_value[0];
} ML_EXT_MSG_PACKED rfbMLExt_KeyEvtListing_t;

/**
 * Key Event Listing Request
 * Payload length: 4
 */
typedef struct {
    /**
     * Configuration (0 = Disable, 1 = Enable)
     * [0] : Server key event listing
     *   ‘1’: Start event listing
     *   ‘0’: Stop event listing
     * [1] : Incremental updates supported from MirrorLink client
     * [2] : Reset key event counter
     *
     */
    uint32_t cfg_bits;
} ML_EXT_MSG_PACKED rfbMLExt_KeyEvtListingReq_t;

/**
 * Virtual Keyboard Trigger
 * Payload length: 16
 */
typedef struct {
    /**
     * Configuration (0 = no, 1 = yes)
     * [0] : Valid cursor position
     * [1] : Valid text input area
     * [2] : Key Event listing follows
     * [3] : Virtual keyboard control
     *   (0 = show keyboard, 1 = remove keyboard)
     * [4] : Text Entry Exchange
     *   (0 = not available, 1 = available)
     * [15:8] : Virtual Keyboard Type:
     *          0x00: Unknown
     *          0x01: QWERTY keyboard
     *          0x02: Numeric keyboard (including ‘+’, and ‘#’)
     */
    uint32_t cfg_bits;
    uint16_t cursor_x; /* Cursor – X Position */
    uint16_t cursor_y; /* Cursor – Y Position */
    uint16_t text_x; /* Text input area – X-Position */
    uint16_t text_y; /* Text input area – Y-Position */
    uint16_t text_w; /* Text input area – Width */
    uint16_t text_h; /* Text input area – Height */
} ML_EXT_MSG_PACKED rfbMLExt_VKBTrigger_t;

/**
 * Virtual Keyboard Trigger Request
 * Payload length: 4
 */
typedef struct {
    /**
     * Configuration (0 = no, 1 = yes)
     * [0] : Enable trigger
     * [1] : Support text entry exchange
     * [15:8] : Maximum number of characters supported as initial text. A
     *          zero value (0) indicates no length limitation.
     */
    uint32_t cfg_bits;
} ML_EXT_MSG_PACKED rfbMLExt_VKBTriggerReq_t;


/**
 * Device Status
 * Payload length: 4
 */
typedef struct {
    /**
     * Status of Device Features
     *  (00 = unknown, 01 = reserved, 10 = disabled, 11 = enabled)
     * [1:0] : Key-lock (Do not allow key and pointer event entry at the CE
     * device)
     * [3:2] : Device-lock
     * (In device-lock state, the MirrorLink Server is locked
     * and MAY NOT respond to remote key and pointer
     * events.
     * Note: The User MAY need to enter a PIN code to
     * un-lock the device; PIN code entry MAY NOT be
     * possible via the MirrorLink Client)
     * [5:4] : Screen saver
     * (Do not show content on server display, e.g. dim-
     * ming the display backlight)
     * [7:6] : Night mode
     * [9:8] : Voice control input on MirrorLink Server
     * [11:10] : Microphone input on MirrorLink Client 11
     * (Microphone input on MirrorLink Client (Enables or
     * disables the microphone input at the MirrorLink Cli-
     * ent).
     * The audio from the microphone SHOULD be treated
     * as conversational audio, if Voice Control Input sta-
     * tus is not enabled.
     * The audio from the microphone MUST be treated as
     * voice command audio, if Voice Control Input status
     * is enabled.
     * [17:16] : Driver Distraction Avoidance
     * (00 = unknown: MirrorLink Client display is not sub-
     * ject to driver distraction avoidance
     * 01 = reserved
     * 10 = disabled: MirrorLink Client display is subject to
     * driver distraction avoidance; MirrorLink Client is in
     * non-restricted driving mode
     * 11 = enabled: MirrorLink Client display is subject to
     * driver distraction avoidance; MirrorLink Client is in
     * restricted driving mode)
     * [26:24] : Absolute Framebuffer rotation (clock-wise)
     * (000 = unknown, 001, 010, 011 = reserved,
     * 100 = 0o, 101 = 90o, 110 = 180o, 111 = 270o)
     * [28:27] : Framebuffer orientation
     * (00 = unknown, 01 = reserved,
     * 10 = Landscape, 11 = Portrait)
     */
    uint32_t sts_bits;
} ML_EXT_MSG_PACKED rfbMLExt_DeviceStatus_t;

/**
 * Device Status Request
 * Payload length: 4
 */
typedef struct {
    /**
     * Status of Device Features
     * (00 = ignore, 01 = reserved 10 = disable, 11 = enable)
     *
     * [1:0] : Key-lock (block key entry on the device)
     * [3:2] : Device lock (bring device into device-lock state)
     * [5:4] : Screen saver (power-down the device screen)
     * [7:6] : Night mode (run device in night mode)
     * [9:8] : Voice input (route the incoming audio stream to a
     * voice recognition engine on the mobile device)
     * [11:10] : Microphone input on MirrorLink Client routed from
     * microphone to the MirrorLink server
     * [17:16] : Driver Distraction Avoidance
     * (MirrorLink Client is in restricted driving mode (ena-
     * bled), non-restricted driving mode (disabled) or does
     * not enforce a specific driving mode (ignore))
     * [26:24] : Absolute Framebuffer rotation (clock-wise)
     * (000 = ignore, 001, 010, 011 = reserved
     * 100 = 0o, 101 = 90o, 110 = 180o, 111 = 270o)
     * [28:27] : Framebuffer orientation
     * (00 = ignore, 01 = reserved,
     * 10 = Landscape, 11 = Portrait)
     */
    uint32_t sts_bits;
} ML_EXT_MSG_PACKED rfbMLExt_DeviceStatusReq_t;

/**
 * Content Attestation Response
 * Payload length: 122(Maximum size)
 */
typedef struct {
    /**
     * SignedInfo Flag
     * Defines, what has been attested and included into
     * the hash (‘1’ = include, ‘0’ = do not include)
     * Note, that the MirrorLink server MAY choose to attest
     * different content than what was requested by the cli-
     * ent, i.e. the SignedInfo flag set in Content Attestation
     * Response MAY be different from the one in Content
     * Attestation Request. It is up to the client to decide
     * whether such attestation is acceptable.
     *
     * [0] : SignedInfo includes context information pseudo en-
     * coding as provided within the last framebuffer update
     * Note: Signature is calculated over all context infor-
     * mation pseudo encoding rectangles (as defined in
     * Table 31) in the Framebuffer Update message, i.e.
     * excluding the 4 byte message header and any regu-
     * lar framebuffer encoding.
     *
     * [1] : SignedInfo includes the framebuffer content, as pro-
     * vided with the last framebuffer update
     * Note: Signature is calculated over all rectangles in
     * the Framebuffer Update message, i.e. excluding the
     * 4 byte message header and any pseudo encoded
     * rectangle (e.g. context information or desktop size).
     *
     * [2] : SignedInfo includes number of updated framebuffer
     * pixels sent since previous content attestation re-
     * sponse message
     * Note: Signature is calculated over N = ∑ j ∑ i width i ∙
     * height i , where i identifies the different rectangles
     * within the regular framebuffer update j, i.e. excluding
     * any pseudo encoded rectangle. N is a 32-bit un-
     * signed integer (in network byte-order).)
    */
    uint16_t signedinfo_bits;
    /**
     * Error code
     * 0 Success – no change to SignedInfo flag
     * 1 Success – with change to SignedInfo flag
     * 2 Success – no signature added, no change to SignedInfo flag
     * 3 Success – no signature added, with change to SignedInfo flag
     * 128 Error – no session key
     * 129 Error – content attestation not implemented
     * 255 Error – other error
     */
    uint16_t err_code;

    /**
     * 18 - 86
     * SignedInfo * (Size dependent of SignedInfo Flag)
     */
    uint8_t signed_info[86];
    /* (Optional) Signature */
    uint8_t signature[32];
#if 0
    /**
     * Nonce as provided by the MirrorLink client in Content Attestation Request (Table 24))
     */
    uint8_t nonce[16];
    /**
     * SignedInfo flag that defines the attested content.
     * The possible values are defined in Table 22
     */
    uint16_t signedinfo_flag;
    /**
     * (Optional) SHA-256 hash of context information
     * pseudo encoding, as provided within the last frame-
     * buffer update (as defined in Table 22)
     * Included if SignedInfo flag has bit 0 set.
     */
    uint8_t ctx_hash[32];
    /**
     * (Optional) SHA-256 hash of framebuffer content, as
     * provided with the last framebuffer update (as defined in Table 22)
     * Included if SignedInfo flag has bit 1 set.
     */
    uint8_t  fb_hash[32];
    /**
     * (Optional) Number of framebuffer pixels sent since
     * previous content attestation response message (asdefined in Table 22).
     * 32-bit unsigned integer in network byte order.
     * Included if SignedInfo flag has bit 2 set.
     */
    uint32_t number_fb_pixels;
#endif
} ML_EXT_MSG_PACKED rfbMLExt_ContentAttestationRes_t;

/**
 * Content Attestation Request
 * Payload length: 20 + N
 */
typedef struct {
    /* Random Nonce */
    uint8_t nonce[16];
    /**
     * Defines, what MUST be attested and included into the hash
     * (‘1’ = include, ‘0’ = do not include)
     * [0] Include context information pseudo encoding, as defined in Table 22
     * [1] Include last framebuffer update, as defined in Table 22
     * [2] Include number of pixels sent since previous content
     *     attestation response message, as defined in Table 22
     */
    uint16_t attested_bits;
    /**
     * Used signature type
     * 0 : No signature
     * 1 : The signature algorithm is HMAC-SHA-256 signature.
     *     The signed data is defined in Table 22.
     */
    uint8_t signature_type;
    /**
     * Used session key
     * 0 : No session key included
     * 1 : Random 128-bit symmetric session key that is en-
     * crypted using the application specific public key that
     * was bound to attestation of VNC Server. The encryp-
     * tion is done according to RSAPKCS#1 v1.5 format.
     * The session key is used from the MirrorLink server in
     * all subsequent Content Attestation Response mes-
     * sages, until a new key is provided in next Content At-
     * testation Request.
     */
    uint8_t used_session_key;
    /**
     * (Optional) Session key. The client MUST set session
     * key in the beginning of each session so that the
     * server does not have to remember previous session
     * keys and mapping of these keys to different client de-
     * vices.
     */
    uint8_t session_key[0];
} ML_EXT_MSG_PACKED rfbMLExt_ContentAttestationReq_t;

/**
 * Framebuffer Blocking Notification
 * Payload length: 14
 */
typedef struct {
    uint16_t x; /* X-position of rectangle (top left corner) */
    uint16_t y; /* Y-position of rectangle (top left corner) */
    uint16_t w; /* Width of rectangle */
    uint16_t h; /* Height of rectangle */
    /**
     * Unique application identifier
     *  (MUST be identical to the unique application ID provided
     *  within the Context Information Pseudo Encoding mes- sage)
     */
    uint32_t app_unique_id;
    /**
     * Reason for blocking (‘1’ = reason, ‘0’ no reason)
     * [0] Not allowed content category
     * [1] Not allowed application category
     * [2] Not sufficient content trust level
     * [3] Not sufficient application trust level
     * [4] Content rules not followed
     * [5] Not allowed application ID
     *     The MirrorLink Client MUST use this reason value, if
     *     it blocks an application for certification status reason
     *     (e.g. a non-drive application in drive mode).
     * [8] UI not in focus on remote display
     *     The MirrorLink Server MUST NOT change the appli-
     *     cation state and MUST keep the VNC session alive.
     * [9] UI not visible on remote display
     *     The MirrorLink Server MUST NOT change the appli-
     *     cation state and MUST keep the VNC session alive.
     * [10] UI layout not supported (after a Desktop Size
     *      Pseudo Encoding)
     */
    uint16_t reasion_bits;

} ML_EXT_MSG_PACKED rfbMLExt_FBBlockingNotify_t;

/**
 * Audio Blocking Notification
 * Payload length: 6
 */
typedef struct {
    /**
     * Unique application identifier
     * If zero, this identifies RTP streams, belonging to ap-
     * plications, not being advertised individually.
     * */
    uint32_t app_unique_id;
    /**
     * Reason for blocking (‘1’ = reason, ‘0’ no reason)
     * [0] Not allowed application category
     * [1] Not sufficient application trust level
     * [2] Not allowed application ID
     *     The MirrorLink Client MUST use this reason value, if
     *     it blocks an application for certification status reason
     *     (e.g. a non-drive application in drive mode).
     * [3] Global audio muted
     * [4] Audio stream, as given by application ID, muted
     */
    uint16_t reasion_bits;
} ML_EXT_MSG_PACKED rfbMLExt_AudioBlockingNotify_t;

/**
 * Touch Event
 * Payload length: 1 + N*6
 */
typedef struct {
    uint8_t evts_cnt; /* Number of individual events */
    /* Array of 6 bytes: Description of individual event */
    struct {
        uint16_t x; /* X-position of the individual event */
        uint16_t y; /* Y-position of the individual event */
        uint8_t id; /* Event identifier */

        /**
         * Pressure value A zero value (0) indicates a touch release event,
         *  A non-zero value indicates a touch press event,
         *  with the given pressure level.)
         */
        uint8_t pressure;
    } ML_EXT_MSG_PACKED evts[0];
} ML_EXT_MSG_PACKED rfbMLExt_TouchEvt_t;

/**
 * Framebuffer Alternative Text
 * Payload length: 6 + N
 */
typedef struct {
    /**
     * Unique application id.
     * Applications being advertised via UPnP, MUST match the advertised appID.
     */
    uint32_t app_unique_id;
    /**
     * Length of Textual Information.
     * A zero value invalidates any previous meta infor-
     * mation for that application.
     */
    uint16_t textual_len;
    /* Array of U8, Textual information (free text format) */
    uint8_t textual[0];
} ML_EXT_MSG_PACKED rfbMLExt_FBAlternativeText_t;

/**
 * Framebuffer Alternative Text Request
 * Payload length: 2
 */
typedef struct {
    /* Maximum length of the meta information. A zero length disables the feature.*/
    uint16_t max_textual_len;
} ML_EXT_MSG_PACKED rfbMLExt_FBAlternativeTextReq_t;

/**
 * Context Information Pseudo Encoding
 */
typedef struct {
    /**
     * Unique application identifier
     * For application being adver-tised via UPnP, the unique application id MUST match the advertised appID.
     * This field MAY be left empty (i.e. zero value).
     *
     */
    uint32_t app_unique_id;
    /* Trust Level for Application Category (see [22], Table 6-1) */
    uint16_t app_category_trust_level;
    /* Trust Level for Content Category (see [22], Table 6-1) */
    uint16_t content_category_trust_level;
    /* Application Category (see [22], Table 6-2) */
    uint32_t app_category;
    /* Content Category (see [22], Table 6-3) */
    uint32_t content_category;
    /** Content rules
     *  which are followed to prevent Driver Distraction.
     *  RuleIds are defined in [22], Table 6-5.
     *  [0] '1'. Rule with ruleId 0 supported. '0' otherwise
     *  [1] '1'. Rule with ruleId 1 supported. '0' otherwise
     *  ...
     *  [31] '1'. Rule with ruleId 31 supported. '0' otherwise
     */
    uint32_t content_rules_bits;
} ML_EXT_MSG_PACKED rfbMLExt_ContextInformation_t;

#define sz_rfbMLExtContextInformation 20

/**
 * 6 A DDITIONAL ENCODINGS AND PSEUDOENCODINGS
 */
/**
 * MirrorLink Encoding
 * Advertise the support of MirrorLink extension messages.
 * Not used within Framebuffer Update messages.
 * Mandatory
 */
#define rfbMLExt_PseudoEncoding_523 /* (-523) */ 0xFFFFFDF5

/**
 * Context Information
 * Indicate context information within a Framebuffer Update message
 * Mandatory
 */
#define rfbMLExt_PseudoEncoding_524 /* (-524) */ 0xFFFFFDF4

/**
 * Desktop Size
 * Change the VNC Server’s framebuffer resolution
 * Mandatory
 * same with rfbEncodingNewFBSize
 */
#define rfbMLExt_PseudoEncoding_223                                            \
    rfbEncodingNewFBSize /* (-223) 0xFFFFFF21                                  \
                            */

/**
 * Run-length-encoding
 * Scan line based run-length-encoding
 * Optional
 */
#define rfbMLExt_Encoding_525 /* (-525) */ 0xFFFFFDF3

/**
 * Transform Encoding
 * Framebuffer encoding, which includes a pixel format and a down-scale factor
 * used within the framebuffer update
 * Optional
 */
#define rfbMLExt_Encoding_526 /* (-526) */ 0xFFFFFDF2

#endif

/*-----------------------------------------------------------------------------
 * Union of all server->client messages.
 */

typedef union {
    uint8_t type;
    rfbFramebufferUpdateMsg fu;
    rfbSetColourMapEntriesMsg scme;
    rfbBellMsg b;
    rfbServerCutTextMsg sct;
	rfbResizeFrameBufferMsg rsfb;
	rfbPalmVNCReSizeFrameBufferMsg prsfb; 
	rfbFileTransferMsg ft;
	rfbTextChatMsg tc;
        rfbXvpMsg xvp;
} rfbServerToClientMsg;



/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RDV Cache Encoding.  
 * special is not used at this point, can be used to reset cache or other specials
 * just put it to make sure we don't have to change the encoding again.  
 */

typedef struct {
    uint16_t special;
} rfbCacheRect;

#define sz_rfbCacheRect 2




/*****************************************************************************
 *
 * Message definitions (client -> server)
 *
 *****************************************************************************/


/*-----------------------------------------------------------------------------
 * SetPixelFormat - tell the RFB server the format in which the client wants
 * pixels sent.
 */

typedef struct {
    uint8_t type;			/* always rfbSetPixelFormat */
    uint8_t pad1;
    uint16_t pad2;
    rfbPixelFormat format;
} rfbSetPixelFormatMsg;

#define sz_rfbSetPixelFormatMsg (sz_rfbPixelFormat + 4)


/*-----------------------------------------------------------------------------
 * FixColourMapEntries - when the pixel format uses a "colour map", fix
 * read-only colour map entries.
 *
 *    ***************** NOT CURRENTLY SUPPORTED *****************
 */

typedef struct {
    uint8_t type;			/* always rfbFixColourMapEntries */
    uint8_t pad;
    uint16_t firstColour;
    uint16_t nColours;

    /* Followed by nColours * 3 * uint16_t
       r1, g1, b1, r2, g2, b2, r3, g3, b3, ..., rn, bn, gn */

} rfbFixColourMapEntriesMsg;

#define sz_rfbFixColourMapEntriesMsg 6


/*-----------------------------------------------------------------------------
 * SetEncodings - tell the RFB server which encoding types we accept.  Put them
 * in order of preference, if we have any.  We may always receive raw
 * encoding, even if we don't specify it here.
 */

typedef struct {
    uint8_t type;			/* always rfbSetEncodings */
    uint8_t pad;
    uint16_t nEncodings;
    /* followed by nEncodings * uint32_t encoding types */
} rfbSetEncodingsMsg;

#define sz_rfbSetEncodingsMsg 4


/*-----------------------------------------------------------------------------
 * FramebufferUpdateRequest - request for a framebuffer update.  If incremental
 * is true then the client just wants the changes since the last update.  If
 * false then it wants the whole of the specified rectangle.
 */

typedef struct {
    uint8_t type;			/* always rfbFramebufferUpdateRequest */
    uint8_t incremental;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} rfbFramebufferUpdateRequestMsg;

#define sz_rfbFramebufferUpdateRequestMsg 10


/*-----------------------------------------------------------------------------
 * KeyEvent - key press or release
 *
 * Keys are specified using the "keysym" values defined by the X Window System.
 * For most ordinary keys, the keysym is the same as the corresponding ASCII
 * value.  Other common keys are:
 *
 * BackSpace		0xff08
 * Tab			0xff09
 * Return or Enter	0xff0d
 * Escape		0xff1b
 * Insert		0xff63
 * Delete		0xffff
 * Home			0xff50
 * End			0xff57
 * Page Up		0xff55
 * Page Down		0xff56
 * Left			0xff51
 * Up			0xff52
 * Right		0xff53
 * Down			0xff54
 * F1			0xffbe
 * F2			0xffbf
 * ...			...
 * F12			0xffc9
 * Shift		0xffe1
 * Control		0xffe3
 * Meta			0xffe7
 * Alt			0xffe9
 */

typedef struct {
    uint8_t type;			/* always rfbKeyEvent */
    uint8_t down;			/* true if down (press), false if up */
    uint16_t pad;
    uint32_t key;			/* key is specified as an X keysym */
} rfbKeyEventMsg;

#define sz_rfbKeyEventMsg 8


/*-----------------------------------------------------------------------------
 * PointerEvent - mouse/pen move and/or button press.
 */

typedef struct {
    uint8_t type;			/* always rfbPointerEvent */
    uint8_t buttonMask;		/* bits 0-7 are buttons 1-8, 0=up, 1=down */
    uint16_t x;
    uint16_t y;
} rfbPointerEventMsg;

#define rfbButton1Mask 1
#define rfbButton2Mask 2
#define rfbButton3Mask 4
#define rfbButton4Mask 8
#define rfbButton5Mask 16
/* RealVNC 335 method */
#define rfbWheelUpMask rfbButton4Mask
#define rfbWheelDownMask rfbButton5Mask

#define sz_rfbPointerEventMsg 6



/*-----------------------------------------------------------------------------
 * ClientCutText - the client has new text in its cut buffer.
 */

typedef struct {
    uint8_t type;			/* always rfbClientCutText */
    uint8_t pad1;
    uint16_t pad2;
    uint32_t length;
    /* followed by char text[length] */
} rfbClientCutTextMsg;

#define sz_rfbClientCutTextMsg 8



/*-----------------------------------------------------------------------------
 * sf@2002 - Set Server Scale
 * SetServerScale - Server must change the scale of the client buffer.
 */

typedef struct _rfbSetScaleMsg {
    uint8_t type;			/* always rfbSetScale */
    uint8_t scale;		/* Scale value 1<sv<n */
    uint16_t pad;
} rfbSetScaleMsg;

#define sz_rfbSetScaleMsg 4


/*-----------------------------------------------------------------------------
 * Copyright (C) 2001 Harakan Software
 * PalmVNC 1.4 & 2.? SetScale Factor message 
 * SetScaleFactor - tell the RFB server to alter the scale factor for the
 * client buffer.
 */
typedef struct {
    uint8_t type;			/* always rfbPalmVNCSetScaleFactor */

    uint8_t scale;		/* Scale factor (positive non-zero integer) */
    uint16_t pad2;
} rfbPalmVNCSetScaleFactorMsg;

#define sz_rfbPalmVNCSetScaleFactorMsg (4)


/*-----------------------------------------------------------------------------
 * rdv@2002 - Set input status
 * SetServerInput - Server input is dis/enabled
 */

typedef struct _rfbSetServerInputMsg {
    uint8_t type;			/* always rfbSetScale */
    uint8_t status;		/* Scale value 1<sv<n */
    uint16_t pad;
} rfbSetServerInputMsg;

#define sz_rfbSetServerInputMsg 4

/*-----------------------------------------------------------------------------
 * rdv@2002 - Set SW
 * SetSW - Server SW/full desktop
 */

typedef struct _rfbSetSWMsg {
    uint8_t type;			/* always rfbSetSW */
    uint8_t status;		
    uint16_t x;
    uint16_t y;
} rfbSetSWMsg;

#define sz_rfbSetSWMsg 6



/*-----------------------------------------------------------------------------
 * Union of all client->server messages.
 */

typedef union {
    uint8_t type;
    rfbSetPixelFormatMsg spf;
    rfbFixColourMapEntriesMsg fcme;
    rfbSetEncodingsMsg se;
    rfbFramebufferUpdateRequestMsg fur;
    rfbKeyEventMsg ke;
    rfbPointerEventMsg pe;
    rfbClientCutTextMsg cct;
	rfbSetScaleMsg ssc;
	rfbPalmVNCSetScaleFactorMsg pssf;
	rfbSetServerInputMsg sim;
	rfbFileTransferMsg ft;
	rfbSetSWMsg sw;
	rfbTextChatMsg tc;
        rfbXvpMsg xvp;
#ifdef LIBVNCSERVER_HAVE_ML_EXT
    rfbMLExtMsg ml;
#endif
} rfbClientToServerMsg;

/* 
 * vncauth.h - describes the functions provided by the vncauth library.
 */

#define MAXPWLEN 8
#define CHALLENGESIZE 16

extern int rfbEncryptAndStorePasswd(char *passwd, char *fname);
extern char *rfbDecryptPasswdFromFile(char *fname);
extern void rfbRandomBytes(unsigned char *bytes);
extern void rfbEncryptBytes(unsigned char *bytes, char *passwd);


#endif
