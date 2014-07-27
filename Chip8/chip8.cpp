#include "chip8.h"
#include <QFileDialog>
#include <qbitmap.h>
#include <qpainter.h>
#include <qtimer.h>

Chip8::Chip8(QWidget *parent)
: QMainWindow( parent ),
  _emuThread( &_emu )
{
  ui.setupUi(this);

  connect(ui.actionOpenGame, SIGNAL(triggered()), this, SLOT(openGame()));
  connect(ui.actionZoomIn, SIGNAL(triggered()), this, SLOT(zoomIn()));
  connect(ui.actionZoomOut, SIGNAL(triggered()), this, SLOT(zoomOut()));
  // toolbar
  connect(ui.actionStartEmulator, SIGNAL(triggered()), this, SLOT(play()));
  connect(ui.actionPauseEmulator, SIGNAL(triggered()), this, SLOT(pause()));
  //thread
  connect(&_emuThread, SIGNAL(screenInvalidated()), this, SLOT(screenInvalidated()), Qt::BlockingQueuedConnection);
  connect(&_emuThread, SIGNAL(threadExit()), this, SLOT(threadExit()), Qt::BlockingQueuedConnection);

  initPallette();
  initBitmap();

  _scale = 5;

  // set up timer
  _timer = new QTimer(this);
  connect(_timer, SIGNAL(timeout()), this, SLOT(timerTick()));
  _timer->start(100);
}

void Chip8::initPallette()
{
  _pallette.clear();
  _pallette.append(QRgb(0xFF000000));
  _pallette.append(QRgb(0xFFFFFFFF));
  for (int dummy = 2; dummy < 256; dummy++)
    _pallette.append(QRgb(0xFF800000));

}

void Chip8::initBitmap()
{
  _scr = QImage(64, 32, QImage::Format::Format_Indexed8);
  _scr.setColorTable(_pallette);
}

Chip8::~Chip8()
{

}

// painting 

void Chip8::paintEvent(QPaintEvent *event)
{
  QPainter pnt(this);
  QRect target(10, 80, _scale*_scr.width(), _scale*_scr.height());
  pnt.drawImage(target, _scr);
}

//slot
void Chip8::screenInvalidated()
{
  _scr = QImage(
    _emu.SCR.Data(),
    _emu.SCR.Width(), _emu.SCR.Height(), _emu.SCR.BytesPerLine(),
    QImage::Format::Format_Indexed8
    );
  _scr.setColorTable(_pallette);

  update();
}

// thread stuff


void Chip8::timerTick()
{
  _emu.DecreaseTimers();
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
      _emu.Init(Emulator::CHIP8);
      _emu.storeProgram((uint8_t*)(progData.data()), progData.size());
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
  if (!_emuThread.isRunning()) {
    _emuThread.start();
    UpdateUI();
    // thread
  }
}

void Chip8::pause()
{
  if (_emuThread.isRunning()) {
    _emuThread.stop();
    UpdateUI();
  }
}

void Chip8::UpdateUI()
{
  ui.actionStartEmulator->setChecked(_emuThread.isRunning());
  ui.actionPauseEmulator->setChecked(_emuThread.isFinished());
}