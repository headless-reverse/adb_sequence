#include "h264decoder.h"
#include <QDebug>
#include <QThread>
#include <QIODevice>
#include <QMutexLocker>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/error.h>
    #include <libavutil/mem.h>
    #include <libavformat/avformat.h>
}

H264Decoder::H264Decoder(QObject *parent) : QObject(parent) {
    init();
}

H264Decoder::~H264Decoder() {
    cleanup();
}

bool H264Decoder::init() {
    QMutexLocker locker(&m_mutex);
    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec) return false;
    m_parser = av_parser_init(m_codec->id);
    m_codecCtx = avcodec_alloc_context3(m_codec);
    if (!m_codecCtx) return false;
    m_codecCtx->flags  |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST | AV_CODEC_FLAG2_SHOW_ALL;
    m_codecCtx->thread_count = 0;
    if (avcodec_open2(m_codecCtx, m_codec, nullptr) < 0) return false;
    m_frame = av_frame_alloc();
    m_pkt = av_packet_alloc();
    return m_frame && m_pkt;}

bool H264Decoder::initSize(int width, int height) {
    m_width = width;
    m_height = height;
    return true; }

void H264Decoder::decode(const QByteArray &packet) {
    parseAndDecode(packet);}

void H264Decoder::parseAndDecode(const QByteArray &data) {
    QMutexLocker locker(&m_mutex);
    if (!m_codecCtx || !m_parser) return;

    const uint8_t *curData = reinterpret_cast<const uint8_t*>(data.constData());
    int curSize = data.size();

    while (curSize > 0) {
        int len = av_parser_parse2(m_parser, m_codecCtx,
                                   &m_pkt->data, &m_pkt->size,
                                   curData, curSize,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (len < 0) break;
        curData += len;
        curSize -= len;

        if (m_pkt->size > 0) {
            decodePacket(m_pkt);
            av_packet_unref(m_pkt);}}}

void H264Decoder::decodePacket(AVPacket *pkt) {
    int ret = avcodec_send_packet(m_codecCtx, pkt);
    if (ret < 0) return;

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            emit decoderError("Błąd dekodowania ramki");
            break;}
        AVFrame *outFrame = av_frame_clone(m_frame);
        if (outFrame) {
            auto sharedFrame = std::shared_ptr<AVFrame>(outFrame, [](AVFrame *f) {
                av_frame_free(&f);});
            emit frameReady(sharedFrame);}
        av_frame_unref(m_frame);}}

int H264Decoder::read_socket_callback(void *opaque, uint8_t *buf, int buf_size) {
    QIODevice *device = static_cast<QIODevice*>(opaque);
    if (device->bytesAvailable() == 0) {
        if (!device->waitForReadyRead(500)) return AVERROR(EAGAIN);}
    return device->read(reinterpret_cast<char*>(buf), buf_size);}

void H264Decoder::cleanup() {
    QMutexLocker locker(&m_mutex);
    if (m_parser) av_parser_close(m_parser);
    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
    if (m_frame) av_frame_free(&m_frame);
    if (m_pkt) av_packet_free(&m_pkt);
    if (m_formatCtx) avformat_close_input(&m_formatCtx);}
