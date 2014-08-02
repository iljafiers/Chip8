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
	EmulatorThread _emuThread;
  Emulator _emu;
  QVector<QRgb> _pallette;      // a palette, used in _scr.
  QImage _scr;                  // a copy of the emulator screen, in QImage format
  int _scale;                   // factor to multiply the bitmap.
  QTimer *_timer;

private:
  void initPallette();
  void initBitmap();
  virtual void paintEvent(QPaintEvent *event);
  void UpdateUI();
  // key handling
  void registerKey(bool down, int key);
  virtual bool eventFilter(QObject * /*object*/ , QEvent *event);

public slots:
  void screenInvalidated();
  void timerTick();
	void openGame();
	void zoomIn();
	void zoomOut();
  void play();
  void pause();
};

#endif // CHIP8_H
