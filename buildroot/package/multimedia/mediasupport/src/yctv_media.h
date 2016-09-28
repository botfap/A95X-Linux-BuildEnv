// Copyright 2013 Yahoo! -- All rights reserved.
//
// See sample/config_media.json for configuraiton file sample.
// See sample/MediaSupportSample.cpp for implementation example.
#ifndef __YCTV_SDK_MEDIA_H__
#define __YCTV_SDK_MEDIA_H__
#include "Export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Callback function, triggers when a new event arrives.
// Data passed into this function is invalid after function
// returns.
typedef void (*YCTVMedia_OnReceiveEvent)( const char** json, const int stream_count );

typedef struct {

	#define MEDIA_MAX_STATES 10
	typedef enum {
		StatePlaying,    	// 0
		StatePaused,     	// 1
		StateStopped,    	// 2
		StateBuffering,  	// 3
		StateErrorFormat,   // 4
		StateErrorDecode,   // 5
		StateErrorResource, // 6
		StateErrorUnknown,  // 7
		StateSeeking,    	// 8
		StateEndOfStream 	// 9
	} State;

	#define MEDIA_OBJ_TYPES 15
	typedef enum {
		TypeDisplay,     // 0
		TypeSound,       // 1
		TypeProgramInfo, // 2
		TypeDecoder,     // 3
		TypeTuner,       // 4
		TypeHDMI,        // 5
		TypeConnector,   // 6
		TypeURI,         // 7
		TypeStorage,     // 8
		TypeRVU,         // 9
		TypePeripheral,  // 10
		TypeChannelMap,  // 11
		TypeTunerInfo,   // 12
		TypeDirInfo,     // 13
		TypeHotPlug      // 14
	} ObjectType;

	#define MEDIA_MAX_PROPERTIES_TYPES 4
	typedef enum {
		TypeInteger,     // 0
		TypeFloat,       // 1
		TypeBoolean,     // 2
		TypeList         // 3
	} PropertyType;

	YCTVMedia_OnReceiveEvent OnReceiveEvent; // See prototype above.
	const char*              ConfigFilePath; // JSON file specifying valid IDs
} YCTVMedia;

// Call this function first to set up callback function.
EXPORT bool YCTVMediaInit( const YCTVMedia& mdi, bool RunInThisThread = false );

// Call this function to pump events within your execution thread.
EXPORT bool YCTVMediaWaitForEvents( void );

// Call this function before exiting process.
EXPORT bool YCTVMediaShutdown( void );

// Call this function to send event to Yahoo!
EXPORT bool YCTVMediaSendEventToYahoo( const char** json, const int stream_count );

// Call this function to update playback time position in milliseconds.
EXPORT bool YCTVMediaUpdateTimeIndex( const int session_id, const int input_id, const long long time_ms );

// Call this function to send buffer progress as percentage between 0% to 100%.
EXPORT bool YCTVMediaUpdateBufferProgress( const int session_id, const int input_id, const int percentage );

// Call this function to send closed caption text.
EXPORT bool YCTVMediaUpdateClosedCaption( const int session_id, const int input_id, const char* cc_text );

// Call this function to send program info JSON object.
// See sample/JSON_DTD/ProgramInfo.txt for list of properties.
EXPORT bool YCTVMediaUpdateProgramInfo( const int session_id, const int input_id, const char* json );

// Call this function to send changes in decoder state.
EXPORT bool YCTVMediaUpdateDecoderState( const int session_id, const int input_id, const YCTVMedia::State, const char* status );

// Call this function to get the interface version.
EXPORT const char *YCTVMediaGetVersion( void );

#ifdef __cplusplus
}
#endif

#endif //#ifndef __YCTV_SDK_MEDIA_H__
