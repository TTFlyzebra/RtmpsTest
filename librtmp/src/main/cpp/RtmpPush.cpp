#include "RtmpPush.h"


RtmpPush::RtmpPush(const char *url, CallJava *callJava) {
    this->url = static_cast<char *>(malloc(512));
    strcpy(this->url, url);
    this->queue = new Queue();

    this->callJava = callJava;
}


RtmpPush::~RtmpPush() {


    if (this->queue) {
        delete (queue);
        queue = NULL;
    }

    if (this->url) {
        free(this->url);
        this->url = NULL;
    }
}


void *callbackPush(void *data) {
    RtmpPush *rtmpPush = static_cast<RtmpPush *>(data);
    rtmpPush->isStartPush = false;
    rtmpPush->callJava->conn(THREAD_CHILD);

    rtmpPush->rtmp = RTMP_Alloc();


    RTMP_Init(rtmpPush->rtmp);
    rtmpPush->rtmp->Link.timeout = 10;
    rtmpPush->rtmp->Link.lFlags |= RTMP_LF_LIVE;


    RTMP_SetupURL(rtmpPush->rtmp, rtmpPush->url);
    RTMP_EnableWrite(rtmpPush->rtmp);
    LOGE("RTMP SetupURL3333");
    if (!RTMP_Connect(rtmpPush->rtmp, NULL)) {
        LOGE("RTMP can not connect  url: %s", rtmpPush->url);
        rtmpPush->callJava->connf(THREAD_CHILD, "RTMP can not connect  url ");
        goto end;
    }

//    LOGE("RTMP connect success, url: %s", rtmpPush->url);
    if (!RTMP_ConnectStream(rtmpPush->rtmp, 0)) {
        LOGE("RTMP can not connect stream url: %s", rtmpPush->url);
        rtmpPush->callJava->connf(THREAD_CHILD, "RTMP can not connect stream url");

        goto end;
    }
    rtmpPush->callJava->conns(THREAD_CHILD);
    LOGE("RTMP connect stream success, url: %s", rtmpPush->url);

    rtmpPush->isStartPush = true;
    rtmpPush->startTime = RTMP_GetTime();


    while (rtmpPush->isStartPush) {
        RTMPPacket *packet = rtmpPush->queue->getRtmpPacket();
        if (RTMP_IsConnected(rtmpPush->rtmp)) {
            if (packet) {
                // queue 缓存队列大小
                if (rtmpPush->rtmp) {
                    int result = RTMP_SendPacket(rtmpPush->rtmp, packet, 1);
                    if (result == 1) {
                        rtmpPush->failedTimes = 0;
                    } else {
                        rtmpPush->failedTimes++;
                    }
//                    LOGD("RTMP_SendPacket result:%d", result);
                    RTMPPacket_Free(packet);
                    free(packet);
//                packet = NULL;
                }
            }
        } else {
            rtmpPush->failedTimes++;
            if (rtmpPush->failedTimes > 300) {
                rtmpPush->failedTimes = 0;
                rtmpPush->callJava->connf(THREAD_CHILD, "RTMP can not connect stream url");
                goto end;
            }
        }
    }


    end:
    RTMP_Close(rtmpPush->rtmp);
    RTMP_Free(rtmpPush->rtmp);
    rtmpPush->rtmp = NULL;
    pthread_exit(&rtmpPush->push_thrad);
}


void RtmpPush::init() {
    pthread_create(&push_thrad, NULL, callbackPush, this);
}

