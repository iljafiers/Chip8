#include "emulatorthread.h"
#include <iostream>

#include "Emulator.h"

EmulatorThread::EmulatorThread(Emulator *emu)
: QThread()
  
{
  c8emu = emu;
  stopped = false;
}

EmulatorThread::~EmulatorThread()
{

}

void EmulatorThread::run()
{
  stopped = false;
  while (!stopped)
  {
    c8emu->DoInstruction();
    msleep(1);
    if (c8emu->ScreenIsInvalidated()) {
      // send signal to UI
      emit screenInvalidated();
    }
  }

  emit threadExit();
}

void EmulatorThread::stop()
{
  stopped = true;
}
