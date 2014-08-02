#include "Emulator.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////
//
// Emulator::Screen class

Emulator::Screen::Screen()
{
  Init(CHIP8);
  Clear();
}

void Emulator::Screen::Init(Emulator::ChipMode mode)
{
  switch (mode) {
  default:
  case CHIP8:
    this->width = 64; this->height = 32;
    break;
  case SCHIP:
    this->width = 128; this->height = 64;
    break;
  }
  this->lineSize = width;
  memory.resize(lineSize*height);
  Clear();
}

void Emulator::Screen::Clear()
{
  std::fill(memory.begin(), memory.end(), 0);
}

void Emulator::Screen::SetPixel(int x, int y, bool on)
{
  if (x < 0 || x >= static_cast<int>(width) ||
    y < 0 || y >= static_cast<int>(height)
    )
  {
    return;   // todo: check wraparound
  }

  if (on)
    memory[y*lineSize + x] = 1;
  else
    memory[y*lineSize + x] = 0;
}

bool Emulator::Screen::GetPixel(int x, int y)
{
  if (x<0 || x>=width || y<0 || y>=height)
    return false;
  if (memory[y*lineSize + x] != 0)
    return true;
  else
    return false;
}

void Emulator::Screen::ScrollHor(int delta)
{
  if (delta > 0) {
    // scroll right
    for (int x = static_cast<int>(width)-1; x >= delta; x--) {
      for (int y = 0; y < static_cast<int>(height); y++) {
        SetPixel(x, y, GetPixel(x - delta, y));
      }
    }
  }
  else if (delta < 0)
  {
    // scroll left
    for (int x = 0; x < static_cast<int>(width)-1 + delta; x++) {
      for (int y = 0; y < static_cast<int>(height); y++) {
        SetPixel(x, y, GetPixel(x - delta, y));
      }
    }
  }
}

void Emulator::Screen::ScrollVer(int delta)
{
  if (delta > 0) {
    // scroll down
    for (int y = static_cast<int>(height)-1; y >= delta; y--)
      for (int x = 0; x < static_cast<int>(width); x++)
        SetPixel(x, y, GetPixel(x, y - delta));
  }
  else if (delta < 0) {
    // scroll up
    for (int y = 0; y < static_cast<int>(height)-1 + delta; y++)
      for (int x = 0; x < static_cast<int>(width); x++)
        SetPixel(x, y, GetPixel(x, y - delta));
  }
}

bool Emulator::Screen::DrawSprite(uint8_t* sprite, int xpos, int ypos, size_t nr_bytes)
{
  bool collision = false;
  if (nr_bytes > 0)
  {
    for (size_t line = 0; line < nr_bytes; line++)
      for (size_t pixel = 0; pixel < 8; pixel++)
      {
      if (sprite[line] & (0x80 >> pixel))   // is the pixel set in the sprite?
      {
        if (GetPixel(xpos + pixel, ypos + line)) {  // is the pixel set on the screen?
          // set it to off, this is a collison.
          SetPixel(xpos + pixel, ypos + line, false);
          collision = true;
        }
        else
        {
          // pixel was not yet on, do that now.
          SetPixel(xpos + pixel, ypos + line, true);
        }
      }
      }

  }
  return collision;
}


///////////////////////////////////////////////////////////////////////////
//
// Emulator class

Emulator::Emulator(void)
{
  Init(CHIP8);
}


Emulator::~Emulator(void)
{
}

void Emulator::Init(ChipMode m)
{
  mode = m;

  SCR.Init(mode);

  // zero all memory and registers
  memset(memory, 0, memorySize);
  memset(V, 0, nrRegisters);
  I = 0;

  // programs start at 0x200
  PC = 0x200;

  // make stack empty
  memset(stack, 0, _countof(stack));
  SP = 0;

  // reset HP48 flags
  for (size_t fl = 0; fl < nrHPFlags; fl++)
    HP48[fl] = 0;

  // copy font sprites to memory starting at 0. not sure if this is correct,
  // however LD F instruction is corrected for this.
  size_t offset = fontOffset;
  for (size_t sind = 0; sind < 16; sind++)
  {
    for (size_t bt = 0; bt < 5; ++bt)
    {
      memory[offset++] = chip8_font[sind][bt];
    }
  }

  // timers
  DT = ST = 0;

  // keys
  keys = 0;

  // error
  errorOccured = false;
  exitCalled = false;
  screenInvalidated = false;
  errorMessage.clear();

  // randomizer. set to fixed seed for easier debugging.
  srand(42);
}


