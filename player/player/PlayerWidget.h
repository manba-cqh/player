#pragma once

#include <QImage>

#include "ui_PlayerWidget.h"

class PlayerThread;

class PlayerWidget : public QWidget
{
    Q_OBJECT

public:
    PlayerWidget(QWidget *parent = nullptr);
    ~PlayerWidget();

	void openStream(const QString& fileName);

protected:
	virtual void paintEvent(QPaintEvent *event);


private:
	void init();

private:
    Ui::PlayerWidgetClass ui;

	PlayerThread *m_playerThread;
	QImage m_image;
};
