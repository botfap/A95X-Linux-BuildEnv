// Copyright 2012-2013 Yahoo! -- All rights reserved.
// yctv_hacs.h: Host Audio Capturing Services Interface (C Interface)
// See sample/AudioCaptureSample.cpp for implementation example.
#ifndef __YCTV_SDK_HACS_H__
#define __YCTV_SDK_HACS_H__
#include "Export.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
** HACS Settings values for iHACS_GEOMETRY struct
*/

typedef enum iHACS_SAMPLE_RATE
{
	// Possible frequencies (each SP might have its own limitations!)
	GEOM_SR_8KHz        = 8000,
	GEOM_SR_11KHz       = 11025,
	GEOM_SR_16KHz       = 16000,
	GEOM_SR_22KHz       = 22050,
	GEOM_SR_32KHz       = 32000,
	GEOM_SR_44KHz_NTSC  = 44056,
	GEOM_SR_44KHz       = 44100,
	GEOM_SR_47KHz       = 47250,
	GEOM_SR_48KHz       = 48000,
	GEOM_SR_50KHz       = 50000,
	GEOM_SR_50KHz_MX80  = 50400,
	GEOM_SR_88KHz       = 88200,
	GEOM_SR_96KHz       = 96000,
	GEOM_SR_176KHz      = 176400,
	GEOM_SR_192KHz      = 192000,
	GEOM_SR_352KHz      = 352800,
	GEOM_SR_2822KHz     = 2822400,
	GEOM_SR_5644KHz     = 5644800

} iHACS_SAMPLE_RATE;

typedef enum iHACS_SAMPLE_DEPTH
{
	// Possible sample depth's
	GEOM_SD_S8          =  8,       // Signed 8  bits
	GEOM_SD_S16         = 16,       // Signed 16 bits
	GEOM_SD_S24         = 24,       // Signed 24 bits
	GEOM_SD_S32         = 32,       // Signed 32 bits
	GEOM_SD_S64         = 64,       // Signed 64 bits

	GEOM_SD_U8          =  8+1,     // Unsigned 8  bits
	GEOM_SD_U16         = 16+1,     // Unsigned 16 bits
	GEOM_SD_U24         = 24+1,     // Unsigned 24 bits
	GEOM_SD_U32         = 32+1,     // Unsigned 32 bits
	GEOM_SD_U64         = 64+1      // Unsigned 64 bits

} iHACS_SAMPLE_DEPTH;

typedef enum iHACS_DATA_TYPE
{
	// Possible data types
	GEOM_FORMAT_CHAR    = 0,        // Only 8 bits
	GEOM_FORMAT_SHORT   = 1,        // Only 16 bits
	GEOM_FORMAT_INT     = 2,        // Only 24/32/64 bits
	GEOM_FORMAT_FLOAT   = 3         // Only 24/32/64 bits

} iHACS_DATA_TYPE;

typedef enum iHACS_CHANNELS
{
	// Number of channels
	GEOM_CH_MONO        = 1,        // 1 Channel  / Linear
	GEOM_CH_STEREO      = 2,        // 2 Channels / Interleaved
	GEOM_CH_TWOONE      = 3,        // 3 Channels
	GEOM_CH_FOURONE     = 5         // 5 Channels

} iHACS_CHANNELS;

typedef enum iHACS_ENDIANESS
{
	// Sample Data Order
	GEOM_DT_HOST        = 0,        // To be determined by HOST
	GEOM_DT_LE          = 1,        // Non swapped (Little Endian)
	GEOM_DT_BE          = 2         // Swapped (Big Endian)

} iHACS_ENDIANESS;

typedef enum iHACS_SOURCES
{
	SRC_TV_TUNER        = 0,        // TV Tuner
	SRC_HDMI_1          = 1,        // HDMI Input 1
	SRC_HDMI_2          = 2,        // HDMI Input 2
	SRC_HDMI_3          = 3,        // HDMI Input 3
	SRC_HDMI_4          = 4,        // HDMI Input 4
	SRC_HDMI_5          = 5,        // HDMI Input 5
	SRC_HDMI_6          = 6,        // HDMI Input 6
	SRC_HDMI_7          = 7,        // HDMI Input 7
	SRC_HDMI_8          = 8,        // HDMI Input 8
	SRC_RCA             = 9,        // RCA Input
	SRC_PC              = 10,       // PC Audio Input
	SRC_IPTV            = 11,       // IPTV/Mediaplayer Input
	SRC_RVU             = 12,       // RVU Network Streaming
	SRC_OTHER           = 13        // Other Input (Always last!)

} iHACS_SOURCES;

