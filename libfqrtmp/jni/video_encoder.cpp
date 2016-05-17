#include <memory>

#include "video_encoder.h"
#include "rtmp_handler.h"
#include "xqueue.h"

#define DUMP_YUV    0
#define DUMP_X264   0

#define THREAD_NAME "video_encoder"
extern JNIEnv *jni_get_env(const char *name);

VideoEncoder::VideoEncoder() :
    m_enc(NULL), m_thrd(NULL), m_quit(false), m_file_yuv(NULL), m_file_x264(NULL)
{
    memset(&m_params, 0, sizeof(m_params));

    m_thrd = CREATE_THREAD_ROUTINE(encode_routine, NULL, false);

#if defined(DUMP_YUV) && (DUMP_YUV != 0)
    m_file_yuv = new xfile::File;
    m_file_yuv->open("/sdcard/fqrtmp.yuv", "wb");
#endif
#if defined(DUMP_X264) && (DUMP_X264 != 0)
    m_file_x264 = new xfile::File;
    m_file_x264->open("/sdcard/fqrtmp.h264", "wb");
#endif
}

VideoEncoder::~VideoEncoder()
{
    Packet *pkt = NULL;
    int i;

    m_quit = true;
    m_queue.cancel_wait();
    JOIN_DELETE_THREAD(m_thrd);
    for (i = 0; i < m_queue.size(); ++i) {
        if (m_queue.pop(pkt) < 0)
            break;
        SAFE_DELETE(pkt);
    }
    x264_encoder_close(m_enc);
    m_enc = NULL;

    SAFE_DELETE(m_file_yuv);
    SAFE_DELETE(m_file_x264);
}

static void X264_log(void *p, int level, const char *fmt, va_list args)
{
    char buf[4096];

    static const int level_map[] = {
        ANDROID_LOG_ERROR,
        ANDROID_LOG_WARN,
        ANDROID_LOG_INFO,
        ANDROID_LOG_DEBUG
    };

    if (level < 0 || level > X264_LOG_DEBUG)
        return;

    vsnprintf(buf, sizeof(buf), fmt, args);
    __android_log_write(level_map[level], LOG_TAG, buf);
}

enum ImageFormat {
    PIX_FMT_NV21 = 0x00000011,
    PIX_FMT_YV12 = 0x32315659,
};

static int convert_pix_fmt(ImageFormat pix_fmt)
{
    switch (pix_fmt) {
#ifdef X264_CSP_NV21
    case PIX_FMT_NV21:      return X264_CSP_NV21;
#endif
#ifdef X264_CSP_YV12
    case PIX_FMT_YV12:      return X264_CSP_YV12;
#endif
    default:
        E("Unsupported pix_fmt: %d", pix_fmt);
        return -1;
    };
}

int VideoEncoder::init(jobject video_config)
{
    if (load_config(video_config) < 0) {
        E("Video encoder load_config failed");
        return -1;
    }

    x264_param_default(&m_params);

    if (!m_preset.empty() || !m_tune.empty()) {
        if (x264_param_default_preset(&m_params, STR(m_preset), STR(m_tune)) < 0) {
            E("Error setting preset/tune %s/%s",
              STR(m_preset), STR(m_tune));
            return -1;
        }
    }

    if (m_deblocking_filter) {
        m_params.b_deblocking_filter = 1;
        m_params.i_deblocking_filter_alphac0 = 0;
        m_params.i_deblocking_filter_beta = 0;
    }

    m_params.pf_log = X264_log;
    m_params.p_log_private = this;
    m_params.i_log_level = X264_LOG_DEBUG;
    m_params.i_csp = convert_pix_fmt((ImageFormat) m_input_csp);

    if (m_bitrate > 0) {
        m_params.rc.i_rc_method = X264_RC_ABR;
        m_params.rc.i_bitrate = m_bitrate / 1000;
        m_params.rc.i_vbv_max_bitrate = m_bitrate * 1.2 / 1000;
        m_params.rc.i_vbv_buffer_size = m_bitrate / 1000;
    }

    m_params.i_level_idc = m_level_idc;

    m_params.i_bframe = m_b_frames;

    m_params.analyse.i_subpel_refine = 5;
    m_params.analyse.i_me_method = X264_ME_DIA;
    m_params.analyse.i_me_range = 16;
    m_params.analyse.b_chroma_me = 1;
    m_params.analyse.i_mv_range = -1;
    m_params.analyse.b_fast_pskip = 1;
    m_params.analyse.b_dct_decimate = 1;

    m_params.b_repeat_headers = m_repeat_headers ? 1 : 0;

    if (!m_profile.empty()) {
        if (x264_param_apply_profile(&m_params, STR(m_profile)) < 0) {
            E("Error setting profile %s", STR(m_profile));
            return -1;
        }
    }

    m_params.i_width = m_width;
    m_params.i_height = m_height;

    m_params.i_fps_num = m_fps.num;
    m_params.i_fps_den = m_fps.den;

    m_params.i_keyint_max = m_fps.num / m_fps.den * m_i_frame_interval;

    m_params.i_threads = xutil::cpu_num();

    m_params.b_annexb = 1;

    m_enc = x264_encoder_open(&m_params);
    if (!m_enc) {
        E("x264_encoder_open failed");
        return -1;
    }

    x264_encoder_parameters(m_enc, &m_params);

    frac_init(&m_pts, 0, 1000 * m_fps.den, m_fps.num);
    return 0;
}

