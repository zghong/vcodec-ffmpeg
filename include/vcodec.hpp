/*
 * @Copyright: iflytek
 * @Autor: zghong
 * @Date: 2020-04-07 12:18:54
 * @Description: 利用FFmpeg，对常见的编码格式文件进行编解码
 * 
 * 编解码封装在vcodec类中，接口为encode()和decode()函数。
 * 使用时需要指出<源文件>，<目标文件>及<编解码器名>。其中编解码器名可以通过`ffmpeg -encoders, ffmpeg -decoders`查看。
 */

#ifndef _VCODEC_HPP
#define _VCODEC_HPP

#include <string>

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"

#ifdef __cplusplus
}
#endif

/**********************************************************************
 * vcodec类定义部分
 * 
 * 进行视频编解码类，实例化时需要指出<源文件>，<目标文件>及<编解码器名>
 * 对外的接口如下：
 * 1、encode()，编码
 * 2、decode()，解码
 **********************************************************************
 */
class vcodec
{
public:
    vcodec(std::string in_path, std::string out_path, std::string codec_name);
    ~vcodec();
    int encode();
    int decode();

private:
    FILE *fin, *fout;
    std::string codec_name;

    AVFrame *frame;
    AVPacket *packet;
    AVCodec *codec;
    AVCodecContext *codec_ctx;
    AVCodecParserContext *parser_ctx;

    int encode_frame2packet();
    int decode_packet2frame();
};

/**********************************************************************
 * vcodec类实现部分
 **********************************************************************
 */
/**
 * @brief: vcodec类构造函数
 * @param in_path 输入文件路径
 * @param out_path 输出文件路径
 * @param codec_name 编解码器名
 */
vcodec::vcodec(std::string in_path, std::string out_path, std::string codec_name)
{
    this->fin = fopen((in_path).c_str(), "rb");
    if (!this->fin)
    {
        fprintf(stderr, "[ERROR] Failed to open <%s>\n", in_path.c_str());
        exit(-1);
    }
    this->fout = fopen((out_path).c_str(), "wb");
    if (!this->fout)
    {
        fprintf(stderr, "[ERROR] Failed to open <%s>\n", out_path.c_str());
        exit(-1);
    }
    this->codec_name = codec_name;
}

/**
 * @brief: vcodec类析构函数
 */
vcodec::~vcodec()
{
    fclose(this->fin);
    fclose(this->fout);
}

/**
 * @brief: 使用给定的编解码器对输入文件编码，将结果写入输出文件
 * @return: 0 for ok, -1 for error
 */
