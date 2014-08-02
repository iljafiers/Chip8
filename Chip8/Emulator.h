#pragma once

#include <stdint.h>
#include <vector>

#define HINIBBLE(x) ((x&0xF0)>>4)

static const uint8_t chip8_font[16][5] =
{
  { 0xF0, 0x90, 0x90, 0x90, 0xF0 },     // sprite '0'
  { 0x20, 0x60, 0x20, 0x20, 0x70 },     // sprite '1'
  { 0xF0, 0x10, 0xF0, 0x80, 0xF0 },     // sprite '2'
  { 0xF0, 0x10, 0xF0, 0x10, 0xF0 },     // sprite '3'
  { 0x90, 0x90, 0xF0, 0x10, 0x10 },     // sprite '4'
  { 0xF0, 0x80, 0xF0, 0x10, 0xF0 },     // sprite '5'
  { 0xF0, 0x80, 0xF0, 0x90, 0xF0 },     // sprite '6 
  { 0xF0, 0x10, 0x20, 0x40, 0x40 },     // sprite '7'
  { 0xF0, 0x90, 0xF0, 0x90, 0xF0 },     // sprite '8'
  { 0xF0, 0x90, 0xF0, 0x10, 0xF0 },     // sprite '9'
  { 0xF0, 0x90, 0xF0, 0x90, 0x90 },     // sprite 'A'
  { 0xE0, 0x90, 0xE0, 0x90, 0xE0 },     // sprite 'B'
  { 0xF0, 0x80, 0x80, 0x80, 0xF0 },     // sprite 'C' 
  { 0xE0, 0x90, 0x90, 0x90, 0xE0 },     // sprite 'D'
  { 0xF0, 0x80, 0xF0, 0x80, 0xF0 },     // sprite 'E'
  { 0xF0, 0x80, 0xF0, 0x80, 0x80 }      // sprite 'F'
};

class Emulator
{
public:
  // working mode
  enum ChipMode {
    CHIP8,							// normal mode
    SCHIP							// super chip mode
  } mode;

private:

  // registers, V0..VF and I
  static const int nrRegisters = 16;
  uint8_t V[nrRegisters];				// V0 to VF
  uint16_t I;							// special register I
  uint16_t PC;						// program counter

  // screen
  class Screen {
  private:
    size_t width;
    size_t height;
    size_t lineSize;
    std::vector<uint8_t> memory;

  public:
    Screen();
    size_t Width() const { return width; }
    size_t Height() const { return height; }
    size_t BytesPerLine() const { return width; }
    uint8_t* Data() const { return (uint8_t*)&memory[0]; }
    void Init(Emulator::ChipMode mode);
    void Clear();
    void SetPixel(int x, int y, bool on);
    bool GetPixel(int x, int y);
    void ScrollHor(int delta);                    // call with positive delta to scroll right, negative to scroll left
    void ScrollVer(int delta);                    // call with positive delta to scroll down, negative to scroll left
    bool DrawSprite(                              // draws a sprite on the screen, using xor draw. returns true if collision.
      uint8_t* sprite,                            // pointer to sprite data.
      int xpos, int ypos,                         // x,y position where to paint sprite
      size_t nr_bytes);                           // size of sprite in bytes. if zero, sprite is 16 x 16 pixels. if >0, sprite is 8 x nr_bytes.
  };

  // memory
  static const size_t memorySize = 4096;
  uint8_t memory[memorySize];

  // stack
  static const size_t stackSize = 16;
  uint16_t stack[stackSize];
  size_t SP;

  // HP48 flags
  static const int nrHPFlags = 8;
  uint8_t HP48[nrHPFlags];

  // sprites
  static const int fontOffset = 0;	// memory location for the 4x5 bits hexadecimal font

  // timers
  uint32_t DT;                      // delay timer. while>0, pause emulator. counts down @60hz
  uint32_t ST;                      // sound timer. while >0, beep plays. counts down @60hz
  uint64_t timer60Hz;               // current timer, counts uptime in nanoseconds

  // keys
  uint16_t keys;                    // key bitfield

  // errors
  bool errorOccured;
  bool exitCalled;
  bool screenInvalidated;
  std::wstring errorMessage;

private:
  void SetError(const wchar_t *szText);
  void SetScreenInvalidated(bool bInvalidated = true) { screenInvalidated = bInvalidated; }

public:
  Screen SCR;

public:
  void Init(ChipMode m);
  Emulator(void);
  ~Emulator(void);
  void storeProgram(uint8_t* data, size_t len);
  void DoInstruction();             // performs x instructions, exits if instructions done, or if exit called.
  bool ScreenIsInvalidated(bool reset = true);
  void DecreaseTimers();
  void SetKey(int idx, bool on);
  bool IsKeyPressed(int idx);
};