int VideoEncoder::feed(uint8_t *buffer, int len)
{
    Packet *pkt = new Packet(buffer, len);
    return m_queue.push(pkt);
}

unsigned int VideoEncoder::encode_routine(void *arg)
{
    Packet *pkt;
    x264_picture_t pic_out;
    x264_nal_t *nals;
    int num_of_nals;
    int frame_size;

    D("x264 encode_routine started ..");

    while (!m_quit) {
        std::auto_ptr<Packet> pkt_out(new Packet);
        int ret;

        if (m_queue.pop(pkt) < 0)
            break;

        if (m_quit)
            break;

        if (m_file_yuv) {
            m_file_yuv->write_buffer(pkt->data, pkt->size);
        }

        x264_picture_init(&m_pic);
        m_pic.img.i_csp = m_params.i_csp;
        m_pic.img.i_plane = 2;

        m_pic.img.plane[0] = pkt->data;
        m_pic.img.plane[1] = m_pic.img.plane[0] + m_params.i_width * m_params.i_height;
        m_pic.img.i_stride[0] = m_params.i_width;
        m_pic.img.i_stride[1] = m_params.i_width;
        m_pic.i_pts = m_pts.val;

        do {
            frame_size = x264_encoder_encode(m_enc, &nals, &num_of_nals, &m_pic, &pic_out);
            if (frame_size < 0) {
                E("x264_encoder_encode failed");
                goto cleanup;
            }
            
            ret = encode_nals(pkt_out.get(), nals, num_of_nals);
            if (ret < 0) {
               E("encode_nals failed");
               goto cleanup;
            }
        } while (!m_quit && !ret && x264_encoder_delayed_frames(m_enc));

        pkt_out->pts = pic_out.i_pts;
        pkt_out->dts = pic_out.i_dts;

        if (m_file_x264) {
            m_file_x264->write_buffer(pkt_out->data, pkt_out->size);
        }

        if (gfq.rtmp_hdlr)
            gfq.rtmp_hdlr->send_video(pkt_out->pts, pkt_out->data, pkt_out->size);

        frac_add(&m_pts, 1000 * m_fps.den);
cleanup:
        SAFE_DELETE(pkt);
    }

    D("x264 encode_routine ended");
    return 0;
}

int VideoEncoder::encode_nals(Packet *pkt, const x264_nal_t *nals, int nnal)
{
    int i, size = 0;
    uint8_t *p;

    if (!nnal)
        return 0;

    for (i = 0; i < nnal; ++i)
        size += nals[i].i_payload;

    pkt->data = (uint8_t *) realloc(pkt->data, size);
    if (!pkt->data) {
        E("realloc for out-pkt failed: %s", ERRNOMSG);
        return -1;
    }

    p = pkt->data;
    pkt->size = size;

    for (i = 0; i < nnal; ++i) {
        memcpy(p, nals[i].p_payload, nals[i].i_payload);
        p += nals[i].i_payload;
    }

    return 1;
}

volatile bool VideoEncoder::quit() const
{
    return m_quit;
}

int VideoEncoder::load_config(jobject video_config)
{
    JNIEnv *env = jni_get_env(THREAD_NAME);
    jvalue jval;

#define CALL_METHOD(obj, name, signature) do { \
    jboolean has_exception = JNI_FALSE; \
    jval = jnu_call_method_by_name(&has_exception, obj, name, signature); \
    if (has_exception) { \
        E("Exception with %s()", name); \
        return -1; \
    } \
} while (0)

