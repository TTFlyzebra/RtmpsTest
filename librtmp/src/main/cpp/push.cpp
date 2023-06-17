#include <jni.h>
#include <string>

#include "RtmpPush.h"

JavaVM *jvm = NULL;


extern "C"
JNIEXPORT jlong JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_n_1init(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    LOGE("init RTMP  url: %s", url);

    CallJava *callJava = new CallJava(env, jvm, &instance);
    RtmpPush *rtmpPush = new RtmpPush(url, callJava);

    rtmpPush->isExit = false;
    rtmpPush->init();
    env->ReleaseStringUTFChars(url_, url);

    long idd = reinterpret_cast<long> (rtmpPush);

    LOGE("init RTMP id %s", idd);
    return idd;

}


extern "C"
JNIEXPORT void JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_n_1pushSPSPPS(JNIEnv *env, jobject instance, jlong cptr,
                                                       jbyteArray sps_,
                                                       jint spsLen, jbyteArray pps_, jint ppsLen) {
    jbyte *sps = env->GetByteArrayElements(sps_, NULL);
    jbyte *pps = env->GetByteArrayElements(pps_, NULL);


    RtmpPush *rtmpPush = reinterpret_cast<RtmpPush *>(cptr);

    if (rtmpPush && !rtmpPush->isExit ) {
        rtmpPush->pushSPSPPS(reinterpret_cast<char *>(sps), spsLen, reinterpret_cast<char *>(pps),
                             ppsLen);
    }
    env->ReleaseByteArrayElements(sps_, sps, 0);
    env->ReleaseByteArrayElements(pps_, pps, 0);
}




extern "C"
JNIEXPORT void JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_n_1pushSPSPPSVPS(JNIEnv *env, jobject instance, jlong cptr,
                                                          jbyteArray sps_,
                                                          jint spsLen, jbyteArray pps_, jint ppsLen,
                                                          jbyteArray vps_, jint vpsLen) {
    jbyte *sps = env->GetByteArrayElements(sps_, NULL);
    jbyte *pps = env->GetByteArrayElements(pps_, NULL);
    jbyte *vps = env->GetByteArrayElements(vps_, NULL);

    RtmpPush *rtmpPush = reinterpret_cast<RtmpPush *>(cptr);

    if (rtmpPush && !rtmpPush->isExit) {
        rtmpPush->pushSPSPPSVPS(reinterpret_cast<char *>(sps), spsLen,
                                reinterpret_cast<char *>(pps),
                                ppsLen, reinterpret_cast<char *>(vps),
                                vpsLen);
    }
    env->ReleaseByteArrayElements(sps_, sps, 0);
    env->ReleaseByteArrayElements(pps_, pps, 0);
    env->ReleaseByteArrayElements(vps_, vps, 0);
}






extern "C"
JNIEXPORT void JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_n_1pushVideoData(JNIEnv *env, jobject instance, jlong cptr,
                                                          jbyteArray data_,
                                                          jint dataLen, jboolean keyFrame,
                                                          jboolean isH264) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    RtmpPush *rtmpPush = reinterpret_cast<RtmpPush *>(cptr);
    if (rtmpPush && !rtmpPush->isExit&& rtmpPush->isStartPush) {
        rtmpPush->pushVideoData(reinterpret_cast<char *>(data), dataLen, keyFrame, isH264);
    }
    env->ReleaseByteArrayElements(data_, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_n_1pushAudioData(JNIEnv *env, jobject instance,
                                                          jlong cptr,
                                                          jbyteArray data_,
                                                          jint dataLen) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    RtmpPush *rtmpPush = reinterpret_cast<RtmpPush *>(cptr);
    if (rtmpPush && !rtmpPush->isExit&& rtmpPush->isStartPush) {
        rtmpPush->pushAudioData(reinterpret_cast<char *>(data), dataLen);
    }

    env->ReleaseByteArrayElements(data_, data, 0);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_n_1stop(JNIEnv *env, jobject instance, jlong cptr) {
    RtmpPush *rtmpPush = reinterpret_cast<RtmpPush *>(cptr);
    if (rtmpPush && !rtmpPush->isExit) {
        rtmpPush->isExit = true;
        rtmpPush->pushStop();
//        delete rtmpPush->callJava;
        delete rtmpPush;
//        rtmpPush->callJava = NULL;
        rtmpPush = NULL;
    }
}




extern "C"
JNIEXPORT void JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_n_1pushAudioHeader(JNIEnv *env, jobject instance,
                                                            jlong cptr, jbyteArray data_,
                                                            jint dataLen) {

    jbyte *data = env->GetByteArrayElements(data_, NULL);
    RtmpPush *rtmpPush = reinterpret_cast<RtmpPush *>(cptr);
    if (rtmpPush && !rtmpPush->isExit) {
        rtmpPush->pushAudioHeader(reinterpret_cast<char *>(data), dataLen);
    }

    env->ReleaseByteArrayElements(data_, data, 0);
}





extern "C"
JNIEXPORT jstring JNICALL
Java_com_bill_mei_1utils_rtmp_RtmpHelper_printShabi(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "ni shi shabi";
    return env->NewStringUTF(hello.c_str());
}



extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *javaVM, void *reserved) {
    jint result = -1;
    jvm = javaVM;
    JNIEnv *jniEnv = NULL;

    if ((result = javaVM->GetEnv((void **) (&jniEnv), JNI_VERSION_1_4)) != JNI_OK) {
        LOGE("GetEnv ERROR");
        return result;
    }
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    jvm = NULL;
}




