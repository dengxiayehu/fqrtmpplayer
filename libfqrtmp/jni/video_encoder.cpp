#include <memory>
#include <libyuv.h>

#include "video_encoder.h"
#include "rtmp_handler.h"
#include "xqueue.h"

//#define XDEBUG

#define DUMP_YUV    0
#define DUMP_X264   0

using namespace xutil;
using namespace libyuv;

#define THREAD_NAME "video_encoder"
extern JNIEnv *jni_get_env(const char *name);

VideoEncoder::VideoEncoder() :
    m_enc(NULL), m_start_pts(0), m_frame_num(0), m_thrd(NULL), m_quit(false), m_file_yuv(NULL), m_file_x264(NULL)
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

    D("Average fps is: %.2f", m_fps_calc.get_fps());

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

int VideoEncoder::FPSCtrl::init(int tgt_fps, int orig_fps)
{
    a = 10000000;
    b = 10000000000LL/((int64_t) orig_fps*1000)*tgt_fps;

    if ((a + (b>>1))/b >= 10) {
        E("Ratio must be less than 10 for linear access");
        return -1;
    }

    last_frame = -1;
    n = 1;
    dropped_frames = 0;
    adoped_frames = 0;
    capture_start_time = get_time_now();
    start_timestamp = 0;
    first_timestamp = true;
    avg_timestamp_per_frame = 0;
    tgt_avg_time_per_frame = 10000000000LL/(int64_t) (tgt_fps*1000);
    this->tgt_fps = tgt_fps;
    return 0;
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
    m_params.i_log_level = X264_LOG_INFO;
    m_params.i_csp = X264_CSP_I420;

    if (m_bitrate > 0) {
        m_params.rc.i_bitrate = m_bitrate / 1000;
        m_params.rc.i_vbv_buffer_size = m_bitrate / 1000;
        m_params.rc.i_vbv_max_bitrate = m_bitrate * 1.2 / 1000;
        m_params.rc.i_rc_method = X264_RC_CRF;
        m_params.rc.f_rf_constant = 28;
        m_params.rc.i_qp_min = 20;
        m_params.rc.i_qp_max = 69;
        m_params.rc.i_qp_step = 4;
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

    m_params.i_threads = cpu_num();

    m_params.b_annexb = 1;

    m_enc = x264_encoder_open(&m_params);
    if (!m_enc) {
        E("x264_encoder_open failed");
        return -1;
    }

    x264_encoder_parameters(m_enc, &m_params);

    if (m_fps_ctrl.init(m_fps.num/m_fps.den,
                        m_orig_fps.num/m_orig_fps.den) < 0)
        return -1;

    return 0;
}

int VideoEncoder::feed(uint8_t *buffer, int len, int rotation)
{
    int dst_i420_y_size = m_width * m_height;
    int dst_i420_uv_size = ((m_width + 1) / 2) * ((m_height + 1) / 2);
    int dst_i420_size = dst_i420_y_size + dst_i420_uv_size * 2;

    align_buffer_64(dst_i420_c, dst_i420_size);

    switch (rotation) {
    default:
    case 0:
        NV12ToI420Rotate(buffer, m_width,
                         buffer + m_width * m_height, (m_width + 1) & ~1,
                         dst_i420_c, m_width,
                         dst_i420_c + dst_i420_y_size + dst_i420_uv_size, (m_width + 1) / 2,
                         dst_i420_c + dst_i420_y_size, (m_width + 1) / 2,
                         m_width, m_height, kRotate0);
        break;
    case 180:
        NV12ToI420Rotate(buffer, m_width,
                         buffer + m_width * m_height, (m_width + 1) & ~1,
                         dst_i420_c, m_width,
                         dst_i420_c + dst_i420_y_size + dst_i420_uv_size, (m_width + 1) / 2,
                         dst_i420_c + dst_i420_y_size, (m_width + 1) / 2,
                         m_width, m_height, kRotate180);
        break;
    }

    uint64_t now = get_time_now();

    if (!m_start_pts) {
        m_start_pts = now;
    }

    uint64_t pts = now - m_start_pts;
    if (!m_fps_ctrl.first_timestamp) {
        uint64_t stamp_differ = now - m_fps_ctrl.start_timestamp;
        m_fps_ctrl.start_timestamp = now;
        m_fps_ctrl.avg_timestamp_per_frame =
            (m_fps_ctrl.avg_timestamp_per_frame+stamp_differ)/2;

        if (stamp_differ > m_fps_ctrl.tgt_avg_time_per_frame) {
            ++m_fps_ctrl.low_diff_accept_frames;
        } else {
            m_fps_ctrl.get_frame =
                ((int64_t) m_fps_ctrl.n*m_fps_ctrl.a+(m_fps_ctrl.b>>1))/m_fps_ctrl.b;
            if ((m_fps_ctrl.last_frame < m_fps_ctrl.get_frame-1 &&
                 m_fps_ctrl.get_frame-m_fps_ctrl.last_frame < 10)) {
                if (m_fps_ctrl.last_frame < m_fps_ctrl.get_frame-1) {
                    ++m_fps_ctrl.last_frame;
                    ++m_fps_ctrl.dropped_frames;
                    free_aligned_buffer_64(dst_i420_c);
                    return 0;
                }
            }
        }
    } else {
        m_fps_ctrl.start_timestamp = now;
        m_fps_ctrl.avg_timestamp_per_frame = m_fps_ctrl.tgt_avg_time_per_frame;
        m_fps_ctrl.first_timestamp = false;
    }

    ++m_fps_ctrl.adoped_frames;

#ifdef XDEBUG
    if (!(m_fps_ctrl.adoped_frames%m_fps_ctrl.tgt_fps)) {
        D("Demux adopted frame rate %d%%",
          (int) (m_fps_ctrl.adoped_frames*100/(m_fps_ctrl.adoped_frames+m_fps_ctrl.dropped_frames)));
        D("Demux low_diff_accept_frames %ld",
          (long) m_fps_ctrl.low_diff_accept_frames);
        D("Demux frame rate is: %.2f fps",
          m_fps_ctrl.adoped_frames*1000.0f/(get_time_now()-m_fps_ctrl.capture_start_time));
        D("Demux average_timestamp_per_frame %.2f ms",
          m_fps_ctrl.avg_timestamp_per_frame/10000.0f);
    }
#endif

    m_fps_ctrl.last_frame = m_fps_ctrl.get_frame;
    ++m_fps_ctrl.n;

    Packet *pkt = new Packet(dst_i420_c, dst_i420_size, pts, pts);
    free_aligned_buffer_64(dst_i420_c);
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
        m_pic.img.i_plane = 3;

        m_pic.img.plane[0] = pkt->data;
        m_pic.img.plane[1] = m_pic.img.plane[0] + m_params.i_width * m_params.i_height;
        m_pic.img.plane[2] = m_pic.img.plane[1] + m_params.i_width * m_params.i_height / 4;
        m_pic.img.i_stride[0] = m_params.i_width;
        m_pic.img.i_stride[1] = (m_params.i_width + 1) / 2;
        m_pic.img.i_stride[2] = (m_params.i_width + 1) / 2;
        m_pic.i_pts = m_frame_num++;

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

        pkt_out->pts = pkt->pts;
        pkt_out->dts = pkt->dts;

        if (m_file_x264) {
            m_file_x264->write_buffer(pkt_out->data, pkt_out->size);
        }

        if (gfq.rtmp_hdlr) {
            gfq.rtmp_hdlr->send_video(pkt_out->pts, pkt_out->data, pkt_out->size);
        }

        m_fps_calc.check();

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

    CALL_METHOD(video_config, "getOrigFPS", "()Lcom/dxyh/libfqrtmp/LibFQRtmp$Rational;");
    INIT_RATIONAL_MEMBER(m_orig_fps, jval.l);

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

jint sendRawVideo(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len, int rotation)
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

        ret = gfq.video_enc->feed(buffer, len, rotation);

        env->ReleaseByteArrayElements(byte_arr, (jbyte *) buffer, 0);
    }

    return ret;
}
