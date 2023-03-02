#include <atltypes.h>

#include <QDebug>

#include "PlayerThread.h"

//SDL ����
static SDL_Window *sdl_win = NULL;
//��Ⱦ��
static SDL_Renderer *sdl_render = NULL;
//����
static SDL_Texture *sdl_texture = NULL;

PlayerThread::PlayerThread(HWND handle, int winWidth, int winHeight, ShowMode showMode, QObject *parent/* = nullptr*/)
    : QThread(parent), m_winHandle(handle), m_winWidth(winWidth), m_winHeight(winHeight), m_showMode(showMode),
	m_videoType(-1), m_audioType(-1), m_bitBuffer(nullptr), m_startTime(0)
{
	init();
}

PlayerThread::~PlayerThread()
{
	quit();
	wait();
}

void PlayerThread::init()
{
	m_fmtCtx = avformat_alloc_context();
	if (m_fmtCtx == nullptr) {
		qDebug() << "avformat_alloc_context error!";
	}
	sdlInit();
	connect(this, &PlayerThread::finished, this, [this]() {
		if (avformat_free_context) {
			avformat_free_context(m_fmtCtx);
			m_fmtCtx = nullptr;
		}
		if (m_videoCodecCtx) {
			avcodec_free_context(&m_videoCodecCtx);
			m_videoCodecCtx = nullptr;
		}
		if (m_avPacket) {
			av_packet_unref(m_avPacket);
			m_avPacket = nullptr;
		}
		if (m_avFrame) {
			av_frame_unref(m_avFrame);
			m_avFrame = nullptr;
		}
		if (m_avFrame2) {
			av_frame_unref(m_avFrame2);
			m_avFrame2 = nullptr;
		}
		if (m_avFrame3) {
			av_frame_unref(m_avFrame3);
			m_avFrame3 = nullptr;
		}
		if (m_buffer) {
			av_free(m_buffer);
			m_buffer = nullptr;
		}
	}, Qt::DirectConnection);
}

int PlayerThread::sdlInit()
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		return -1;
	}

	sdl_win = SDL_CreateWindowFrom(m_winHandle);
	if (!sdl_win)
	{
		return -1;
	}

	sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
	if (!sdl_render)
	{
		return -1;
	}

	sdl_texture = SDL_CreateTexture(
		sdl_render,
		SDL_PIXELFORMAT_ARGB8888/*���ظ�ʽ*/,
		SDL_TEXTUREACCESS_STREAMING/*��Ⱦ��ʽ���ɼ���*/,
		m_winWidth/*������label���д�����*/,
		m_winHeight/*��*/
	);
	if (!sdl_texture)
	{
		return -1;
	}
}

void PlayerThread::showRGBToWnd(HWND hWnd, BYTE* data, int width, int height)
{
	if (data == NULL)
		return;

	static BITMAPINFO *bitMapinfo = NULL;
	static bool First = TRUE;

	if (First)
	{
		m_bitBuffer = new BYTE[40 + 4 * 256];//����һ���ڴ�����

		if (m_bitBuffer == NULL)
		{
			return;
		}
		First = FALSE;
		memset(m_bitBuffer, 0, 40 + 4 * 256);
		bitMapinfo = (BITMAPINFO *)m_bitBuffer;
		bitMapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitMapinfo->bmiHeader.biPlanes = 1;
		for (int i = 0; i < 256; i++)
		{ //��ɫ��ȡֵ��Χ (0-255)
			bitMapinfo->bmiColors[i].rgbBlue = bitMapinfo->bmiColors[i].rgbGreen = bitMapinfo->bmiColors[i].rgbRed = (BYTE)i;
		}
	}
	bitMapinfo->bmiHeader.biHeight = -height;
	bitMapinfo->bmiHeader.biWidth = width;
	bitMapinfo->bmiHeader.biBitCount = 3 * 8;
	CRect drect;
	GetClientRect(hWnd, drect);    //pWndָ��CWnd���һ��ָ�� 
	HDC hDC = GetDC(hWnd);     //HDC��Windows��һ���������ͣ����豸���������
	SetStretchBltMode(hDC, COLORONCOLOR);
	StretchDIBits(hDC,
		0,
		0,
		drect.right,   //��ʾ���ڿ��
		drect.bottom,  //��ʾ���ڸ߶�
		0,
		0,
		width,      //ͼ����
		height,      //ͼ��߶�
		data,
		bitMapinfo,
		DIB_RGB_COLORS,
		SRCCOPY
	);
	ReleaseDC(hWnd, hDC);
}