void Emulator::storeProgram(uint8_t* data, size_t len)
{
  if (len <= (4096 - 512))
  {
    std::memcpy(&memory[0x200], data, len);
  }
}

void Emulator::SetError(const wchar_t *szText)
{
  errorOccured = true;
  errorMessage = szText;
}

bool Emulator::ScreenIsInvalidated(bool reset/*=true*/)
{
  bool res = screenInvalidated;
  if (reset)
    screenInvalidated = false;
  return res;
}


void Emulator::DoInstruction()
{
  uint16_t instruction;
  int parmX, parmY, parmN, parmKK;

  bool incrementPC = true;              // code sets this false if the program counter (PC)
  // should not be increased, for example if a jump is executed
  bool invalidInstruction = false;      // code sets this true if an unknown or invlaid instruction is 
  // executed

  // execute the instruction at memory[SP].
  // instructions are 16 bit, stored as MSB-LSB.
  instruction = (memory[PC] << 8) | memory[PC + 1];

  switch (instruction & 0xF000) {
  case 0x0000: // 00XX, several instructions
    switch (instruction & 0x0FFF)
    {
    case 0x00E0:  //00E0 Erase the screen
      SCR.Clear();
      SetScreenInvalidated();
      break;

    case 0x00EE:  //00EE Return from a CHIP-8 sub-routine
      if (SP == 0) {
        // stack underflow
        std::wstringstream ss;
        ss << std::hex << std::setw(4) << std::showbase << "Stack underflow, PC=" << SP;
        SetError(ss.str().c_str());
      }
      else
      {
        // decrease stack pointer, and jump to address
        // on the stack. stack stores address of call instruction,
        // so it needs to be incremented before use.
        PC = stack[--SP];
      }
      break;


    case 0x00FB:  //00FB Scroll 4 pixels right (***)
      SCR.ScrollHor(+4);
      SetScreenInvalidated();
      break;

    case 0x00FC:  //00FC Scroll 4 pixels left (***)
      SCR.ScrollHor(-4);
      SetScreenInvalidated();
      break;

    case 0x00FD:  //00FD Quit the emulator (***)
      SetError(L"Quit (00FD) called.");
      break;

    case 0x00FE:  //00FE Set CHIP-8 graphic mode (***)
      mode = CHIP8;
      SCR.Init(mode);
      SetScreenInvalidated();
      break;

    case 0x00FF:  //00FF Set SCHIP graphic mode (***)
      mode = SCHIP;
      SCR.Init(mode);
      SetScreenInvalidated();
      break;

    default:
      if ((instruction & 0x0FF0) == 0x00C0)
      {
        // 00CN Scroll down N lines (***)
        SCR.ScrollVer(instruction & 0x000F);
        screenInvalidated = true;
      }
      else
      {
        invalidInstruction = true;
      }
      break;
    }
    break;

  case 0x1000:  //1NNN Jump to NNN
    PC = instruction & 0x0FFF;
    incrementPC = false;
    break;

  case 0x2000:  // 2NNN Call CHIP-8 sub-routine at NNN (16 successive calls max)
    if (SP >= stackSize) {
      std::wstringstream ss;
      ss << "Stack overflow at PC=" << std::hex << std::showbase << std::setw(4) << SP;
      SetError(ss.str().c_str());
    }
    else
    {
      // store current program counter an the stack
      // and jump to location provided in lower 3 nibbles of instruction
      stack[SP++] = PC;
      PC = instruction & 0x0FFF;
      incrementPC = false;
    }
    break;

  case 0x3000:  //3XKK Skip next instruction if VX == KK
    parmX = (instruction & 0x0F00) >> 8;
    parmKK = (instruction & 0x00FF);
    if (V[parmX] == parmKK)
    {
      PC += 2;
    }
    break;

  case 0x4000:  //4XKK Skip next instruction if VX != KK
    parmX = (instruction & 0x0F00) >> 8;
    parmKK = (instruction & 0x00FF);
    if (V[parmX] != parmKK)
    {
      PC += 2;
    }
    break;

  case 0x5000:
    switch (instruction & 0x000F)
    {
    case 0x0:  //5XY0 Skip next instruction if VX == VY
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      if (V[parmX] == V[parmY])
      {
        PC += 2;
      }
      break;

    default:
      invalidInstruction = true;
      break;

    }
    break;

  case 0x6000:  //6XKK VX = KK
    parmX = (instruction & 0x0F00) >> 8;
    parmKK = (instruction & 0x00FF);
    V[parmX] = parmKK;
    break;

  case 0x7000:  //7XKK VX = VX + KK
    parmX = (instruction & 0x0F00) >> 8;
    parmKK = (instruction & 0x00FF);
    V[parmX] = V[parmX] + parmKK;
    break;

  case 0x8000:
    switch (instruction & 0x000F)
    {
    case 0x0:  //8XY0 VX = VY
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      V[parmX] = V[parmY];
      break;

    case 0x1: //8XY1 VX = VX OR VY
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      V[parmX] |= V[parmY];
      break;

    case 0x2:  //8XY2 VX = VX AND VY
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      V[parmX] &= V[parmY];
      break;

    case 0x3:  //8XY3 VX = VX XOR VY (*)
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      V[parmX] ^= V[parmY];
      break;

    case 0x4:  //8XY4 VX = VX + VY, VF = carry
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      V[0xF] = (V[parmX] + V[parmY] > 255 ? 1 : 0);  // store carry in VF
      V[parmX] += V[parmY];
      break;

    case 0x5:  //8XY5 VX = VX – VY, VF = not borrow
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      // VF is NOT BORROW
      V[0xF] = (V[parmX] > V[parmY] ? 1 : 0);
      V[parmX] -= V[parmY];
      break;

    case 0x6:  //8XY6 VX = VX SHR 1 (VX=VX/2), VF = carry
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      V[0xF] = V[parmX] & 0x01 ? 1 : 0;   // shift LSB out to VF
      V[parmX] = V[parmX] >> 1;
      break;

    case 0x7:  //8XY7 VX = VY – VX, VF = not borrow
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      // VF is NOT BORROW
      V[0xF] = (V[parmY] > V[parmX] ? 1 : 0);
      V[parmX] = V[parmY] - V[parmX];
      break;

    case 0xE:  //8XYE VX = VX SHL 1 (VX=VX*2), VF = carry
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      V[0xF] = V[parmX] & 0x80 ? 1 : 0;   // shift LSB out to VF
      V[parmX] = V[parmX] << 1;
      break;

    default:
      invalidInstruction = true;
      break;
    }
    break;

  case 0x9000:
    switch (instruction & 0x000F)
    {
    case 0x0:  //9XY0 Skip next instruction if VX != VY
      parmX = (instruction & 0x0F00) >> 8;
      parmY = (instruction & 0x00F0) >> 4;
      if (V[parmX] != V[parmY])
      {
        PC += 2;
      }
      break;

    default:
      invalidInstruction = true;
      break;

    }
    break;

  case 0xA000: //ANNN I = NNN
    I = instruction & 0x0FFF;
    break;

  case 0xB000:  //BNNN Jump to NNN + V0
    PC = (instruction & 0x0FFF) + V[0x0];
    incrementPC = false;
    break;

  case 0xC000:  //CXKK VX = Random number AND KK
    parmX = (instruction & 0x0F00) >> 8;
    parmKK = (instruction & 0x00FF);
    V[parmX] = rand() & parmKK;
    break;

  case 0xD000:  //DXYN Draws a sprite at (VX,VY) starting at M(I). VF = collision.
    //If N=0, draws the 16 x 16 sprite, else an 8 x N sprite.
    parmX = (instruction & 0x0F00) >> 8;
    parmY = (instruction & 0x00F0) >> 4;
    parmN = (instruction & 0x000F);
    if (SCR.DrawSprite(
      &memory[I],                     // memory location of sprite to draw
      V[parmX], V[parmY],             // position on screen
      parmN))                       // byte size of sprite. if 0, sprite is 16x16
    {
      // there was a collision.
      V[0xF] = 1;
    }
    else
    {
      V[0xF] = 0;
    }
    screenInvalidated = true;
    break;

  case 0xE000:
    switch (instruction & 0x00FF)
    {
    case 0x9E: //EX9E Skip next instruction if key VX pressed
      parmX = (instruction & 0x0F00) >> 8;
      if (IsKeyPressed(V[parmX]))
      {
        PC += 2;
      }
      break;

    case 0xA1: //EXA1 Skip next instruction if key VX not pressed
      parmX = (instruction & 0x0F00) >> 8;
      if (!IsKeyPressed(V[parmX]))
      {
        PC += 2;
      }
      break;

    default:
      invalidInstruction = true;
      break;
    }
    break;

  case 0xF000:
    switch (instruction & 0x00FF)
    {
    case 0x07: //FX07 VX = Delay timer
      parmX = (instruction & 0x0F00) >> 8;
      V[parmX] = DT;
      break;

    case 0x0A: //FX0A Waits a keypress and stores it in VX. todo
      parmX = (instruction & 0x0F00) >> 8;

      break;

    case 0x15:  //FX15 Delay timer = VX
      parmX = (instruction & 0x0F00) >> 8;
      DT = V[parmX];
      break;

    case 0x18:  //FX18 Sound timer = VX
      parmX = (instruction & 0x0F00) >> 8;
      ST = V[parmX];
      break;

    case 0x1E:  //FX1E I = I + VX
      parmX = (instruction & 0x0F00) >> 8;
      I += V[parmX];
      break;

    case 0x29: //FX29 I points to the 4 x 5 font sprite of hex char in VX
      parmX = (instruction & 0x0F00) >> 8;
      I = fontOffset + V[parmX] * 5;
      break;

    case 0x33:  //FX33 Store BCD representation of VX in M(I)…M(I+2)
      parmX = (instruction & 0x0F00) >> 8;
      parmKK = V[parmX];
      memory[I] = parmKK % 100; parmKK -= parmKK % 100;
      memory[I + 1] = parmKK % 10; parmKK -= parmKK % 10;
      memory[I + 2] = parmKK;
      break;

    case 0x55: //FX55 Save V0…VX in memory starting at M(I)
      parmX = (instruction & 0x0F00) >> 8;
      if (I + parmX >= memorySize)
      {
        std::wstringstream ss;
        ss << "Memory overflow at PC=" << std::hex << std::showbase << std::setw(4) << SP;
        SetError(ss.str().c_str());
      }
      else
      {
        for (int idx = 0; idx <= parmX; idx++)
          memory[I + idx] = V[idx];
      }
      break;

    case 0x65:  //FX65 Load V0…VX from memory starting at M(I)
      parmX = (instruction & 0x0F00) >> 8;
      if (I + parmX >= memorySize)
      {
        std::wstringstream ss;
        ss << "Memory overflow at PC=" << std::hex << std::showbase << std::setw(4) << SP;
        SetError(ss.str().c_str());
      }
      else
      {
        for (int idx = 0; idx <= parmX; idx++)
          V[idx] = memory[I + idx];
      }
      break;

    case 0x75:  //FX75 Save V0…VX (X<8) in the HP48 flags
      parmX = (instruction & 0x0F00) >> 8;
      if (parmX >= nrHPFlags)
      {
        std::wstringstream ss;
        ss << "HP48 flag " << parmX << "being written at PC=" << std::hex << std::showbase << std::setw(4) << SP;
        SetError(ss.str().c_str());
      }
      else
      {
        for (int idx = 0; idx <= parmX; idx++)
          HP48[idx] = V[idx];
      }
      break;

    case 0x85:  //FX85 Load V0…VX (X<8) from the HP48 flags (***)
      parmX = (instruction & 0x0F00) >> 8;
      if (I + parmX >= memorySize)
        if (parmX >= nrHPFlags)
        {
        std::wstringstream ss;
        ss << "HP48 flag " << parmX << "being read at PC=" << std::hex << std::showbase << std::setw(4) << SP;
        SetError(ss.str().c_str());
        }
        else
        {
          for (int idx = 0; idx <= parmX; idx++)
            V[idx] = HP48[idx];
        }
      break;

    default:
      invalidInstruction = true;
      break;
    }
    break;
  }

  if (invalidInstruction)
  {
    std::wstringstream ss;
    ss << "Unsupported instruction " << std::hex << std::showbase << std::setw(4) << instruction << " at PC=" << SP;
    SetError(ss.str().c_str());
  }
  else
  {
    // step PC to next instruction
    if (incrementPC) {
      PC += 2;
    }
  }



  // Handle timers. The delay timer DT and the sound timer DS
  // both count down at 60 Hz, if they are set by code.
  // While the sound timer is active, a beeper plays.
  // While the delay timer is active, execution of code is halted.
  //
  //uint64_t now = GetTickCount() * 1000;
  //while (timer60Hz < now) {
  //  if (DT > 0) DT--;
  //  if (ST > 0) ST--;
  //  timer60Hz += 16667;                   // 60Hz = 1,000,000 / 60 = 16666.666... nanoseconds
  //}
}

void Emulator::DecreaseTimers()
{
  if (DT > 0) {
    DT--;
  }
  if (ST > 0) {
    ST--;
  }
}

void Emulator::SetKey(int idx, bool on)
{
  if (on)
    keys |=  (1 << idx);
  else
    keys &= ~(1 << idx);
}

bool Emulator::IsKeyPressed(int idx)
{
  return (keys & (1 << idx)) ? true : false;
}
