#ifndef _RTMP_HANDLER_H_
#define _RTMP_HANDLER_H_

#include <librtmp/rtmp.h>

#include "xutil.h"

#ifdef __cplusplus
extern "C" {
#endif

class VideoRawParser;
class AudioRawParser;

class RtmpHandler {
public:
    RtmpHandler();
    ~RtmpHandler();

    int connect(const std::string &liveurl);
    int disconnect();

    int send_video(int32_t timestamp, byte *dat, uint32_t length);
    int send_audio(int32_t timestamp, byte *dat, uint32_t length);

    bool send_rtmp_pkt(int pkttype, uint32_t ts,
                       const byte *buf, uint32_t pktsize);

private:
    struct DataInfo {
        int32_t lts;
        int32_t tm_offset;
        bool need_cfg;

        DataInfo() :
            lts(0), tm_offset(0), need_cfg(true) { }
    };

private:
    static int make_asc_body(const byte asc[], byte buf[], uint32_t len);
    static int make_audio_body(const byte *dat, uint32_t dat_len, byte buf[], uint32_t len);

    static int make_avc_dcr_body(byte *buf,
                                 const byte *sps, uint32_t sps_len,
                                 const byte *pps, uint32_t pps_len);
    static int make_video_body(byte *buf, uint32_t dat_len, bool key_frame);

    static byte pkttyp2channel(byte typ);

private:
    std::string m_url;

    RTMP *m_rtmp;

    VideoRawParser *m_vparser;
    AudioRawParser *m_aparser;

    DataInfo m_vinfo;
    DataInfo m_ainfo;

    xutil::MemHolder m_mem_pool;
};

#ifdef __cplusplus
}
#endif
#endif /* end of _RTMP_HANDLER_H_ */
