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
  QVector<QRgb> _pallette;

private:
  void initPallette();
  void doRender();
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
