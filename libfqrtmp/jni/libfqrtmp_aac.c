#include "libfqrtmp_aac.h"

static const char *aac_get_error(AACENC_ERROR err)
{
    switch (err) {
    case AACENC_OK:
        return "No error";
    case AACENC_INVALID_HANDLE:
        return "Invalid handle";
    case AACENC_MEMORY_ERROR:
        return "Memory allocation error";
    case AACENC_UNSUPPORTED_PARAMETER:
        return "Unsupported parameter";
    case AACENC_INVALID_CONFIG:
        return "Invalid config";
    case AACENC_INIT_ERROR:
        return "Initialization error";
    case AACENC_INIT_AAC_ERROR:
        return "AAC library initialization error";
    case AACENC_INIT_SBR_ERROR:
        return "SBR library initialization error";
    case AACENC_INIT_TP_ERROR:
        return "Transport library initialization error";
    case AACENC_INIT_META_ERROR:
        return "Metadata library initialization error";
    case AACENC_ENCODE_ERROR:
        return "Encoding error";
    case AACENC_ENCODE_EOF:
        return "End of file";
    default:
        return "Unknown error";
    }
}

jint openAudioEncoder(JNIEnv *env, jobject thiz, jobject audio_config)
{
    jvalue jval;
    int aot;
    int samplerate;
    int channels;
    CHANNEL_MODE mode;
    int sce = 0, cpe = 0;
    int bitrate;
    AACENC_InfoStruct info = { 0 };
    AACENC_ERROR err;

#define CALL_METHOD(obj, name, signature) do { \
    jboolean has_exception = JNI_FALSE; \
    jval = jnu_call_method_by_name(&has_exception, obj, name, signature); \
    if (has_exception) { \
        E("Exception with %s()", name); \
        goto error; \
    } \
} while (0)

    CALL_METHOD(audio_config, "getChannelCount", "()I");
    if ((err = aacEncOpen(&LibFQRtmp.aac_enc, 0, jval.i)) != AACENC_OK) {
        E("Unable to open the encoder: %s",
          aac_get_error(err));
        goto error;
    }
    channels = jval.i;

    CALL_METHOD(audio_config, "getAudioObjectType", "()I");
    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_AOT,
                                   jval.i)) != AACENC_OK) {
        E("Unable to set the AOT %d: %s",
          jval.i, aac_get_error(err));
        goto error;
    }
    aot = jval.i;

    CALL_METHOD(audio_config, "getSamplerate", "()I");
    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_SAMPLERATE,
                                   jval.i)) != AACENC_OK) {
        E("Unable to set the sample rate %d: %s\n",
          jval.i, aac_get_error(err));
        goto error;
    }
    samplerate = jval.i;

    switch (channels) {
    case 1: mode = MODE_1;       sce = 1; cpe = 0; break;
    case 2: mode = MODE_2;       sce = 0; cpe = 1; break;
    case 3: mode = MODE_1_2;     sce = 1; cpe = 1; break;
    case 4: mode = MODE_1_2_1;   sce = 2; cpe = 1; break;
    case 5: mode = MODE_1_2_2;   sce = 1; cpe = 2; break;
    case 6: mode = MODE_1_2_2_1; sce = 2; cpe = 2; break;
    default:
        E("Unsupported number of channels %d", channels);
        goto error;
    }
    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_CHANNELMODE,
                                   mode)) != AACENC_OK) {
        E("Unable to set channel mode %d: %s\n",
          mode, aac_get_error(err));
        goto error;
    }
    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_CHANNELORDER,
                                   1)) != AACENC_OK) {
        E("Unable to set wav channel order %d: %s\n",
          mode, aac_get_error(err));
        goto error;
    }

#define SET_FIELD(obj, name, signature, val) do { \
    jboolean has_exception = JNI_FALSE; \
    jnu_set_field_by_name(&has_exception, obj, name, signature, val); \
    if (has_exception) { \
        E("Exception occurred when set field \"%s\"", name); \
        goto error; \
    } \
} while (0)

    CALL_METHOD(audio_config, "getBitrate", "()I");
    if (jval.i <= 0) {
        if (aot == AOT_PS) {
            sce = 1;
            cpe = 0;
        }
        bitrate = (96*sce + 128*cpe) * samplerate / 44;
        if (aot == AOT_SBR || aot == AOT_PS) {
            bitrate /= 2;
        }
    } else {
        bitrate = jval.i;
    }
    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_BITRATE,
                                   bitrate)) != AACENC_OK) {
        E("Unable to set the bitrate %d: %s",
          bitrate, aac_get_error(err));
        goto error;
    }
    SET_FIELD(audio_config, "mBitrate", "I", bitrate);

    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_TRANSMUX,
                                   2 /* ADTS bitstream format */)) != AACENC_OK) {
        E("Unable to set the transmux format: %s",
          aac_get_error(err));
        goto error;
    }

    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_SIGNALING_MODE,
                                   0)) != AACENC_OK) {
        E("Unable to set signaling mode: %s",
          aac_get_error(err));
        goto error;
    }

    if ((err = aacEncoder_SetParam(LibFQRtmp.aac_enc, AACENC_AFTERBURNER,
                                   1)) != AACENC_OK) {
        E("Unable to set afterburner: %s",
          aac_get_error(err));
        goto error;
    }

    if ((err = aacEncEncode(LibFQRtmp.aac_enc, NULL, NULL, NULL, NULL)) != AACENC_OK) {
        E("Unable to initialize the encoder: %s\n",
          aac_get_error(err));
        goto error;
    }

    if ((err = aacEncInfo(LibFQRtmp.aac_enc, &info)) != AACENC_OK) {
        E("Unable to get encoder info: %s",
          aac_get_error(err));
        goto error;
    }

    SET_FIELD(audio_config, "mFrameLength", "I", info.frameLength);
    SET_FIELD(audio_config, "mEncoderDelay", "I", info.encoderDelay);
    return 0;

error:
    closeAudioEncoder(env, thiz);
    return -1;

#undef CALL_AUDIO_CONFIG_METHOD
#undef SET_FIELD
}

jint closeAudioEncoder(JNIEnv *env, jobject thiz)
{
    if (LibFQRtmp.aac_enc) {
        aacEncClose(&LibFQRtmp.aac_enc);
    }
    return 0;
}

jint sendRawAudio(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len)
{
    return 0;
}
