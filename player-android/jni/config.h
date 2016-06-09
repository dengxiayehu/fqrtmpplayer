#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VERSION_MESSAGE "0.1"

#define SOCK_TIMEOUT            30 // In seconds
#define RTMP_LOGLEVEL           RTMP_LOGDEBUG
#define RTMP_DEF_BUFTIME        (10*60*60*1000) // 10 hours default
#define RTMP_MAX_PLAY_BUFSIZE   (10*1024*1024) // 10M

#define NEW_STREAM_TIMESTAMP_THESHO 300

#endif /* end of _CONFIG_H_ */
