#include "PlayerWidget.h"
#include <QtWidgets/QApplication>

#undef main

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	PlayerWidget w;
	w.openStream("a.mp4");
    w.show();
    
	return a.exec();
}