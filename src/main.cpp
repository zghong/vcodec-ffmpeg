/*
 * @Copyright: iflytek
 * @Autor: zghong
 * @Date: 2020-04-02 16:52:55
 * @Description: 测试vcodec类
 * 
 * libx264和libx265第三方库集成在`./include, ./lib`中。
 * 已经测试的编解码算法有：h264, h265
 */

#include <stdio.h>
#include <string>
#include "vcodec.hpp"

using namespace std;

int main()
{
    // decoder_h264
    vcodec decoder_h264 = vcodec("../bin/video/input.h264", "./decoder_h264.yuv", "h264");
    decoder_h264.decode();
    // encoder_h264
    vcodec encoder_h264 = vcodec("./decoder_h264.yuv", "./encoder_h264.h264", "libx264");
    encoder_h264.encode();
    // encoder_h265
    vcodec encoder_h265 = vcodec("./decoder_h264.yuv", "./encoder_h265.h265", "libx265");
    encoder_h265.encode();
    // decoder_h265
    vcodec decoder_h265 = vcodec("./encoder_h265.h265", "./decoder_h265.yuv", "hevc");
    decoder_h265.decode();
    return 0;
}