#include "rtmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <librtmp/rtmp.h>

#include "config.h"
#include "common.h"
#include "xutil.h"

using namespace xutil;

struct rtmp_t {
    const char *input;
    const char *live;
    RTMP *hdl;
};

enum RTMPChannel {
    RTMP_NETWORK_CHANNEL = 2,
    RTMP_SYSTEM_CHANNEL,
    RTMP_AUDIO_CHANNEL,
    RTMP_VIDEO_CHANNEL   = 6,
    RTMP_SOURCE_CHANNEL  = 8,
};

static int rtmp_send_pkt(rtmp_t *r, int pkttype,
                         uint32_t ts, const uint8_t *buf, uint32_t pktsize);

static int parse_arg(rtmp_t *r, const char *str)
{
    int n = 3, argc = 0;
    const char **argv =
        (const char **) malloc(n * sizeof(const char *));
    char *cmdline = strdup(str);
    const char *delim = " ";
    const char *p = strtok(cmdline, delim);
    int retval = 0;

    argv[argc++] = "libfqrtmp";
    while (p) {
        if (argc == n)
           argv = (const char **) realloc(argv, (n+=2)*sizeof(const char *));
        argv[argc++] = p;
        p = strtok(NULL, delim);
    }

    BEGIN
    struct option longopts[] = {
        {"input",   required_argument, NULL, 'i'},
        {"live",    required_argument, NULL, 'L'},
        {0, 0, 0, 0}
    };
    int ch;

    optind = 0;
    while ((ch = getopt_long(argc, (char * const *) argv,
                             ":i:L:W;", longopts, NULL)) != -1) {
        switch (ch) {
        case 'i':
            r->input = strdup(optarg);
            break;

        case 'L':
            r->live = strdup(optarg);
            break;

        case 0:
            break;

        case '?':
        default:
            E("Unknown option: %c", optopt);
            retval = -1;
            goto out;
        }
    }
    END

out:
    SAFE_FREE(argv);
    SAFE_FREE(cmdline);
    return retval;
}

static int check_arg(rtmp_t *r)
{
    if (!r->live) {
        E("Missing option -L");
        return -1;
    }
    return 0;
}

rtmp_t *rtmp_init(const char *str)
{
    rtmp_t *r = (rtmp_t *) calloc(1, sizeof(rtmp_t));
    if (!r) {
        E("calloc for rtmp_t failed");
        return NULL;
    }

    if (parse_arg(r, str) || check_arg(r))
        return NULL;

    return r;
}

int rtmp_connect(rtmp_t *r)
{
    r->hdl = RTMP_Alloc();

    RTMP_Init(r->hdl);
    r->hdl->Link.timeout = SOCK_TIMEOUT;
    r->hdl->m_outChunkSize = RTMP_CHUNK_SIZE;

    RTMP_LogSetLevel(RTMP_LOGLEVEL);
    RTMP_LogSetCallback(rtmp_log);

    if (!RTMP_SetupURL(r->hdl, (char *) r->live)) {
        E("RTMP_SetupURL() failed");
        goto bail;
    }

    if (r->live)
        RTMP_EnableWrite(r->hdl);

    if (!RTMP_Connect(r->hdl, NULL)) {
        E("RTMP_Connect failed");
        goto bail;
    }

    if (!RTMP_ConnectStream(r->hdl, 0)) {
        E("RTMP_ConnectStream failed");
        goto bail;
    }

    if (r->hdl->m_outChunkSize != RTMP_DEFAULT_CHUNKSIZE) {
        uint8_t buf[4];
        put_be32(buf, r->hdl->m_outChunkSize);
        if (!rtmp_send_pkt(r, RTMP_PACKET_TYPE_CHUNK_SIZE, 0, buf, 4)) {
            E("Send chunk size %d to server failed",
                    r->hdl->m_outChunkSize);
            goto bail;
        }
    }

    return 0;

bail:
    rtmp_disconnect(r);
    return -1;
}

int rtmp_disconnect(rtmp_t *r)
{
    if (r->hdl) {
        RTMP_Close(r->hdl);
        RTMP_Free(r->hdl);
        r->hdl = NULL;
    }
    return 0;
}

void rtmp_destroy(rtmp_t **r)
{
    if (*r) {
        SAFE_FREE(*r);
        *r = NULL;
    }
}

static uint8_t pkttyp2channel(uint8_t typ)
{
    if (typ == RTMP_PACKET_TYPE_VIDEO)
        return RTMP_VIDEO_CHANNEL;
    else if (typ == RTMP_PACKET_TYPE_AUDIO ||
             typ == RTMP_PACKET_TYPE_INFO)
        return RTMP_AUDIO_CHANNEL;
    else
        return RTMP_SYSTEM_CHANNEL;
}

static int rtmp_send_pkt(rtmp_t *r, int pkttype,
                         uint32_t ts, const uint8_t *buf, uint32_t pktsize)
{
    RTMPPacket rtmp_pkt;
    RTMPPacket_Reset(&rtmp_pkt);
    RTMPPacket_Alloc(&rtmp_pkt, pktsize);
    memcpy(rtmp_pkt.m_body, buf, pktsize);
    rtmp_pkt.m_packetType = pkttype;
    rtmp_pkt.m_nChannel = pkttyp2channel(pkttype);
    rtmp_pkt.m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmp_pkt.m_nTimeStamp = ts;
    rtmp_pkt.m_hasAbsTimestamp = 0;
    rtmp_pkt.m_nInfoField2 = r->hdl->m_stream_id;
    rtmp_pkt.m_nBodySize = pktsize;
    int retval = RTMP_SendPacket(r->hdl, &rtmp_pkt, 0);
    RTMPPacket_Free(&rtmp_pkt);
    return retval;
}