int PlayerThread::openStream(const QString& fileName)
{
	int ret = 0;
	if (m_fmtCtx){
		ret = avformat_open_input(&m_fmtCtx, fileName.toUtf8().data(), nullptr, nullptr);
		if (ret < 0) {
			qDebug() << "avformat_open_input error!";
			return ret;
		}

		ret = avformat_find_stream_info(m_fmtCtx, nullptr);
		if (ret < 0) {
			qDebug() << "avformat_find_stream_info error!";
			return ret;
		}

		for (int i = 0; i < m_fmtCtx->nb_streams; i++) {
			if (m_fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				m_videoType = i;
			}
			if (m_fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
				m_audioType = i;
			}
		}
		if (m_videoType == -1) {
			qDebug() << "û�ҵ���Ƶ��!";
			return -1;
		}
		if (m_audioType == -1) {
			qDebug() << "û�ҵ���Ƶ��!";
			return -1;
		}

		AVCodecID codec_id = m_fmtCtx->streams[m_videoType]->codecpar->codec_id;
		const AVCodec *m_videoDecoder = avcodec_find_decoder(codec_id);
		if (m_videoDecoder == nullptr) {
			qDebug() << "û���ҵ���Ӧ�Ľ�����!";
			return -1;
		}

		m_videoCodecCtx = avcodec_alloc_context3(m_videoDecoder);
		if (m_videoCodecCtx == nullptr) {
			qDebug() << "avcodec_alloc_context3 error!";
			return -1;
		}
		avcodec_parameters_to_context(m_videoCodecCtx, m_fmtCtx->streams[m_videoType]->codecpar);
		if (m_videoCodecCtx == nullptr) {
			qDebug() << "avcodec_parameters_to_context error!";
			return -1;
		}
		ret = avcodec_open2(m_videoCodecCtx, m_videoDecoder, nullptr);
		if (ret < 0) {
			qDebug() << "avcodec_open2 error!";
			return -1;
		}

		m_videoWidth = m_videoCodecCtx->width;
		m_videoHeight = m_videoCodecCtx->height;

		m_avPacket = av_packet_alloc();
		m_avFrame = av_frame_alloc();
		m_avFrame2 = av_frame_alloc();
		m_avFrame3 = av_frame_alloc();

		AVPixelFormat srcFormat = m_videoCodecCtx->pix_fmt == -1 ? AV_PIX_FMT_YUV420P : m_videoCodecCtx->pix_fmt;
		AVPixelFormat dstFormat = AV_PIX_FMT_RGB32;

		int byte = av_image_get_buffer_size(dstFormat, m_winWidth, m_winHeight, 1);
		m_buffer = (uint8_t *)av_malloc(byte * sizeof(uint8_t));

		av_image_fill_arrays(m_avFrame3->data, m_avFrame3->linesize, m_buffer, dstFormat, m_winWidth, m_winHeight, 1);

		//ͼ��ת��
		m_swsContext = sws_getContext(m_videoWidth, m_videoHeight, srcFormat, m_winWidth, m_winHeight, dstFormat,
			SWS_BICUBIC, NULL, NULL, NULL);
	}

	return ret;
}

void PlayerThread::run()
{
	while (1) {
		if (isInterruptionRequested()) {
			return;
		}

		if (m_fmtCtx != nullptr && av_read_frame(m_fmtCtx, m_avPacket) >= 0) {
			if (m_avPacket->stream_index == m_videoType) {
				if (avcodec_send_packet(m_videoCodecCtx, m_avPacket) < 0)
				{
					continue;
				}

				if (avcodec_receive_frame(m_videoCodecCtx, m_avFrame2) < 0)
				{
					continue;
				}

				sws_scale(m_swsContext, (const uint8_t *const *)m_avFrame2->data, m_avFrame2->linesize, 0, m_videoHeight,
					m_avFrame3->data, m_avFrame3->linesize);

				if (m_buffer)
				{
					if (m_startTime == 0) {
						m_startTime = GetTickCount();
					}
					DWORD other = (double)((double)(m_avPacket->pts) * (double)((double)(m_fmtCtx->streams[m_videoType]->time_base.num) / (double)(m_fmtCtx->streams[m_videoType]->time_base.den))) * 1000;
					DWORD showTime = m_startTime + other;
					DWORD curTime = GetTickCount();
					if (curTime < showTime) {
						DWORD delay = showTime - curTime;
						msleep(delay);
					}

					if (m_showMode == Qt_RepaintMode) {
						QImage image((uchar *)m_buffer, m_winWidth, m_winHeight, QImage::Format_RGB32);
						if (!image.isNull())
						{
							emit signalReceveImage(image);
						}
					}
					else if (m_showMode == Win_Mode) {
						showRGBToWnd(m_winHandle, m_buffer, m_winWidth, m_winWidth);
					}
					else if (m_showMode == SDL_Mode) {
						SDL_Rect rect = SDL_Rect{ 0,0,m_winWidth, m_winWidth };   // �ߴ���Ҫ��texture��ͬ
						//5 �ڴ�����д�����
						SDL_UpdateTexture(sdl_texture, NULL, m_buffer, m_avFrame3->linesize[0]/*һ�еĴ�С*/);
						//6 ������Ļ
						SDL_RenderClear(sdl_render);
						//7 ���Ʋ��ʵ���Ⱦ��
						SDL_RenderCopy(sdl_render, sdl_texture,
							NULL,//ԭͼλ�úͳߴ�
							&rect//Ŀ��λ�úͳߴ�
						);
						//8 ��Ⱦ
						SDL_RenderPresent(sdl_render);
					}
				}
			}
			else if (m_avPacket->stream_index == m_audioType) {
				//SDL_PauseAudio(0);
			}
		}
		av_packet_unref(m_avPacket);
		av_freep(m_avPacket);
	}
}