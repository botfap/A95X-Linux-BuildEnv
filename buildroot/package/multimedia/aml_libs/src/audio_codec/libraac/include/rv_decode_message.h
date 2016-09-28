/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv_decode_message.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc.
 * All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the Real Format Source Code
 * Porting and Optimization License, available at
 * https://helixcommunity.org/2005/license/realformatsource (unless
 * RealNetworks otherwise expressly agrees in writing that you are
 * subject to a different license).  You may also obtain the license
 * terms directly from RealNetworks.  You may not use this file except
 * in compliance with the Real Format Source Code Porting and
 * Optimization License. There are no redistribution rights for the
 * source code of this file. Please see the Real Format Source Code
 * Porting and Optimization License for the rights, obligations and
 * limitations governing use of the contents of the file.
 * 
 * RealNetworks is the developer of the Original Code and owns the
 * copyrights in the portions it created.
 * 
 * This file, and the files included with this file, is distributed and
 * made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL
 * SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT
 * OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 * https://rarvcode-tck.helixcommunity.org
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef RV_DECODE_MESSAGE_H
#define RV_DECODE_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/* Define CPU scalability constants.  Though these values range from */
/* 0 to 100, they are *not* intended to indicate a CPU utilization percentage. */

#define CPU_Scalability_Maximum  100
    /* Maximum value for CPU usage setting: */
    /* no shortcuts taken, best possible quality at the cost of higher */
    /* CPU usage.  This value is typically used for "off-line" compression. */

#define CPU_Scalability_Minimum    0
    /* Minimum value for CPU usage setting: */
    /* all implemented shortcuts taken, reduced CPU usage at the cost */
    /* of reduced video quality. */

#define CPU_Scalability_Default   50
    /* Default value for CPU usage setting: */
    /* this represents the normal mode of operation and is equivalent */
    /* to previously not choosing a CPU usage setting. */

#define RV89COMBO_MSG_ID_Decoder_CPU_Scalability 2002
    /* Allows for the setting and getting of the decoders CPU scalability parameter */

/* */
/* Define formats for representing custom codec messages. */
/* Custom codec messages have semantics unknown to the HIVE/RV layer. */
/* The messages are defined by the underlying codec, and typically documented */
/* for use by applications. */
/* */

/* RV_Custom_Message_ID identifies a specific custom message.  These */
/* identifiers are not guaranteed to be unique among different codecs. */
/* Thus an application using such messages, must have knowledge regarding */
/* which codec is being used, and what custom messages it supports. */
/* A codec will typically distribute a header file that defines its */
/* custom messages. */
/* */
/* Specific custom messages will be passed into a codec via a structure whose */
/* first member is a RV_Custom_Message_ID, and additional parameters */
/* understood by the application and the codec.  The size of each such */
/* custom message structure is variable, depending on the specific message id. */
/* Messages that require no values other than a message id, can be indicated */
/* by using the RV_Custom_Message_ID as is.  For such messages, there is no */
/* need to wrap the message id within another structure. */

typedef UINT32     RV_Custom_Message_ID;

/* RV_MSG_Simple is a structure used to pass custom messages which */
/* take one or two 32-bit values.  The "message_id" member identifies a */
/* particular message, and the "value1" and "value2" members specify the */
/* 32-bit values.  The interpretation of the values depends on the message id. */
/* They could be simple booleans (zero or non-zero), or arbitrary integers. */
/* Some messages will use only value1, and not value2. */

typedef struct {

    RV_Custom_Message_ID       message_id;

    INT32                      value1;
    INT32                      value2;

} RV_MSG_Simple;


/* The RV_MSG_DISABLE, RV_MSG_ENABLE and RV_MSG_GET values may be used as */
/* controls when manipulating boolean-valued custom messages.  Typically, */
/* such messages will employ the RV_MSG_Simple structure.  The message_id */
/* will identify a boolean-valued option being manipulated.  The "value1" */
/* member will be set to one of RV_MSG_DISABLE, RV_MSG_ENABLE or RV_MSG_GET. */
/* Depending on the message id, "value2" may specify additional information */
/* such as a layer number for scalable video sequences. */
/* */
/* When value1 is RV_MSG_DISABLE or RV_MSG_ENABLE, the specified option */
/* will be disabled or enabled, respectively. */
/* When value1 is RV_MSG_GET, the current setting for the option (zero for */
/* disabled and non-zero for enabled) will be returned in "value2". */

