
#pragma once


#include <stdint.h>
namespace rtpserver{

constexpr char LOGGERNAME[] = "Debug";
#if (DEBUG)
constexpr char LOGGER_ERROR_NAME[] = "Debug_Error";
#else 
constexpr char LOGGERFILENAME[] = "RTPServer.txt";
#endif

constexpr uint16_t VIDEO_PORTBASE = 20000;
constexpr uint16_t AUDIO_PORTBASE = VIDEO_PORTBASE + 2;

constexpr char ENGINENAME[] = "RTPServer";

#define UNUSED(x) (void)x;
}
