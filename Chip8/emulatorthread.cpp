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
  while (!stopped)
  {
    c8emu->DoInstruction();
    if (c8emu->ScreenIsInvalidated()) {
      // send signal to UI
      emit screenInvalidated();
    }
  }

  stopped = false;
  emit threadExit();
}

void EmulatorThread::stop()
{
  stopped = true;
}