#define RV_MSG_DISABLE   0
#define RV_MSG_ENABLE    1
#define RV_MSG_GET       2

/* The RV_MSG_SET control, along with RV_MSG_GET, provide an alternative */
/* way in which custom messages might use the RV_MSG_Simple structure. */
/* When value1 is RV_MSG_SET, value2 indicates the value which is being */
/* applied.  The message_id provides the context to which value2 applies. */
/* Again, refer to the documentation describing each custom message to see */
/* if it uses RV_MSG_GET and RV_MSG_SET, or if it uses RV_MSG_DISABLE, */
/* RV_MSG_ENABLE and RV_MSG_GET. */

#define RV_MSG_SET       3





/* Decoder postfilters: */
/*  - the smoothing postfilter is designed to remove general compression */
/*    artifacts, mosquito noise, and ringing noise. */
/*  - the annex J deblocking filter when used as a postfilter (annex J */
/*    not encoded in bitstream), removes blocking artifacts (block */
/*    edges) almost as efficiently as when used in-the-loop, i.e. encoded */
/*    in bitstream. */
/*  For the most video quality improvement, both filters should be used. */
/*  However, each filter takes up a certain amount of CPU cycles, and */
/*  for large formats, it may be too computationally expensive to use */
/*  both. */
/*  These filters are more effective, the lower the bitrate. */

/* */
/* Define a custom decoder message for enabling the smoothing postfilter. */
/* */
/* This message must be sent with the RV_MSG_Simple structure. */
/* Its value1 member should be RV_MSG_DISABLE, RV_MSG_ENABLE or RV_MSG_GET. */
/* Its value2 is only used for RV_MSG_GET, in which case it is used to */
/* return the current setting. */
/* */
#define RV_MSG_ID_Smoothing_Postfilter 17

/* Define a decoder message that helps the decoder determine when to display */
/* frames. */
/* */
/* The SNR, spatial and temporal scalability features of H.263+ Annex O */
/* have the property that frames are received at the decoder out of order, */
/* or that multiple frames (enhancement layers) are received at the decoder */
/* yet only one of these frames should be displayed. */
/* */
/* In the absence of apriori knowledge regarding the video sequence it is */
/* going to receive, an H.263+ decoder might not know when it is okay to */
/* go ahead and display a picture it has just decoded. */
/* */
/* To help solve this problem, the ILVC decoder can operate in two modes. */
/* The first and default mode is termed "no latency".  In this scenario, */
/* the decoder assumes it will not be receiving B frames nor enhancement */
/* layers.  The decoder will decode and display the very first frame it sees. */
/* If the decoder encounters any B frames or enhancement layers, it then */
/* goes into latency mode (described below).  When B frames or enhancement */
/* layers are present under latency mode, some of these frame types will */
/* not be displayed by the decoder at the beginning of a video sequence. */
/* This is because the decoder either already displayed a frame for the */
/* same temporal reference point, or because the decoder no longer has */
/* the appropriate reference frames from which to decode a B frame. */
/* */
/* Under latency mode, the decoder will generally not display a decoded frame, */
/* until it has detected that no enhancement layer frames or dependent */
/* B frames will be coming.  This detection usually occurs when a subsequent */
/* non-B frame is encountered.  This mode has the advantage that no frames */
/* will be dropped at the beginning of a sequence.  The disadvantage is that */
/* a latency of one or more temporal references is introduced before a frame */
/* is displayed. */
/* */
/* This message must be sent with the RV_MSG_Simple structure. */
/* Its value1 member should be RV_MSG_DISABLE, RV_MSG_ENABLE or RV_MSG_GET. */
/* For RV_MSG_GET, the current setting is returned in value2. */

#define RV_MSG_ID_Latency_Display_Mode 21



/* */
/* Define a custom decoder message that enables/disables the error */
/* concealment in the decoder. */
/* */
/* This message must be sent with the RV_MSG_Simple structure. */
/* Its value1 member should be RV_MSG_DISABLE, RV_MSG_ENABLE or RV_MSG_GET. */
/* Its value2 is only used for RV_MSG_GET, in which case it is used to */
/* return the current setting. */
/* */
#define RV_MSG_ID_Error_Concealment  23


/* #define  RV_MSG_ID_* = 24; */
/*    // Obsolete, do not use 24. */
/* #define  RV_MSG_ID_* = 25; */
/*    // Obsolete, do not use 25. */

