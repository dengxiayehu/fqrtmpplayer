#ifndef _JITTER_BUFFER_H_
#define _JITTER_BUFFER_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RtmpPacket : public Packet {
    int pkttype;

    RtmpPacket();
    RtmpPacket(int pkttype, uint8_t *data_, int size_, uint64_t pts_, uint64_t dts_);
    virtual ~RtmpPacket();

    int clone(RtmpPacket *pkt, bool reuse_buffer = false);
};

struct PacketList {
    RtmpPacket pkt;
    PacketList *next;
};

struct PacketCallback {
    void *opaque;
    bool (*cb) (void *opaque, int pkttype,
                uint32_t pts, const byte *buf, uint32_t pktsize);
};

class JitterBuffer {
public:
    JitterBuffer(int max_interleave_delta = 1000);
    ~JitterBuffer();

    int add_packet(RtmpPacket *pkt);

    int set_packet_callback(PacketCallback pc);

private:
    int interleave_packet_per_pts(RtmpPacket *out, RtmpPacket *pkt);
    int interleave_add_packet(RtmpPacket *pkt,
                              int (*compare)(JitterBuffer *s, RtmpPacket *, RtmpPacket *));
    static int interleave_compare_pts(JitterBuffer *s,
                                      RtmpPacket *next, RtmpPacket *pkt);
    PacketList *&locate_last(int pkttype);

private:
    PacketList *m_packet_buffer;
    PacketList *m_packet_buffer_end;
    uint64_t m_max_interleave_delta;
    enum { STREAM_NUM = 2 };
    PacketList *m_last_pktl[STREAM_NUM];
    PacketCallback m_pc;
    volatile bool m_quit;
};

#ifdef __cplusplus
}
#endif

#endif /* end of _JITTER_BUFFER_H_ */
