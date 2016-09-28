// Copyright 2013 Yahoo! -- All rights reserved.
#ifndef __YCTV_MEDIA_SUPPORT_SAMPLE_H__
#define __YCTV_MEDIA_SUPPORT_SAMPLE_H__
#include <string>

struct SampleDisplay
{
	int   SessionID;
	float X;
	float Y;
	float Width;
	float Height;
};

struct SampleSound
{
	int   SessionID;
	float Volume;
};

struct SampleProgramInfo
{
	int         SessionID;
	int         InputID;
	int         ChannelNumberMajor;
	int         ChannelNumberMinor;
	std::string ChannelCallSign;
	std::string ChannelAffiliate;
	std::string VChipRating;
	std::string ClosedCaptionServices[16];
	std::string AnalogClosedCaptionServices[16];
	std::string Title;
	int         Size;
	std::string ShortDescription;
	int         Duration;
	int         VideoWidth;
	int         VideoHeight;
	int         AudioSampleRate;
	std::string AudioFormat;
	std::string AudioChannels[16];
};

struct SampleInput
{
	std::string   Name;
	int           InputID;
	int           SessionID;
	std::string   URI;
	int           DecoderID;
	SampleDisplay Display;
	SampleSound   Sound;
};

struct SampleDecoder
{
	std::string Name;
	int         DecoderID;
	int         SessionID;
	float       GlobalMute;
	bool        EnableClosedCaption;
	int         ClosedCaptionChannel;
	bool        EnableAnalogClosedCaption;
	int         AnalogClosedCaptionChannel;
	int         InputID;
	SampleInput Input;
	int         PlayCount;
	int         BufferPercentage;
};

struct SampleHotPlug
{
	std::string   Name;
	int           InputID;
	int           SessionID;
	bool          Connected;
	int           HotPlugType;
	int           PeripheralID;
	std::string   StorageRootPath;
};

#endif // #ifndef __YCTV_MEDIA_SUPPORT_SAMPLE_H__