void RtmpPush::pushSPSPPS(char *sps, int spsLen, char *pps, int ppsLen) {
    if (!this->rtmp) return;
    if (!this->queue) return;
    LOGE("RTMP pushSPSPPS");
    int bodySize = spsLen + ppsLen + 16;
    RTMPPacket *rtmpPacket = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(rtmpPacket, bodySize);
    RTMPPacket_Reset(rtmpPacket);

    char *body = rtmpPacket->m_body;

    int i = 0;
    //frame type(4bit)和CodecId(4bit)合成一个字节(byte)
    //frame type 关键帧1  非关键帧2
    //CodecId  7表示avc
    body[i++] = 0x17;

    //fixed 4byte
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    //configurationVersion： 版本 1byte
    body[i++] = 0x01;

    //AVCProfileIndication：Profile 1byte  sps[1]
    body[i++] = sps[1];

    //compatibility：  兼容性 1byte  sps[2]
    body[i++] = sps[2];

    //AVCLevelIndication： ProfileLevel 1byte  sps[3]
    body[i++] = sps[3];

    //lengthSizeMinusOne： 包长数据所使用的字节数  1byte
    body[i++] = 0xff;

    //sps个数 1byte
    body[i++] = 0xe1;
    //sps长度 2byte
    body[i++] = (spsLen >> 8) & 0xff;
    body[i++] = spsLen & 0xff;

    //sps data 内容
    memcpy(&body[i], sps, spsLen);
    i += spsLen;
    //pps个数 1byte
    body[i++] = 0x01;
    //pps长度 2byte
    body[i++] = (ppsLen >> 8) & 0xff;
    body[i++] = ppsLen & 0xff;
    //pps data 内容
    memcpy(&body[i], pps, ppsLen);


    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    rtmpPacket->m_nBodySize = bodySize;
    rtmpPacket->m_nTimeStamp = 0;
    rtmpPacket->m_hasAbsTimestamp = 0;
    rtmpPacket->m_nChannel = 0x04;//音频或者视频
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    rtmpPacket->m_nInfoField2 = this->rtmp->m_stream_id;

    queue->putRtmpPacket(rtmpPacket);

}

#define RTMP_HEAD_SIZE (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

void RtmpPush::pushSPSPPSVPS(char *sps, int spsLen, char *pps, int ppsLen, char *vps, int vpsLen) {
    if (!this->rtmp) return;
    if (!this->queue) return;
    LOGE("RTMP_BBD_1 pushSPSPPSVPS");
    int bodySize = RTMP_HEAD_SIZE + 1024;
    RTMPPacket *rtmpPacket = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(rtmpPacket, bodySize);
    RTMPPacket_Reset(rtmpPacket);

    char *body = rtmpPacket->m_body;

    int i = 0;
    body[i++] = (1 << 4) | 12;  // 1:Iframe  7:AVC --  (frametype << 4) | codecID;
    body[i++] = 0x00;  // Type: SequenceHeader
    i += 3;            // 3个字节的0x00
    body[i++] = 0x01;  //configurationVersion, always is 0x01
    // general_profile_idc 8bit
    body[i++] = sps[1];
    // general_profile_compatibility_flags 32 bit
    body[i++] = sps[2];
    body[i++] = sps[3];
    body[i++] = sps[4];
    body[i++] = sps[5];

    // 48 bit NUll nothing deal in rtmp
    body[i++] = sps[6];
    body[i++] = sps[7];
    body[i++] = sps[8];
    body[i++] = sps[9];
    body[i++] = sps[10];
    body[i++] = sps[11];

    // general_level_idc
    body[i++] = sps[12];

    // 48 bit NUll nothing deal in rtmp
    i += 6;

    // bit(16) avgFrameRate;
    i += 2;

    /* bit(2) constantFrameRate; */
    /* bit(3) numTemporalLayers; */
    /* bit(1) temporalIdNested; */
    /* unsigned int(2) lengthSizeMinusOne : always 3 */
    // i += 1;
    body[i++] = (0 << 6) | (0 << 3) | (0 << 2) | 3;

    /* unsigned int(8) numOfArrays; 03 */
    body[i++] = 0x03;
    // vps 32
    body[i++] = 0x20;
    body[i++] = (1 >> 8) & 0xff;
    body[i++] = 1 & 0xff;
    body[i++] = (vpsLen >> 8) & 0xff;
    body[i++] = (vpsLen) & 0xff;
    memcpy(&body[i], vps, vpsLen);
    i += vpsLen;

    // sps
    body[i++] = 0x21;  // sps 33
    body[i++] = (1 >> 8) & 0xff;
    body[i++] = 1 & 0xff;
    body[i++] = (spsLen >> 8) & 0xff;
    body[i++] = spsLen & 0xff;
    memcpy(&body[i], sps, spsLen);
    i += spsLen;

    // pps
    body[i++] = 0x22;  // pps 34
    body[i++] = (1 >> 8) & 0xff;
    body[i++] = 1 & 0xff;
    body[i++] = (ppsLen >> 8) & 0xff;
    body[i++] = (ppsLen) & 0xff;
    memcpy(&body[i], pps, ppsLen);
    i += ppsLen;

    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    rtmpPacket->m_nBodySize = i;
    rtmpPacket->m_nTimeStamp = 0;
    rtmpPacket->m_hasAbsTimestamp = 0;
    rtmpPacket->m_nChannel = 0x04;//音频或者视频
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    rtmpPacket->m_nInfoField2 = this->rtmp->m_stream_id;

    queue->putRtmpPacket(rtmpPacket);

}


