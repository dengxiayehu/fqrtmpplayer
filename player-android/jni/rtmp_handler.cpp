#include <memory>

#include "rtmp_handler.h"
#include "raw_parser.h"
#include "jitter_buffer.h"
#include "xmedia.h"
#include "config.h"

//#define XDEBUG_TIMESTAMP

#define VIDEO_BODY_HEADER_LENGTH    16
#define VIDEO_PAYLOAD_OFFSET        5

using namespace xutil;

RtmpHandler::RtmpHandler() :
    m_rtmp(NULL),
    m_vparser(new VideoRawParser),
    m_aparser(new AudioRawParser),
    m_jitter(new JitterBuffer)
{
    struct PacketCallback pc = { this, packet_cb };
    m_jitter->set_packet_callback(pc);
}

RtmpHandler::~RtmpHandler()
{
    disconnect();

    SAFE_DELETE(m_vparser);
    SAFE_DELETE(m_aparser);
    SAFE_DELETE(m_jitter);
}

int RtmpHandler::connect(const std::string &liveurl)
{
    m_rtmp = RTMP_Alloc();
    if (!m_rtmp) {
        E("RTMP_Alloc() failed for liveurl: \"%s\"",
          liveurl.c_str());
        return -1;
    }

    RTMP_Init(m_rtmp);
    m_rtmp->Link.timeout = SOCK_TIMEOUT;
    m_rtmp->m_outChunkSize = RTMP_CHUNK_SIZE;

    RTMP_LogSetLevel(RTMP_LOGLEVEL);
    RTMP_LogSetCallback(rtmp_log);

    if (!RTMP_SetupURL(m_rtmp,
                       const_cast<char *>(liveurl.c_str()))) {
        E("RTMP_SetupURL() failed for liveurl: \"%s\"",
          liveurl.c_str());
        goto bail;
    }

    // Enable the ability of pushing flv to rtmpserver
    RTMP_EnableWrite(m_rtmp);

    if (!RTMP_Connect(m_rtmp, NULL)) {
        E("RTMP_Connect failed for liveurl: \"%s\"",
          liveurl.c_str());
        goto bail;
    }

    if (!RTMP_ConnectStream(m_rtmp, 0)) {
        E("RTMP_ConnectStream failed for liveurl: \"%s\"",
          liveurl.c_str());
        goto bail;
    }

    if (m_rtmp->m_outChunkSize != RTMP_DEFAULT_CHUNKSIZE) {
        byte buf[4];
        put_be32(buf, m_rtmp->m_outChunkSize);
        if (!send_rtmp_pkt(RTMP_PACKET_TYPE_CHUNK_SIZE, 0, buf, 4)) {
            E("Send chunk size %d to server failed",
              m_rtmp->m_outChunkSize);
            goto bail;
        }
    }

    m_url = liveurl;

    I("Connect to rtmp server with url \"%s\" ok",
      m_url.c_str());
    return 0;

bail:
    disconnect();
    return -1;
}

int RtmpHandler::disconnect()
{
    if (m_rtmp) {
        if (RTMP_IsConnected(m_rtmp)) {
            I("Try to disconnect from rtmp server.. (url: %s)",
              m_url.c_str());
        }

        RTMP_Close(m_rtmp);
        RTMP_Free(m_rtmp);
        m_rtmp = NULL;
    }
    return 0;
}

