/**
 * FileName: RtmpHelper
 * Author: FlyZebra
 * Email:flycnzebra@gmail.com
 * Date: 2023/6/17 8:52
 * Description:
 */
package com.bill.mei.utils.rtmp;

public class RtmpHelper {

    static {
        System.loadLibrary("meitrack_rtmp");
    }

    public native long init(String url_, int w, int h,  int timeOut);

    public native int sendSpsAndPps(long cptr, byte[] sps_, int spsLen, byte[] pps_, int ppsLen, long timestamp);

    public native int sendVideoData(long cptr,  byte[] data_, int len, long timestamp);

    public native int sendAacSpec(long cptr, byte[] data_, int len);

    public native int sendAacData(long cptr, byte[] data_, int len, long timestamp);

    public native int stop(long cptr);

}