//void RtmpPush::pushSeiData(char *data) {
//    if (!this->rtmp) return;
//    if (!this->queue) return;
//    LOGE("RTMP_BBD_2 pushSeiData11111");
//
//    int leng = 28;
//
//    int bodySize = leng + 4;
//    RTMPPacket *rtmpPacket = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
//    RTMPPacket_Alloc(rtmpPacket, bodySize);
//    RTMPPacket_Reset(rtmpPacket);
//
//    char *body = rtmpPacket->m_body;
//
//    int i = 0;
//
//
//    body[i++] = (leng >> 24) & 0xff;
//    body[i++] = (leng >> 16) & 0xff;
//    body[i++] = (leng >> 8) & 0xff;
//    body[i++] = leng & 0xff;
//
//    body[i++] = 0x4e;
//    body[i++] = 0x01;
//    body[i++] = 0xe5;
//    body[i++] = 0x05;
//    body[i++] = 0xe5;
//    body[i++] = 0x16;
//    body[i++] = 0x4D;
//    body[i++] = 0x65;
//    body[i++] = 0x69;
//    body[i++] = 0x74;
//    body[i++] = 0x72;
//    body[i++] = 0x61;
//    body[i++] = 0x63;
//    body[i++] = 0x6B;
//    body[i++] = 0xFF;
//    body[i++] = 0xFF;
//    body[i++] = 0xFF;
//    body[i++] = 0xFF;
//    body[i++] = 0xFF;
//    body[i++] = 0xFF;
//    body[i++] = 0xFF;
//    body[i++] = 0xFF;
//
//
//    int encrypt = 0;
//    for (int i = 25; i <= 33; i++) {
//        for (int j = 0; j < 8; j++) {
//            if ((data[i] >> j) & 0x01 == 1) {
//                encrypt += 1;
//            }
//        }
//    }
//    encrypt = encrypt * encrypt * encrypt + 2 * encrypt + 1;
//    body[i++] = encrypt >> 24;
//    body[i++] = (encrypt >> 16) & 0xFF;
//    body[i++] = 0x03;
//    body[i++] = (encrypt >> 8) & 0xFF;
//    body[i++] = encrypt & 0xFF;
//    body[i++] = 0x03;
//
//
//    LOGE("RTMP_BBD_2 pushSeiData22222 ");
//
//    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
//    rtmpPacket->m_nBodySize = bodySize;
//    rtmpPacket->m_nTimeStamp = 0;
//    rtmpPacket->m_hasAbsTimestamp = 0;
//    rtmpPacket->m_nChannel = 0x04;//音频或者视频
//    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
//    rtmpPacket->m_nInfoField2 = this->rtmp->m_stream_id;
//
//    queue->putRtmpPacket(rtmpPacket);
//}