int RtmpHandler::send_video(int32_t timestamp, byte *dat, uint32_t length)
{
    if (m_vparser->process(dat, length) < 0) {
        E("Process video failed");
        return -1;
    }

    byte *buf = (byte *) m_mem_pool.alloc(
            length + VIDEO_BODY_HEADER_LENGTH + 128 /*Just in case*/);
    byte *cur = buf + VIDEO_PAYLOAD_OFFSET;

    for (uint32_t idx=0; idx<m_vparser->get_nalu_num(); ++idx) {
        const byte *nalu_dat =
            m_vparser->get_nalu_data(idx);
        uint32_t nalu_length =
            m_vparser->get_nalu_length(idx);

        // Check whether startcode is there, remove it
        if (STARTCODE4(nalu_dat)) {
            nalu_dat += 4;
            nalu_length -= 4;
        } else if (STARTCODE3(nalu_dat)) {
            nalu_dat += 3;
            nalu_length -= 3;
        }

        // (X) Ignore sps & pps for it will send in avc_dcr-pkt (Marked)
        // Add sps & pps in I-frame
#if 0
        byte nalu_typ = (*nalu_dat)&0x1F;
        if (nalu_typ == 7 || nalu_typ == 8) {
            // Skip sps&pps in I-frame
            continue;
        }
#endif

        // Put nalu-length(4 bytes) before every nalu
        put_be32(cur, nalu_length);
        memcpy(cur + 4, nalu_dat, nalu_length);
        cur += (4 + nalu_length);
    }

    if (timestamp - m_vinfo.lts < -NEW_STREAM_TIMESTAMP_THESHO) {
        // Take this as a new video stream
        int32_t lvabs_ts = m_vinfo.lts + m_vinfo.tm_offset;
        int32_t laabs_ts = m_ainfo.lts + m_ainfo.tm_offset;
        int32_t shift_ts = laabs_ts - lvabs_ts;

        // Magic number guess it
        if (abs(shift_ts) <= NEW_STREAM_TIMESTAMP_THESHO*30) {
            // Audio frame comes first after new stream starts
            if (shift_ts > 0) {
                // Shift video frame's timestamp
                m_vinfo.tm_offset += shift_ts;
            } else {
                // Shift audio frame's timestamp
                m_ainfo.tm_offset += shift_ts;
            }
        }

        // Starts at the same timestamp with previous frame's
        m_vinfo.tm_offset += m_vinfo.lts;

#ifdef XDEBUG_TIMESTAMP
        I("New video stream starts, atm_offset:%d, vtm_offset:%d",
          m_ainfo.tm_offset, m_vinfo.tm_offset);
#endif

        m_vinfo.need_cfg = true;
    }
    
    // Check whether need to send avc_dcr-pkt
    if (m_vinfo.need_cfg || m_vparser->sps_pps_changed()) {
        if (m_vparser->is_key_frame()) {
            byte avc_dcr_body[2048];
            int body_len = make_avc_dcr_body(avc_dcr_body,
                                             m_vparser->get_sps(), m_vparser->get_sps_length(),
                                             m_vparser->get_pps(), m_vparser->get_pps_length());
            if (!send_rtmp_pkt(RTMP_PACKET_TYPE_VIDEO, timestamp+m_vinfo.tm_offset,
                               avc_dcr_body, body_len)) {
                E("Send video avc_dcr to rtmpserver failed");
                return -1;
            }

            m_vinfo.need_cfg = false;
        }
    }

    m_vinfo.lts = timestamp;

    AutoLock _l(m_mutex);

    int body_len = make_video_body(buf, cur-buf, m_vparser->is_key_frame());
    if (!send_rtmp_pkt(RTMP_PACKET_TYPE_VIDEO, timestamp+m_vinfo.tm_offset,
                       buf, body_len)) {
        E("Send video data to rtmpserver failed");
        return -1;
    }

#ifdef XDEBUG_TIMESTAMP
    I("Video timestamp: %d", timestamp+m_vinfo.tm_offset);
#endif
    return 0;
}

int RtmpHandler::make_video_body(byte *buf, uint32_t dat_len, bool key_frame)
{
    uint32_t idx = 0;

    buf[idx++] = key_frame ? 0x17 : 0x27;

    buf[idx++] = 0x01;

    buf[idx++] = 0x00;    
    buf[idx++] = 0x00;
    buf[idx++] = 0x00;
    return dat_len;
}

int RtmpHandler::make_avc_dcr_body(byte *buf,
                                   const byte *sps, uint32_t sps_len,
                                   const byte *pps, uint32_t pps_len)
{
    uint32_t idx = 0;

    buf[idx++] = 0x17;

    buf[idx++] = 0x00;

    buf[idx++] = 0x00;
    buf[idx++] = 0x00;
    buf[idx++] = 0x00;

    buf[idx++] = 0x01;
    buf[idx++] = sps[1];
    buf[idx++] = sps[2];
    buf[idx++] = sps[3];
    buf[idx++] = 0xFF;
    buf[idx++] = 0xE1;
    buf[idx++] = (byte) ((sps_len>>8)&0xFF);
    buf[idx++] = (byte) (sps_len&0xFF);
    memcpy(buf+idx, sps, sps_len);
    idx += sps_len;

    buf[idx++] = 0x01;
    buf[idx++] = (byte) ((pps_len>>8)&0xFF);
    buf[idx++] = (byte) (pps_len&0xFF);
    memcpy(buf+idx, pps, pps_len);
    idx += pps_len;

#ifdef XDEBUG
    I("[avc_dcr] SPS: %02x %02x %02x %02x ... (%u bytes in total)",
      sps[0], sps[1], sps[2], sps[3], sps_len);
    I("[avc_dcr] PPS: %02x %02x %02x %02x ... (%u bytes in total)",
      pps[0], pps[1], pps[2], pps[3], pps_len);
#endif
    return idx;
}

