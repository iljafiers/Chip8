#ifndef EMULATORTHREAD_H
#define EMULATORTHREAD_H

class Emulator;

#include <QThread>

class EmulatorThread : public QThread
{
  Q_OBJECT

public:
  EmulatorThread(Emulator *);
  ~EmulatorThread();
  void stop();

signals:
  void screenInvalidated();
  void threadExit();

private:
  Emulator *c8emu;
  void run();

private:
  volatile bool stopped;
};

#endif // EMULATORTHREAD_H