/* */
/* Define a custom decoder message to be used when decoding a non-compliant */
/* H.263+ bitstream, that informs the decoder about information which is */
/* normally gleaned from parsing the picture header.  In this scenario, the */
/* encoder is generating a bitstream having a lean and mean picture header. */
/* The invariant picture header contents are sent over the wire only once, */
/* prior to streaming any bitstreams. */
/* */
#define RV_MSG_ID_Set_Picture_Header_Invariants 26
/* */
/* This message must be sent with the RV_MSG_Simple structure. */
/* Its value1 member is interpreted as a bit mask, described below. */
/* Its value2 member is not used. */
/* */
/* The following bits should be set or cleared, as appropriate, to indicate */
/* which codec options will be enabled or disabled for the video sequence. */
/* If any of these change, then the changes should be communicated to the */
/* decoder via this custom message prior to it seeing any input frames */
/* having the new state. */
/* */


#define RV_MSG_SPHI_Slice_Structured_Mode                      0x20


/* */
/* Define messages that are used with the RealVideo bitstream, */
/* to communicate the locations of slice boundaries between the front-end */
/* codec and the back-end encoder or decoder. */
/* */

typedef struct
{
        UINT32      is_valid;
        UINT32      offset;
} RV_Segment_Info;
    /* The RV_Segment_Info structure *MUST* be structurally equivalent */
    /* to the PNCODEC_SEGMENTINFO structure defined in the RealVideo */
    /* front-end.  Typically an array of these structures is allocated, */
    /* with each element describing one slice in a compressed bit stream. */
    /* For the output of an encoder, is_valid is always true.  'offset' */
    /* indicates the offset of a slice within an associated bitstream. */
    /* The first slice is at offset 0. */

typedef struct {
    RV_Custom_Message_ID       message_id;
        /* message_id must be RV_MSG_ID_Get_Encode_Segment_Info */
        /* or RV_MSG_ID_Set_Decode_Segment_Info. */

    UINT32                     number_of_segments;

    RV_Segment_Info           *segment_info;
} RV_Segment_Info_MSG;


#define RV_MSG_ID_Set_Decode_Segment_Info 28
    /* This message, sent with the RV_Segment_Info_MSG structure, is used */
    /* to specify slice information to the decoder just prior to decoding */
    /* a bitstream. */
    /* number_of_segments should be set to one less than the number of */
    /* slices in the frame about to be decoded. */
    /* segment_info should point to an array of RV_Segment_Info */
    /* structures, which point to the slice offsets of the bitstream */
    /* that will be decoded next. */



/* */
/* Define a custom decoder message for setting the smoothing postfilter's */
/* strength.  The strength ranges from 0 to RV_Maximum_Smoothing_Strength, */
/* inclusive. */
/* */
/* This message must be sent with the RV_MSG_Simple structure. */
/* Its value1 member should be RV_MSG_SET or RV_MSG_GET. */
/* For RV_MSG_SET, value2 should be assigned the desired strength setting. */
/* For RV_MSG_GET, value2 is assigned upon return to the current strength */
/* setting. */
/* */
#define RV_MSG_ID_Smoothing_Strength     30

#define RV_Maximum_Smoothing_Strength     3
#define RV_Default_Smoothing_Strength     1



/* Define a message to set the number of sizes, as well as the list of sizes */
/* that are to be used for RealVideo-style RPR */
/* This information is transmitted in the SPO (once). The front-end uses */
/* the RV_MSG_ID_Set_RVDecoder_RPR_Data to communicate this information */
/* to the back-end. Then, if num_sizes != 0, the back-end will read the  */
/* following information from the slice header: */
/* .. */
/* [TR (as before)] */
/* if num_sizes != 0  */
/*      PCTSZ = getbits(log2(num_sizes)) */
/* [MBA (as before)] */
/* .. */


#define RV_MSG_ID_Set_RVDecoder_RPR_Sizes  36

typedef struct {

    RV_Custom_Message_ID    message_id;
        /* message_id must be RV_MSG_ID_Set_RVDecoder_RPR_Sizes. */

    UINT32                  num_sizes;
        /* The number of sizes to be used (switched between).  */
        /* This number should include the original image size. */
        /* The maximum number of sizes is currently limited to 8. */


    UINT32                 *sizes;
        /* Pointer to array of image sizes. The array should have the  */
        /* following format: */
        /* for (i = 0; i < num_sizes; i++) { */
        /*     UINT32 horizontal_size[i] */
        /*     UINT32 vertical_size[i] */
        /* } */
        /* This array should include the original image size */
        /* The order of sizes is not important. The decoder will */
        /* use PCTSZ as an index to look up in the array: */
        /* new_width = horizontal_size[PCTSZ] */
        /* new_height = vertical_size[PCTSZ] */

} RV_MSG_RVDecoder_RPR_Sizes;




