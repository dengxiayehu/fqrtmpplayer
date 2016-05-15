#include "common.h"
#include "xutil.h"

#define THREAD_NAME "common"
extern JNIEnv *jni_get_env(const char *name);

jvalue jnu_get_field_by_name(jboolean *has_exception, jobject obj,
                             const char *name, const char *signature)
{
    JNIEnv *env = jni_get_env(THREAD_NAME);
    jclass cls;
    jfieldID fid;
    jvalue result;

    result.i = 0;

    if (env->EnsureLocalCapacity(3) < 0)
        goto done2;

    cls = env->GetObjectClass(obj);
    fid = env->GetFieldID(cls, name, signature);
    if (fid == NULL)
        goto done1;

    switch (*signature) {
    case '[':
    case 'L':
        result.l = env->GetObjectField(obj, fid);
        break;
    case 'Z':
        result.z = env->GetBooleanField(obj, fid);
        break;
    case 'B':
        result.b = env->GetByteField(obj, fid);
        break;
    case 'C':
        result.c = env->GetCharField(obj, fid);
        break;
    case 'S':
        result.s = env->GetShortField(obj, fid);
        break;
    case 'I':
        result.i = env->GetIntField(obj, fid);
        break;
    case 'J':
        result.j = env->GetLongField(obj, fid);
        break;
    case 'F':
        result.f = env->GetFloatField(obj, fid);
        break;
    case 'D':
        result.d = env->GetDoubleField(obj, fid);
        break;

    default:
        env->FatalError("jnu_get_field_by_name: illegal signature");
    }

done1:
    env->DeleteLocalRef(cls);
done2:
    if (has_exception) {
        *has_exception = env->ExceptionCheck();
    }
    return result;
}

jvalue jnu_call_method_by_name_v(jboolean *has_exception, jobject obj,
                                 const char *name, const char *signature, va_list args)
{
    JNIEnv *env = jni_get_env(THREAD_NAME);
    jclass clazz;
    jmethodID mid;
    jvalue result;
    const char *p = signature;

    while (*p && *p != ')')
        p++;
    p++;

    result.i = 0;

    if (env->EnsureLocalCapacity(3) < 0)
        goto done2;

    clazz = env->GetObjectClass(obj);
    mid = env->GetMethodID(clazz, name, signature);
    if (mid == NULL)
        goto done1;

    switch (*p) {
        case 'V':
            env->CallVoidMethodV(obj, mid, args);
            break;
        case '[':
        case 'L':
            result.l = env->CallObjectMethodV(obj, mid, args);
            break;
        case 'Z':
            result.z = env->CallBooleanMethodV(obj, mid, args);
            break;
        case 'B':
            result.b = env->CallByteMethodV(obj, mid, args);
            break;
        case 'C':
            result.c = env->CallCharMethodV(obj, mid, args);
            break;
        case 'S':
            result.s = env->CallShortMethodV(obj, mid, args);
            break;
        case 'I':
            result.i = env->CallIntMethodV(obj, mid, args);
            break;
        case 'J':
            result.j = env->CallLongMethodV(obj, mid, args);
            break;
        case 'F':
            result.f = env->CallFloatMethodV(obj, mid, args);
            break;
        case 'D':
            result.d = env->CallDoubleMethodV(obj, mid, args);
            break;
        default:
            env->FatalError("jnu_call_method_by_name_v: illegal signature");
    }
done1:
    env->DeleteLocalRef(clazz);
done2:
    if (has_exception) {
        *has_exception = env->ExceptionCheck();
    }
    return result;
}

jvalue jnu_call_method_by_name(jboolean *has_exception, jobject obj,
                               const char *name, const char *signature, ...)
{
    jvalue result;
    va_list args;

    va_start(args, signature);
    result = jnu_call_method_by_name_v(has_exception, obj, name, signature, args);
    va_end(args);

    return result;
}

void jnu_set_field_by_name(jboolean *hasException, jobject obj,
                           const char *name, const char *signature, ...)
{
    JNIEnv *env = jni_get_env(THREAD_NAME);
    jclass cls;
    jfieldID fid;
    va_list args;

    if (env->EnsureLocalCapacity(3) < 0)
        goto done2;

    cls = env->GetObjectClass(obj);
    fid = env->GetFieldID(cls, name, signature);
    if (fid == 0)
        goto done1;

    va_start(args, signature);
    switch (*signature) {
    case '[':
    case 'L':
        env->SetObjectField(obj, fid, va_arg(args, jobject));
    break;      
    case 'Z':
        env->SetBooleanField(obj, fid, (jboolean)va_arg(args, int));
    break;
    case 'B':
        env->SetByteField(obj, fid, (jbyte)va_arg(args, int));
    break;
    case 'C':
        env->SetCharField(obj, fid, (jchar)va_arg(args, int));
    break;
    case 'S':
        env->SetShortField(obj, fid, (jshort)va_arg(args, int));
    break;
    case 'I':
        env->SetIntField(obj, fid, va_arg(args, jint));
    break;
    case 'J':
        env->SetLongField(obj, fid, va_arg(args, jlong));
    break;
    case 'F':
        env->SetFloatField(obj, fid, (jfloat)va_arg(args, jdouble));
    break;
    case 'D':
        env->SetDoubleField(obj, fid, va_arg(args, jdouble));
    break;

    default:
        env->FatalError("jnu_set_field_by_name: illegal signature");
    }
    va_end(args);

 done1:
    env->DeleteLocalRef(cls);
 done2:
    if (hasException) {
        *hasException = env->ExceptionCheck();
    }
}

jstring jnu_new_string(const char *str)
{
    JNIEnv *env = jni_get_env(THREAD_NAME);
    jmethodID cid;
    jbyteArray arr;
    jsize len;
    jstring result;

    cid = env->GetMethodID(LibFQRtmp.String.clazz, "<init>", "([B)V");
    if (!cid)
        return NULL;

    len = strlen(str);
    arr = env->NewByteArray(len);
    if (!arr)
        return NULL;
    env->SetByteArrayRegion(arr, 0, len, (const jbyte *) str);

    result = (jstring) env->NewObject(LibFQRtmp.String.clazz, cid, arr);

    env->DeleteLocalRef(arr);
    return result;
}

//////////////////////////////////////////////////////////////////////////

void rtmp_log(int level, const char *fmt, va_list args)
{
    if (level == RTMP_LOGDEBUG2)
        return;

    char buf[4096];
    vsnprintf(buf, sizeof(buf)-1, fmt, args);

    android_LogPriority prio;

    switch (level) {
        default:
        case RTMP_LOGCRIT:
        case RTMP_LOGERROR:     prio = ANDROID_LOG_ERROR; break;
        case RTMP_LOGWARNING:   prio = ANDROID_LOG_WARN;  break;
        case RTMP_LOGINFO:      prio = ANDROID_LOG_INFO;  break;
        case RTMP_LOGDEBUG:     prio = ANDROID_LOG_DEBUG; break;
    }

    libfqrtmp_log_print("rtmp_module", -1, prio, LOG_TAG, buf);
}