int vcodec::encode()
{
    // allocation
    this->frame = av_frame_alloc();
    if (!this->frame)
    {
        fprintf(stderr, "[ERROR] Failed to allocate frame\n");
        return -1;
    }
    this->packet = av_packet_alloc();
    if (!this->packet)
    {
        fprintf(stderr, "[ERROR] Failed to allocate packet\n");
        return -1;
    }

    this->codec = avcodec_find_encoder_by_name(this->codec_name.c_str());
    if (!this->codec)
    {
        fprintf(stderr, "[ERROR] Failed to find codec\n");
        return -1;
    }
    this->codec_ctx = avcodec_alloc_context3(this->codec);
    if (!this->codec_ctx)
    {
        fprintf(stderr, "[ERROR] Faild to allocate codec context\n");
        return -1;
    }

    // encoder settings
    this->codec_ctx->codec_id = this->codec->id;
    this->codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    this->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    this->codec_ctx->width = 640;
    this->codec_ctx->height = 480;
    this->codec_ctx->time_base = (AVRational){1, 30}; // fps
    // vbr设置
    // int64_t br = 468000;
    // this->codec_ctx->bit_rate = br;
    // this->codec_ctx->rc_min_rate = br;
    // this->codec_ctx->rc_max_rate = br;
    // this->codec_ctx->bit_rate_tolerance = br;
    // this->codec_ctx->rc_buffer_size = br;
    // this->codec_ctx->rc_initial_buffer_occupancy = this->codec_ctx->rc_buffer_size * 3 / 4;
    this->codec_ctx->bit_rate = 468000;
    this->codec_ctx->gop_size = 250;
    this->codec_ctx->max_b_frames = 0; // no b-frame
    if (this->codec_ctx->codec_id == AV_CODEC_ID_H264)
    {
        av_opt_set(this->codec_ctx->priv_data, "preset", "slow", 0);
        av_opt_set(this->codec_ctx->priv_data, "tune", "zerolatency", 0);
    }
    else if (this->codec_ctx->codec_id == AV_CODEC_ID_H265)
    {
        av_opt_set(this->codec_ctx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(this->codec_ctx->priv_data, "tune", "zero-latency", 0);
    }

    this->frame->format = this->codec_ctx->pix_fmt;
    this->frame->width = this->codec_ctx->width;
    this->frame->height = this->codec_ctx->height;

    // open the codec
    if (avcodec_open2(this->codec_ctx, this->codec, NULL) < 0)
    {
        fprintf(stderr, "[ERROR] Failed to open codec\n");
        return -1;
    }
    else
    {
        fprintf(stdout, "[INFO] Open codec successfully\n");
    }

    // encode 1 second of video
    int frame_size = av_image_get_buffer_size(this->codec_ctx->pix_fmt, this->codec_ctx->width, this->codec_ctx->height, 1);
    uint8_t *frame_buf = (uint8_t *)av_malloc(frame_size);
    int y_size = this->codec_ctx->width * this->codec_ctx->height; // size of Y
    av_image_fill_arrays(this->frame->data, this->frame->linesize, frame_buf, this->codec_ctx->pix_fmt, this->codec_ctx->width, this->codec_ctx->height, 1);
    int i = 0;
    while (!feof(this->fin))
    {
        if (fread(frame_buf, 1, y_size * 3 / 2, this->fin) <= 0)
        {
            break;
        }

        // read yuv data from source file into AVFrame
        this->frame->data[0] = frame_buf;                  // Y
        this->frame->data[1] = frame_buf + y_size;         // U
        this->frame->data[2] = frame_buf + y_size * 5 / 4; // V
        this->frame->pts = i++;

        // encode frame into packet
        if (this->encode_frame2packet() != 0)
        {
            return -1;
        }
    }

    // flush the encoder
    this->frame = NULL;
    this->encode_frame2packet();

    av_frame_free(&this->frame);
    av_packet_free(&this->packet);
    avcodec_free_context(&this->codec_ctx);

    fprintf(stdout, "[SUCCESS] Encode file successfully\n");
    return 0;
}

/**
 * @brief: 使用给定的编解码器对输入文件解码，将结果写入输出文件
 * @return: 0 for ok, -1 for error
 */
int vcodec::decode()
{
    // allocation
    this->frame = av_frame_alloc();
    if (!this->frame)
    {
        fprintf(stderr, "[ERROR] Failed to allocate frame\n");
        return -1;
    }
    this->packet = av_packet_alloc();
    if (!this->packet)
    {
        fprintf(stderr, "[ERROR] Failed to allocate packet\n");
        return -1;
    }
    this->codec = avcodec_find_decoder_by_name(this->codec_name.c_str());
    if (!this->codec)
    {
        fprintf(stderr, "[ERROR] Failed to find codec\n");
        return -1;
    }
    this->codec_ctx = avcodec_alloc_context3(this->codec);
    if (!this->codec_ctx)
    {
        fprintf(stderr, "[ERROR] Faild to allocate codec context\n");
        return -1;
    }
    this->parser_ctx = av_parser_init(this->codec->id);
    if (!this->parser_ctx)
    {
        fprintf(stderr, "[ERROR] Failed to find parser of codec\n");
        return -1;
    }

    // open the codec
    if (avcodec_open2(this->codec_ctx, this->codec, NULL) < 0)
    {
        fprintf(stderr, "[ERROR] Failed to open codec\n");
        return -1;
    }
    else
    {
        fprintf(stdout, "[INFO] Open codec successfully\n");
    }

    // start to decode
    int input_buf_size = 4096;
    uint8_t input_buf[input_buf_size + AV_INPUT_BUFFER_PADDING_SIZE] = {0};
    while (!feof(this->fin))
    {
        // read source data
        int read_size = fread(input_buf, 1, input_buf_size, this->fin);
        if (!read_size)
        {
            break;
        }

        uint8_t *input_buf_pos = input_buf;
        while (read_size > 0)
        {
            // parse data into packet
            int parsed_size = av_parser_parse2(this->parser_ctx, this->codec_ctx,
                                               &this->packet->data, &this->packet->size, input_buf_pos, read_size,
                                               AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (parsed_size < 0)
            {
                fprintf(stderr, "Error while parsing\n");
                return -1;
            }
            // update position of buffer and size of remained data
            input_buf_pos += parsed_size;
            read_size -= parsed_size;

            if (this->packet->size)
            {
                // decode packet into frame
                if (this->decode_packet2frame() != 0)
                {
                    return -1;
                }
            }
        }
    }

    fprintf(stdout, "[SUCCESS] Decode file successfully\n");
    return 0;
}

/**
 * @brief: 将数据帧编码成数据包
 * @return: 0 for ok, -1 for error
 */
int vcodec::encode_frame2packet()
{
    int ret = avcodec_send_frame(this->codec_ctx, this->frame);
    if (ret < 0)
    {
        fprintf(stderr, "[ERROR] Failed to send a frame for encoding\n");
        return -1;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(this->codec_ctx, this->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret < 0)
        {
            fprintf(stderr, "[ERROR] Error during encoding, code <%d>\n", ret);
            return -1;
        }

        fwrite(this->packet->data, 1, this->packet->size, this->fout);
        fprintf(stdout, "[INFO] Saving packet %3" PRId64 " (size=%5d)\n", this->packet->pts, this->packet->size);
        av_packet_unref(this->packet);
    }
    return 0;
}

/**
 * @brief: 将数据包解码成数据帧
 * @return: 0 for ok, -1 for error
 */
int vcodec::decode_packet2frame()
{
    int ret = avcodec_send_packet(this->codec_ctx, this->packet);
    if (ret < 0)
    {
        fprintf(stderr, "[ERROR] Failed to send a packet for decoding\n");
        return -1;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(this->codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret < 0)
        {
            fprintf(stderr, "[ERROR] Error during decoding, code <%d>\n", ret);
            return -1;
        }

        // save the frame into file
        // Y, U, V
        for (int i = 0; i < frame->height; i++)
        {
            fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->width, this->fout);
        }
        for (int i = 0; i < frame->height / 2; i++)
        {
            fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->width / 2, this->fout);
        }
        for (int i = 0; i < frame->height / 2; i++)
        {
            fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->width / 2, this->fout);
        }

        fprintf(stdout, "[INFO] Saving frame %3d\n", this->codec_ctx->frame_number);
        av_frame_unref(this->frame);
    }
    return 0;
}

#endif