/***************************************************
 * name     : player_error.c
 * function : player error message
 * date     : 2010.3.23
 ***************************************************/

#include <player_error.h>

int system_error_to_player_error(int error)
{
    return error;
}
char * player_error_msg(int error)
{
    switch (error) {
    case PLAYER_SUCCESS:
        return "player no errors";
    case PLAYER_FAILED:
        return "error:player normal error";
    case PLAYER_NOMEM:
        return "error:can't allocate enough memory";
    case PLAYER_EMPTY_P:
        return "error:Invalid pointer";
    case PLAYER_NOT_VALID_PID:
        return "error:Invalid player id";
    case PLAYER_CAN_NOT_CREAT_THREADS:
        return "error: player create thread failed";
    case PLAYER_ERROR_PARAM:
        return "error:Invalid parameter for player";
    case PLAYER_RD_FAILED:
        return "error:player read file error";
    case PLAYER_RD_EMPTYP:
        return "error:invalid pointer when reading";
    case PLAYER_RD_TIMEOUT:
        return "error:no data for reading,time out";
    case PLAYER_RD_AGAIN:
        return "warning:no data, need read again";
    case PLAYER_WR_FAILED:
        return "error:player write data error";
    case PLAYER_WR_EMPTYP:
        return "error:invalid pointer when writing";
    case PLAYER_WR_FINISH:
        return "error:player write finish";
    case PLAYER_PTS_ERROR:
        return "error:player pts error";
    case PLAYER_NO_DECODER:
        return "error:can't find valid decoder";
    case DECODER_RESET_FAILED:
        return "error:decoder reset failed";
    case DECODER_INIT_FAILED:
        return "error:decoder init failed";
    case PLAYER_UNSUPPORT:
        return "error:player unsupport file type";
    case PLAYER_UNSUPPORT_VIDEO:
        return "warning:video format can't support yet";
    case PLAYER_UNSUPPORT_AUDIO:
        return "warning:audio format can't support yet";
    case PLAYER_SEEK_OVERSPILL:
        return "warning:seek time point overspill";
    case PLAYER_CHECK_CODEC_ERROR:
        return "error:check codec status error";
    case PLAYER_INVALID_CMD:
        return "warning:invalid command under current status";
    case PLAYER_REAL_AUDIO_FAILED:
        return "error: real audio failed";
    case PLAYER_ADTS_NOIDX:
        return "error:adts audio index invalid";
    case PLAYER_SEEK_FAILED:
        return "error:seek file failed";
    case PLAYER_NO_VIDEO:
        return "warning:file without video stream";
    case PLAYER_NO_AUDIO:
        return "warning:file without audio stream";
    case PLAYER_SET_NOVIDEO:
        return "warning:user set playback without video";
    case PLAYER_SET_NOAUDIO:
        return "warning:user set playback without audio";
    case PLAYER_UNSUPPORT_VCODEC:
        return "error:unsupport video codec";

    case FFMPEG_OPEN_FAILED:
        return "error:can't open input file";
    case FFMPEG_PARSE_FAILED:
        return "error:parse file failed";
    case FFMPEG_EMP_POINTER:
        return "error:check invalid pointer";
    case FFMPEG_NO_FILE:
        return "error:not assigned a file to play";

    default:
        return "error:invalid operate";
    }
}

