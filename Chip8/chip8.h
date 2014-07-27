#ifndef CHIP8_H
#define CHIP8_H

#include <QtWidgets/QMainWindow>
#include "ui_chip8.h"

#include "Emulator.h"
#include "emulatorthread.h"

class Chip8 : public QMainWindow
{
	Q_OBJECT

public:
	Chip8(QWidget *parent = 0);
	~Chip8();

private:
	Ui::Chip8Class ui;
	EmulatorThread emuThread;
  Emulator emu;
  QVector<QRgb> _pallette;      // a palette, used in _scr.
  QImage _scr;                  // a copy of the emulator screen, in QImage format
  int _scale;                   // factor to multiply the bitmap.

private:
  void initPallette();
  void initBitmap();
  void doRender(QPainter &pnt);
  virtual void paintEvent(QPaintEvent *event);

public slots:
  void screenInvalidated();
	void openGame();
	void zoomIn();
	void zoomOut();
  void play();
  void pause();
};

#endif // CHIP8_H
