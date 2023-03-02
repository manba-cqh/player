#pragma once
extern "C"
{
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libswresample/swresample.h"
#include "SDL.h"
}

#include <windows.h>

#include <QThread>
#include <QImage>

enum ShowMode {
	Qt_RepaintMode = 0,
	Win_Mode,
	SDL_Mode,
};

class PlayerThread : public QThread
{
    Q_OBJECT

signals:
	void signalReceveImage(QImage image);

public:
    PlayerThread(HWND handle, int winWidth, int winHeight, ShowMode showMode, QObject *parent = nullptr);
    ~PlayerThread();

	int openStream(const QString& fileName);

protected:
	virtual void run();

private:
	void init();
	int sdlInit();
	void showRGBToWnd(HWND hWnd, BYTE* data, int width, int height);

private:
	HWND m_winHandle;
	int m_winWidth;
	int m_winHeight;

	ShowMode m_showMode;

	AVFormatContext *m_fmtCtx;
	AVCodecContext *m_videoCodecCtx;
	AVCodec *m_videoDecoder;
	SwsContext *m_swsContext;
	uint8_t *m_buffer;
	int m_videoWidth;
	int m_videoHeight;
	int m_videoType;
	int m_audioType;

	AVPacket *m_avPacket;
	AVFrame *m_avFrame;
	AVFrame *m_avFrame2;
	AVFrame *m_avFrame3;

	DWORD m_startTime;
	
	BYTE *m_bitBuffer;
};
