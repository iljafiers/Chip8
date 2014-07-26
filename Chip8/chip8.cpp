#include "chip8.h"
#include <QFileDialog>
#include <qbitmap.h>
#include <qpainter.h>

Chip8::Chip8(QWidget *parent)
: QMainWindow( parent ),
  emuThread( &emu )
{
  ui.setupUi(this);

  connect(ui.actionOpenGame, SIGNAL(triggered()), this, SLOT(openGame()));
  connect(ui.actionZoomIn, SIGNAL(triggered()), this, SLOT(zoomIn()));
  connect(ui.actionZoomOut, SIGNAL(triggered()), this, SLOT(zoomOut()));
  // toolbar
  connect(ui.actionStartEmulator, SIGNAL(triggered()), this, SLOT(play()));
  connect(ui.actionPauseEmulator, SIGNAL(triggered()), this, SLOT(pause()));

  initPallette();
}

void Chip8::initPallette()
{
  _pallette.clear();
  _pallette.append(QRgb(0xFF000000));
  _pallette.append(QRgb(0xFFFFFFFF));
  for (int dummy = 2; dummy < 256; dummy++)
    _pallette.append(QRgb(0xFF800000));

}

Chip8::~Chip8()
{

}

void Chip8::paintEvent(QPaintEvent *event)
{
  doRender();
}

void Chip8::doRender()
{
  QImage scr(
    emu.SCR.Data(),
    emu.SCR.Width(), emu.SCR.Height(), emu.SCR.BytesPerLine(),
    QImage::Format::Format_Indexed8
    );

  scr.setColorTable(_pallette);

  QPainter paint(this);
  paint.drawImage(QPoint(10, 80), scr);
}

//slots
void Chip8::screenInvalidated()
{
  doRender();
}


void Chip8::openGame()
{
  QString fileName;

  fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
    "",
    tr("Chip 8 Files (*.ch8);;All files (*.*)")
    );

  if (!fileName.isEmpty())
  {
    QFile progFile(fileName);
    if (progFile.open(QIODevice::ReadOnly))
    {
      QByteArray progData = progFile.readAll();
      emu.Init(Emulator::CHIP8);
      emu.storeProgram((uint8_t*)(progData.data()), progData.size());
    }
  }

}

void Chip8::zoomIn()
{
}

void Chip8::zoomOut()
{
}

void Chip8::play()
{
  if (!emuThread.isRunning()) {
    emuThread.start();
    ui.actionPauseEmulator->setChecked(false);
    // thread
    connect(&emuThread, SIGNAL(screenInvalidated()), this, SLOT(screenInvalidated()), Qt::BlockingQueuedConnection);
  }
}

void Chip8::pause()
{
  if (emuThread.isRunning()) {
    emuThread.stop();
    ui.actionStartEmulator->setChecked(false);
  }
}