/* Define a custom message to obtain timings for parts of the decode */
/* For now, only  the smoothing filte is interesting. */
/* More could be added later */

#define RV_MSG_ID_Get_Decoder_Timings  42
typedef struct {

    RV_Custom_Message_ID       message_id;

    double                     smoothing_time;
    double                     fru_time;
} RV_MSG_Get_Decoder_Timings;


/*  */
/* Define a custom message to set the video surface hardware */
/* allocator and its properties */

#define RV_MSG_ID_Hw_Video_Memory 43
/* */
/* This message must be sent with the RV_MSG_Simple structure. */
/* Its value1 member should be RV_MSG_DISABLE, RV_MSG_ENABLE or RV_MSG_GET. */
/* For RV_MSG_GET, the current number of frame buffers is returned in value2, */
/* and RV_MSG_ENABLE or RV_MSG_DISABLE returned in value1 */


/*  */
/* Define a custom message to perform postfiltering into video */
/* memory */
/* */
#define RV_MSG_ID_PostfilterFrame 44
typedef struct {

    RV_Custom_Message_ID        message_id;
    UINT8 *                     data;
    UINT8 *                     dest_buffer;
    UINT32                      dest_pitch;
    INT32                       cid_dest_color_format;
    UINT32                      flags;
} RV_MSG_PostfilterFrame;



/*  */
/* Define a custom message to release frame from video */
/* memory. */
/* 'data' is a pointer to a DecodedFrame structure */
/* */

#define RV_MSG_ID_ReleaseFrame 45
typedef struct {

    RV_Custom_Message_ID        message_id;
    UINT8 *                     data;
} RV_MSG_ReleaseFrame;


/*  */
/*  Define a custom message to retrieve latest DecodedFrame data */
/* */
#define RV_MSG_ID_RetrieveFrame 46
typedef struct {

    RV_Custom_Message_ID        message_id;
    void *                      frame;
} RV_MSG_RetrieveFrame;


/* */
/* Define a custom message to set the relied callback function */
/* that is the function to be called in the renderer in case */
/* it needs to blit (flip) a frame */
/* */

#define RV_MSG_ID_SetReliefCallback 48
typedef struct {

    RV_Custom_Message_ID        message_id;
    void *                      cmtm;
        /* pointer to PN_CMTM struct (pncodec.h) */
} RV_MSG_SetReliefCallback;

/* */
/* Define a custom message that enables/disables FRU */
/* */
#define RV_MSG_ID_Frame_Rate_Upsampling 49



/* */
/* Define a custom message to get CPU performance counters from the decoder. */
/* */
#define RV_MSG_ID_Get_Decoder_Performance_Counters  52

typedef struct {
    RV_Custom_Message_ID       message_id;
    UINT32                     uNumCounters;
    UINT32                    *pCounters;
} RV_MSG_Get_Decoder_Performance_Counters;


/* */
/* Define a custom message to enable and disable multithreading in the decoder */
/* */
#define RV_MSG_ID_Decoder_Multi_Threading 54

/* */
/* Define a custom message to enable decoding of beta streams with multiple threads */
/* */
#define RV_MSG_ID_Decoder_Beta_Stream 55

/* */
/* Define a custom message to signal RV8 bitstream */
/* */
#define RV_MSG_ID_RealVideo8 56

/*  */
/*  Define a custom message to retrieve latest DecodedFrame data */
/* */
#define RV_MSG_ID_RetrieveFrameData 57
typedef struct {

    RV_Custom_Message_ID          message_id;
    void *                        m_data;
    UINT8 *                       m_pYPlane;
    UINT8 *                       m_pUPlane;
    UINT8 *                       m_pVPlane;
    UINT32                        m_pitch;
    UINT32                        m_width;
    UINT32                        m_height;
} RV_MSG_RetrieveFrameData;



/* */
/* Note to developers: */
/* */
/* Next available message ID is 58. */
/* But 100 and above might already be in use, in "ccustmsg.h", so be careful. */
/* */

#ifdef __cplusplus
}
#endif

#endif /* RV_DECODE_MESSAGE_H */