#define INIT_STRING_MEMBER(member, jstr) do { \
    const char *str = env->GetStringUTFChars((jstring) jstr, NULL); \
    member = str ? str : ""; \
    env->ReleaseStringUTFChars((jstring) jstr, str); \
} while (0)

    CALL_METHOD(video_config, "getPreset", "()Ljava/lang/String;");
    INIT_STRING_MEMBER(m_preset, jval.l);

    CALL_METHOD(video_config, "getTune", "()Ljava/lang/String;");
    INIT_STRING_MEMBER(m_tune, jval.l);

    CALL_METHOD(video_config, "getProfile", "()Ljava/lang/String;");
    INIT_STRING_MEMBER(m_profile, jval.l);

    CALL_METHOD(video_config, "getLevelIDC", "()I");
    m_level_idc = jval.i;

    CALL_METHOD(video_config, "getInputCSP", "()I");
    m_input_csp = jval.i;

    CALL_METHOD(video_config, "getBitrate", "()I");
    m_bitrate = jval.i;

    CALL_METHOD(video_config, "getWidth", "()I");
    m_width = jval.i;

    CALL_METHOD(video_config, "getHeight", "()I");
    m_height = jval.i;

#define INIT_RATIONAL_MEMBER(rational, obj) do { \
    jboolean has_exception = JNI_FALSE; \
    jvalue tmpval = jnu_get_field_by_name(&has_exception, obj, "num", "I"); \
    if (has_exception) { \
        E("Exception with Rational"); \
        return -1; \
    } \
    rational.num = tmpval.i; \
    tmpval = jnu_get_field_by_name(&has_exception, obj, "den", "I"); \
    if (has_exception) { \
        E("Exception with Rational"); \
        return -1; \
    } \
    rational.den = tmpval.i; \
} while (0)

    CALL_METHOD(video_config, "getFPS", "()Lcom/dxyh/libfqrtmp/LibFQRtmp$Rational;");
    INIT_RATIONAL_MEMBER(m_fps, jval.l);

    CALL_METHOD(video_config, "getIFrameInterval", "()I");
    m_i_frame_interval = jval.i;

    CALL_METHOD(video_config, "getRepeatHeaders", "()Z");
    m_repeat_headers = jval.z;

    CALL_METHOD(video_config, "getBFrames", "()I");
    m_b_frames = jval.i;

    CALL_METHOD(video_config, "getDeblockingFilter", "()Z");
    m_deblocking_filter = jval.z;

    dump_config();
    return 0;

#undef CALL_METHOD
#undef INIT_STRING_MEMBER
#undef INIT_RATIONAL_MEMBER
}

void VideoEncoder::dump_config() const
{
    D("preset=%s, tune=%s, profile=%s, level_idc=%d, input_csp=%d, bitrate=%d, width=%d, height=%d, fps={%d/%d}, i_frame_interval=%d, repeat_headers=%s, b_frames=%d, deblocking_filter=%s",
      STR(m_preset), STR(m_tune), STR(m_profile), m_level_idc, m_input_csp, m_bitrate, m_width, m_height, m_fps.num, m_fps.den, m_i_frame_interval, m_repeat_headers ? "true" : "false", m_b_frames, m_deblocking_filter ? "true" : "false");
}

jint openVideoEncoder(JNIEnv *env, jobject thiz, jobject video_config)
{
    gfq.video_enc = new VideoEncoder;

    if (gfq.video_enc->init(video_config) < 0) {
        closeVideoEncoder(env, thiz);
        return -1;
    }

    return 0;
}

jint closeVideoEncoder(JNIEnv *env, jobject thiz)
{
    SAFE_DELETE(gfq.video_enc);
    return 0;
}

jint sendRawVideo(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len)
{
    int ret = 0;

    if (gfq.video_enc && !gfq.video_enc->quit()) {
        uint8_t *buffer;
        jboolean is_copy;

        buffer = (uint8_t *) env->GetByteArrayElements(byte_arr, &is_copy);
        if (!buffer) {
            E("Get video buffer failed");
            return -1;
        }

        ret = gfq.video_enc->feed(buffer, len);

        env->ReleaseByteArrayElements(byte_arr, (jbyte *) buffer, 0);
    }

    return ret;
}