void RtmpPush::pushVideoData(char *data, int dataLen, bool keyFrame, bool isH264) {
    if (!this->rtmp) return;
    if (!this->queue) return;


    int bodySize = dataLen + 9;


    if (!isH264 && keyFrame) {
        bodySize = bodySize + 32;
    }
    RTMPPacket *rtmpPacket = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(rtmpPacket, bodySize);
    RTMPPacket_Reset(rtmpPacket);

    char *body = rtmpPacket->m_body;


    int i = 0;
    //frame type(4bit)和CodecId(4bit)合成一个字节(byte)
    //frame type 关键帧1  非关键帧2
    //CodecId  7表示avc
    if (keyFrame) {
        if (isH264) {
            body[i++] = 0x17;
        } else {
//            pushSeiData(data);
            body[i++] = 0x1C;
        }
    } else {
        if (isH264) {
            body[i++] = 0x27;
        } else {
            body[i++] = 0x2C;
        }
    }

    //fixed 4byte   0x01表示NALU单元
    body[i++] = 0x01;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;


    if (!isH264 && keyFrame) {
        body[i++] = (28 >> 24) & 0xff;
        body[i++] = (28 >> 16) & 0xff;
        body[i++] = (28 >> 8) & 0xff;
        body[i++] = 28 & 0xff;

        body[i++] = 0x4e;
        body[i++] = 0x01;
        body[i++] = 0xe5;
        body[i++] = 0x05;
        body[i++] = 0xe5;
        body[i++] = 0x16;
        body[i++] = 0x4D;
        body[i++] = 0x65;
        body[i++] = 0x69;
        body[i++] = 0x74;
        body[i++] = 0x72;
        body[i++] = 0x61;
        body[i++] = 0x63;
        body[i++] = 0x6B;
        body[i++] = 0xFF;
        body[i++] = 0xFF;
        body[i++] = 0xFF;
        body[i++] = 0xFF;
        body[i++] = 0xFF;
        body[i++] = 0xFF;
        body[i++] = 0xFF;
        body[i++] = 0xFF;


        int encrypt = 0;
        for (int i = 25; i <= 33; i++) {
            for (int j = 0; j < 8; j++) {
                if ((data[i] >> j) & 0x01 == 1) {
                    encrypt += 1;
                }
            }
        }
        encrypt = encrypt * encrypt * encrypt + 2 * encrypt + 1;
        body[i++] = encrypt >> 24;
        body[i++] = (encrypt >> 16) & 0xFF;
        body[i++] = 0x03;
        body[i++] = (encrypt >> 8) & 0xFF;
        body[i++] = encrypt & 0xFF;
        body[i++] = 0x03;
    }



    //dataLen  4byte
    body[i++] = (dataLen >> 24) & 0xff;
    body[i++] = (dataLen >> 16) & 0xff;
    body[i++] = (dataLen >> 8) & 0xff;
    body[i++] = dataLen & 0xff;

    //data
    memcpy(&body[i], data, dataLen);

    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    rtmpPacket->m_nBodySize = bodySize;
    //进入直播播放开始时间
    rtmpPacket->m_hasAbsTimestamp = 0;
    rtmpPacket->m_nChannel = 0x04;//音频或者视频
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmpPacket->m_nInfoField2 = this->rtmp->m_stream_id;
    //持续播放时间



    rtmpPacket->m_nTimeStamp = RTMP_GetTime() - this->startTime;
    queue->putRtmpPacket(rtmpPacket);

//    if (RTMP_IsConnected(rtmp)) {
//        LOGE("RTMP_BBD pushVideoData111 ");
////        free(rtmpPacket);
//    } else {
//
//
//        LOGE("RTMP_BBD is notconnected ");
////        pushStop();
//        if (keyFrame)
//            RTMP_Connect(rtmp, rtmpPacket);
////        queue->putRtmpPacket(rtmpPacket);
////        init();
//    }


}