/*
** Capture default/recommended geometry.
*/

#define IHACS_DEF_FHZ       GEOM_SR_8KHz
#define IHACS_DEF_BPS       GEOM_SD_S16
#define IHACS_DEF_TYP       GEOM_FORMAT_SHORT
#define IHACS_DEF_CHN       GEOM_CH_STEREO
#define IHACS_DEF_SEC       5
#define IHACS_MAX_SEC       10

/*
** IHACS C Interface definition.
*/

typedef enum iHACS_STAT
{
	iHACS_STAT_ERR        = 0,       // Fatal error, check logs for issues
	iHACS_STAT_OK         = 1,       // Samples pushed were captured correctly
	iHACS_STAT_BUSY       = 2,       // Interface busy, retry with next chunk
	iHACS_STAT_NOT_READY  = 3        // Interface not ready yet, retry later

} iHACS_STAT;

typedef struct iHACS_GEOMETRY
{
	int iSampleRate;
	int iSampleDepth;
	int iSampleType;
	int iChannels;
	int iSwapped;

} iHACS_GEOMETRY;

/*
   This function describes the geometry of the audio samples that will be provided
   by the hardware layer. If geometry changes, this function should be called as
   many times as necessary with the new geometry. This function returns true/false
   logic.

   iHACS_GEOMETRY ag                 Structure describing the geometry.
				  ag.iSampleRate     Sample rate definition (iHACS_SAMPLE_RATE).
				  ag.iSampleDepth    Sample bits size definition (iHACS_SAMPLE_DEPTH).
				  ag.iSampleType     Sample data type definition (iHACS_SAMPLE_TYPE).
				  ag.iChannels       Number of channels (iHACS_CHANNELS).
				  ag.iSwapped        Sample data endianess (iHACS_ENDIANESS).

				  Recommended values are:

				  ag.iSampleRate  = GEOM_SR_8KHz or GEOM_SR_48KHz
				  ag.iSampleDepth = GEOM_SD_S16
				  ag.iSampleType  = GEOM_FORMAT_SHORT
				  ag.iChannels    = GEOM_CH_STEREO
				  ag.iSwapped     = GEOM_DT_HOST

	iHACS_SOURCES srcDev             Input Source descriptor from were the audio is
									 being captured.
*/

EXPORT bool YCTVACSettings( iHACS_GEOMETRY ag, iHACS_SOURCES srcDev );

/*
   This function adds samples of audio to the pool for fingerprinting. It should
   be called every time data is available. The samples should be continuous and
   without gaps. This function returns true/false logic.

   const char *oBuffer                Pointer to buffer containing the samples.

   long oLength                       Length of data samples available in oBuffer.

   iHACS_STAT *oStat                  Returns status of last push operation.
									  See comments on iHACS_STAT typedef.
*/

EXPORT bool YCTVACPushData( const char *oBuffer, long oLength,  iHACS_STAT *oStat );

/*
   This function helps the capturing interface to reset and drop current operations
   when the device application knows that a change in the media audio has happened.
   It is recommended to be called when user changes channels or input source. This
   function returns true/false logic. This API allows to specify the source change
   for accuracy and to inform the application layer if things like prompts shall be
   hidden inmediatly. Special note on HDMI inputs: If from and to are the same input
   number, the system understands that it's an over the HDMI change detection and
   can not be 100% reliable. Otherwise, changes between HDMI's inputs are reliable.
   So are changes from and to the TV Tuner. Refer to the iHACS_SOURCES enum table
   for the definitions.
*/

EXPORT bool YCTVACSourceChanged( iHACS_SOURCES from, iHACS_SOURCES to );

/*
   This function shall be invoked when the platform wants to report volume state
   change. curVol to indicate the percentage, range 0%~100%. In muteStat oem shall
   report any change on the mute button status as well.
*/

EXPORT bool YCTVACVolumeChanged( int curVol, bool muteStat );

/*
   This function returns a constant string with the version of the HACS interface.
*/

EXPORT const char *YCTVACGetVersion( void );

/*
   This function should be called at the end of the consumer process to release
   resources acquired during the capture and fingerprinting process, as well as
   notify dependant processes. This function returns true/false logic.
*/

EXPORT bool YCTVACShutdown( void );

#ifdef __cplusplus
}
#endif

#endif //#ifndef __YCTV_SDK_HACS_H__
