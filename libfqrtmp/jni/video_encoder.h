#ifndef _VIDEO_ENCODER_H_
#define _VIDEO_ENCODER_H_

#include <jni.h>
#include <stdint.h>
#include <x264.h>

#include "common.h"
#include "xqueue.h"
#include "xfile.h"
#include "xmedia.h"
#include "xutil.h"

#ifdef __cplusplus
extern "C" {
#endif

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    int init(jobject video_config);
    int feed(uint8_t *buffer, int len);
    volatile bool quit() const;

private:
    int load_config(jobject video_config);
    void dump_config() const;

    int encode_nals(Packet *pkt, const x264_nal_t *nals, int nnal);

    struct FPSCtrl {
        int64_t a, b;
        int last_frame;
        int get_frame;
        int n;
        int64_t dropped_frames;
        int64_t adoped_frames;
        uint64_t capture_start_time;
        uint64_t start_timestamp;
        bool first_timestamp;
        uint64_t avg_timestamp_per_frame;
        uint64_t tgt_avg_time_per_frame;
        int64_t low_diff_accept_frames;
        int tgt_fps;

        int init(int tgt_fps, int orig_fps = 30);
    };

private:
    std::string m_preset;
    std::string m_tune;
    std::string m_profile;
    int m_level_idc;
    int m_input_csp;
    int m_bitrate;
    int m_width;
    int m_height;
    Rational m_fps;
    Rational m_orig_fps;
    int m_i_frame_interval;
    bool m_repeat_headers;
    int m_b_frames;
    bool m_deblocking_filter;
    x264_param_t m_params;
    x264_t *m_enc;
    x264_picture_t m_pic;
    uint64_t m_start_pts;
    int m_frame_num;
    DECL_THREAD_ROUTINE(VideoEncoder, encode_routine);
    xutil::Thread *m_thrd;
    Queue<Packet *> m_queue;
    volatile bool m_quit;
    xfile::File *m_file_yuv;
    xfile::File *m_file_x264;
    xmedia::FPSCalc m_fps_calc;
    FPSCtrl m_fps_ctrl;
};

jint openVideoEncoder(JNIEnv *env, jobject, jobject);
jint closeVideoEncoder(JNIEnv *env, jobject);
jint sendRawVideo(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len, int rotation);

#ifdef __cplusplus
}
#endif
#endif /* end of _VIDEO_ENCODER_H_ */
