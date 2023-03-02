#include <QPainter>

#include "PlayerThread.h"

#include "PlayerWidget.h"

PlayerWidget::PlayerWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

	init();
}

PlayerWidget::~PlayerWidget()
{
	m_playerThread->requestInterruption();
}

void PlayerWidget::init()
{
	m_playerThread = new PlayerThread((HWND)this->winId(), width(), height(), SDL_Mode, this);
	QObject::connect(m_playerThread, &PlayerThread::signalReceveImage, this, [this] (QImage image) {
		m_image = image;
		update();
	}, Qt::QueuedConnection);
}

void PlayerWidget::openStream(const QString& fileName)
{
	m_playerThread->openStream(fileName);
	m_playerThread->start();
}

void PlayerWidget::paintEvent(QPaintEvent *event)
{
	if (!m_image.isNull()) {
		QPainter painter(this);
		painter.drawImage(this->rect(), m_image);
	}

	return QWidget::paintEvent(event);
}