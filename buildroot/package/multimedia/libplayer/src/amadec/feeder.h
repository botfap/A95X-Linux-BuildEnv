/**
 * \file feeder.h
 * \brief  Some Macros for Feeder
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef FEEDER_H
#define FEEDER_H

#include <audio-dec.h>

ADEC_BEGIN_DECLS

#define FORMAT_PATH             "/sys/class/astream/format"
#define CHANNUM_PATH            "/sys/class/astream/channum"
#define SAMPLE_RATE_PATH        "/sys/class/astream/samplerate"
#define DATAWIDTH_PATH      "/sys/class/astream/datawidth"
#define AUDIOPTS_PATH           "/sys/class/astream/pts"

ADEC_END_DECLS

#endif
