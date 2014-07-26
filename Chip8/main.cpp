#include "chip8.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Chip8 w;
	w.show();
	return a.exec();
}