int RtmpHandler::send_audio(int32_t timestamp,
        byte *dat, uint32_t length)
{
    if (m_aparser->process(dat, length) < 0) {
        E("Process audio failed");
        return -1;
    }

    // Need to send asc before audio data
    // If timestamp backwards greatly, take it as a new flv audio stream
    if (timestamp == 0 ||       // Usually first audio frame comes
        timestamp - m_ainfo.lts < -NEW_STREAM_TIMESTAMP_THESHO || // New flv audio stream
        m_ainfo.need_cfg        // Ask to re-send asc
        ) {
        byte asc_body[4]; // asc-pkt payload is 4 bytes fixed
        make_asc_body(m_aparser->get_asc(), asc_body, sizeof(asc_body));

#ifdef XDEBUG
        AudioSpecificConfig asc;
        memcpy(asc.dat, m_aparser->get_asc(), 2);
        print_asc(asc);
#endif

        if (timestamp - m_ainfo.lts < -NEW_STREAM_TIMESTAMP_THESHO) {
            // Take this as a new audio stream
            int32_t lvabs_ts = m_vinfo.lts + m_vinfo.tm_offset;
            int32_t laabs_ts = m_ainfo.lts + m_ainfo.tm_offset;
            int32_t shift_ts = laabs_ts - lvabs_ts;

            if (abs(shift_ts) <= NEW_STREAM_TIMESTAMP_THESHO*30) {
                // Audio frame comes first after new stream starts
                if (shift_ts > 0) {
                    // Shift video frame's timestamp
                    m_vinfo.tm_offset += shift_ts;
                } else {
                    // Shift audio frame's timestamp
                    m_ainfo.tm_offset += shift_ts;
                }
            }

            m_ainfo.tm_offset += m_ainfo.lts;

#ifdef XDEBUG_TIMESTAMP
            I("New audio stream starts, atm_offset:%d, vtm_offset:%d",
              m_ainfo.tm_offset, m_vinfo.tm_offset);
#endif
        }

        // NOTE: asc-pkt's timestamp is always 0
        if (!send_rtmp_pkt(RTMP_PACKET_TYPE_AUDIO, timestamp+m_ainfo.tm_offset,
                           asc_body, sizeof(asc_body))) {
            E("Send asc-pkt failed");
            m_ainfo.need_cfg = true;
            return -1;
        } else {
            // If asc-pkt is successfully sent, reset the flag
            m_ainfo.need_cfg = false;
        }
    }

    m_ainfo.lts = timestamp;

    AutoLock _l(m_mutex);

    // 2 bytes for 0xAF 0x00/0x01 (normally is so)
    byte *buf = (byte *) m_mem_pool.alloc(length-7+2);
    int body_len = make_audio_body(dat+7, length-7, buf, length-7+2);
    if (!send_rtmp_pkt(RTMP_PACKET_TYPE_AUDIO, timestamp+m_ainfo.tm_offset,
                       buf, body_len)) {
        E("Send audio data to rtmpserver failed");
        return -1;
    }

#ifdef XDEBUG_TIMESTAMP
    I("Audio timestamp: %d", timestamp+m_ainfo.tm_offset);
#endif
    return 0;
}

int RtmpHandler::make_asc_body(const byte asc[2], byte buf[], uint32_t len)
{
    buf[0] = 0xAF;
    buf[1] = 0x00;
    memcpy(buf+2, asc, 2);
    return 1 + 1 + 2;
}

// Note: dat is ADTS header removed
int RtmpHandler::make_audio_body(const byte *dat, uint32_t dat_len, byte buf[], uint32_t len)
{
    buf[0] = 0xAF;
    buf[1] = 0x01;
    memcpy(buf+2, dat, dat_len);
    return dat_len + 2;
}

byte RtmpHandler::pkttyp2channel(byte typ)
{
    if (typ == RTMP_PACKET_TYPE_VIDEO)
        return RTMP_VIDEO_CHANNEL;
    else if (typ == RTMP_PACKET_TYPE_AUDIO ||
             typ == RTMP_PACKET_TYPE_INFO)
        return RTMP_AUDIO_CHANNEL;
    else
        return RTMP_SYSTEM_CHANNEL;
}

bool RtmpHandler::packet_cb(void *opaque, int pkttype,
                            uint32_t pts, const byte *buf, uint32_t pktsize)
{
    RtmpHandler *hdlr = (RtmpHandler *) opaque;
    RTMPPacket rtmp_pkt;
    RTMPPacket_Reset(&rtmp_pkt);
    RTMPPacket_Alloc(&rtmp_pkt, pktsize);
    memcpy(rtmp_pkt.m_body, buf, pktsize);
    rtmp_pkt.m_packetType = pkttype;
    rtmp_pkt.m_nChannel = pkttyp2channel(pkttype);
    rtmp_pkt.m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmp_pkt.m_nTimeStamp = pts;
    rtmp_pkt.m_hasAbsTimestamp = 0;
    rtmp_pkt.m_nInfoField2 = hdlr->m_rtmp->m_stream_id;
    rtmp_pkt.m_nBodySize = pktsize;
    bool retval = RTMP_SendPacket(hdlr->m_rtmp, &rtmp_pkt, FALSE);
    RTMPPacket_Free(&rtmp_pkt);
    return retval;
}

bool RtmpHandler::send_rtmp_pkt(int pkttype, uint32_t ts,
                                const byte *buf, uint32_t pktsize)
{
    if (pkttype == RTMP_PACKET_TYPE_AUDIO ||
        pkttype == RTMP_PACKET_TYPE_VIDEO) {
        std::auto_ptr<RtmpPacket> pkt(
                new RtmpPacket(pkttype, (uint8_t *) buf, pktsize, ts, ts));
        return m_jitter->add_packet(pkt.get()) < 0 ? false : true;
    }

    return packet_cb(this, pkttype, ts, buf, pktsize);
}
