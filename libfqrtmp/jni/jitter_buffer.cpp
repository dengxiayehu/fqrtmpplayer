#include <memory>
#include <librtmp/rtmp.h>

#include "jitter_buffer.h"
#include "xutil.h"

RtmpPacket::RtmpPacket() :
    pkttype(0)
{
}

RtmpPacket::RtmpPacket(int pkttype_, uint8_t *data_, int size_, uint64_t pts_, uint64_t dts_) :
    Packet(data_, size_, pts_, dts_), pkttype(pkttype_)
{
}

RtmpPacket::~RtmpPacket()
{
}

int RtmpPacket::clone(RtmpPacket *pkt, bool reuse_buffer)
{
    if (!pkt)
        return -1;

    *this = *pkt;

    if (!reuse_buffer) {
        data = (uint8_t *) malloc(sizeof(uint8_t) * pkt->size);
        if (!data)
            return -1;
        memcpy(data, pkt->data, pkt->size);
    } else {
        pkt->data = NULL;
    }
    return 0;
}

JitterBuffer::JitterBuffer(int max_interleave_delta) :
    m_packet_buffer(NULL), m_packet_buffer_end(NULL),
    m_max_interleave_delta(max_interleave_delta),
    m_quit(false)
{
    memset(m_last_pktl, 0, sizeof(m_last_pktl));
    memset(&m_pc, 0, sizeof(m_pc));
}

JitterBuffer::~JitterBuffer()
{
    PacketList *p = m_packet_buffer, *q;
    while (p) {
        q = p->next;
        SAFE_DELETE(p);
        p = q;
    }
}

int JitterBuffer::add_packet(RtmpPacket *pkt)
{
    while (!m_quit) {
        std::auto_ptr<RtmpPacket> opkt(new RtmpPacket);
        int ret = interleave_packet_per_pts(opkt.get(), pkt);
        pkt = NULL;
        if (ret <= 0) {
            return ret;
        }
        if (m_pc.cb &&
            !m_pc.cb(m_pc.opaque, opkt->pkttype, opkt->pts, opkt->data, opkt->size)) {
            break;
        }
    }
    return 0;
}

int JitterBuffer::set_packet_callback(PacketCallback pc)
{
    m_pc = pc;
    return 0;
}

int JitterBuffer::interleave_packet_per_pts(RtmpPacket *out, RtmpPacket *pkt)
{
    PacketList *pktl;
    unsigned stream_count = 0;
    int ret;
    unsigned i;
    int flush = 0;

    if (pkt &&
        (ret = interleave_add_packet(pkt, interleave_compare_pts)) < 0) {
        return ret;
    }

    for (i = 0; i < STREAM_NUM; ++i) {
        if (m_last_pktl[i]) {
            ++stream_count;
        }
    }
    if (stream_count == STREAM_NUM)
        flush = 1;

    if (m_max_interleave_delta > 0 &&
        m_packet_buffer &&
        !flush) {
        RtmpPacket *top_pkt = &m_packet_buffer->pkt;
        uint64_t top_pts = top_pkt->pts;
        uint64_t delta_pts = 0;

        for (i = 0; i < STREAM_NUM; ++i) {
            uint64_t last_pts;
            const PacketList *last = m_last_pktl[i];

            if (!last)
                continue;

            last_pts = last->pkt.pts;
            delta_pts = MAX(delta_pts, last_pts - top_pts);
        }

        if (delta_pts > m_max_interleave_delta) {
            W("Delay between the first packet and last packet in the "
              "muxing queue is %llu > %llu: forcing output",
              (long long unsigned) delta_pts, (long long unsigned) m_max_interleave_delta);
            flush = 1;
        }
    }

    if (stream_count && flush) {
        pktl = m_packet_buffer;
        out->clone(&pktl->pkt, true);

        m_packet_buffer = pktl->next;
        if (!m_packet_buffer)
            m_packet_buffer_end = NULL;

        if (locate_last(out->pkttype) == pktl)
            locate_last(out->pkttype) = NULL;
        SAFE_DELETE(pktl);
        return 1;
    }

    return 0;
}

int JitterBuffer::interleave_compare_pts(JitterBuffer *s,
                                         RtmpPacket *next, RtmpPacket *pkt)
{
    if (next->pts > pkt->pts)
        return 1;
    return 0;
}

int JitterBuffer::interleave_add_packet(RtmpPacket *pkt,
                                        int (*compare)(JitterBuffer *s, RtmpPacket *, RtmpPacket *))
{
    PacketList **next_point, *this_pktl;

    this_pktl = new PacketList;
    if (!this_pktl) {
        E("calloc for this_pktl failed: %s",
          ERRNOMSG);
        return -1;
    }
    this_pktl->pkt.clone(pkt, true);

    if (locate_last(pkt->pkttype)) {
        next_point = &(locate_last(pkt->pkttype)->next);
    } else {
        next_point = &m_packet_buffer;
    }

    if (*next_point) {
        if (compare(this, &m_packet_buffer_end->pkt, pkt)) {
            while (*next_point &&
                   !compare(this, &(*next_point)->pkt, pkt)) {
                next_point = &(*next_point)->next;
            }
            if (*next_point)
                goto next_non_null;
        } else {
            next_point = &(m_packet_buffer_end->next);
        }
    }

    assert(!*next_point);

    m_packet_buffer_end = this_pktl;

next_non_null:
    this_pktl->next = *next_point;
    locate_last(pkt->pkttype) = *next_point = this_pktl;
    return 0;
}

PacketList *&JitterBuffer::locate_last(int pkttype)
{
    assert(pkttype == RTMP_PACKET_TYPE_VIDEO ||
           pkttype == RTMP_PACKET_TYPE_AUDIO);

    if (pkttype == RTMP_PACKET_TYPE_VIDEO)
        return m_last_pktl[0];
    return m_last_pktl[1];
}