void RtmpPush::pushAudioHeader(char *data, int dataLen) {


    if (!this->rtmp) return;
    if (!this->queue) return;
    int bodySize = dataLen + 2;
    RTMPPacket *rtmpPacket = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(rtmpPacket, bodySize);
    RTMPPacket_Reset(rtmpPacket);

    char *body = rtmpPacket->m_body;
    body[0] = 0xAF;
    body[1] = 0x00;

    //data
    memcpy(&body[2], data, dataLen);

    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    rtmpPacket->m_nBodySize = bodySize;
    //持续播放时间
    rtmpPacket->m_nTimeStamp = 0;
    //进入直播播放开始时间
    rtmpPacket->m_hasAbsTimestamp = 0;
    rtmpPacket->m_nChannel = 0x05;//音频或者视频
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    rtmpPacket->m_nInfoField2 = this->rtmp->m_stream_id;

    queue->putRtmpPacket(rtmpPacket);

    LOGE("RTMP_SendPacket send aac aac aac head succeed");








//    if (!this->rtmp) return 1;
//    if (!this->queue) return 2;
//    RTMPPacket packet;
//    RTMPPacket_Reset(&packet);
//    RTMPPacket_Alloc(&packet, 4);
//
//    packet.m_body[0] = 0xAF;  // MP3 AAC format 48000Hz
//    packet.m_body[1] = 0x00;
//    packet.m_body[2] = 0x11;
//    packet.m_body[3] = 0x90;//0x10修改为0x90,2016-1-19
//
//    packet.m_headerType  = RTMP_PACKET_SIZE_MEDIUM;
//    packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
//    packet.m_hasAbsTimestamp = 0;
//    packet.m_nChannel   = 0x05;
//    packet.m_nTimeStamp = 0;
//    packet.m_nInfoField2 = this->rtmp->m_stream_id;
//    packet.m_nBodySize  = 4;
//
//    //调用发送接口
//    int nRet = RTMP_SendPacket(this->rtmp, &packet, TRUE);
//    RTMPPacket_Free(&packet);//释放内存
//    return nRet;
}


void RtmpPush::pushAudioData(char *data, int dataLen) {
    if (!this->rtmp) return;
    if (!this->queue) return;
    int bodySize = dataLen + 2;
    RTMPPacket *rtmpPacket = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(rtmpPacket, bodySize);
    RTMPPacket_Reset(rtmpPacket);

    char *body = rtmpPacket->m_body;
    //前四位表示音频数据格式  10（十进制）表示AAC，16进制就是A
    //第5-6位的数值表示采样率，0 = 5.5 kHz，1 = 11 kHz，2 = 22 kHz，3(11) = 44 kHz。
    //第7位表示采样精度，0 = 8bits，1 = 16bits。
    //第8位表示音频类型，0 = mono，1 = stereo
    //这里是44100 立体声 16bit 二进制就是1111   16进制就是F
    body[0] = 0xAF;

    //0x00 aac头信息     0x01 aac 原始数据
    //这里都用0x01都可以
    body[1] = 0x01;

    //data
    memcpy(&body[2], data, dataLen);

    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    rtmpPacket->m_nBodySize = bodySize;
    //持续播放时间
    rtmpPacket->m_nTimeStamp = RTMP_GetTime() - this->startTime;
    //进入直播播放开始时间
    rtmpPacket->m_hasAbsTimestamp = 0;
    rtmpPacket->m_nChannel = 0x05;//音频或者视频
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;
    rtmpPacket->m_nInfoField2 = this->rtmp->m_stream_id;

    queue->putRtmpPacket(rtmpPacket);
}

void RtmpPush::pushStop() {
    this->isStartPush = false;
    queue->notifyQueue();
    pthread_join(push_thrad, NULL);
}

