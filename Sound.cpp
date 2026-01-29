// Sound.cpp
// Runs on MSPM0
// Sound assets in sounds/sounds.h
// your name
// your data 
#include <cstddef>
#include <cstdint>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "Sound.h"
#include "sounds/sounds.h"
#include "../inc/DAC5.h"
#include "../inc/Timer.h"

#define Frequency 227 // 11 KHz

// const uint32_t SinWave[32] = {16,19,22,24,27,28,30,31,31,31,30,28,27,24,22,19,16,13,10,8,5,4,2,1,1,1,2,4,5,8,10,13};
const uint8_t SinWave[32] = {16,19,22,24,27,28,30,31,31,31,30,28,27,24,22,19,16,13,10,8,5,4,2,1,1,1,2,4,5,8,10,13};
static const uint8_t *SoundPtr = 0;   // pointer to sound data
static uint32_t SoundIndex = 0;
static uint32_t SoundCount = 0;     
static uint32_t Index=0;
void Sound_Stop();


void SysTick_IntArm(uint32_t period, uint32_t priority){
  // write this
  SysTick->CTRL = 0;         // disable SysTick during setup
  SysTick->LOAD = period-1;  // reload value
  SysTick->VAL = 0;          // any write to current clears it
  SCB->SHP[1] = SCB->SHP[1]&(~0xC0000000)|(priority<<30); // set priority = 1
  SysTick->CTRL = 0x0007;    // enable SysTick with core clock and interrupts
}
// initialize a 11kHz SysTick, however no sound should be started
// initialize any global variables
// Initialize the 5 bit DAC
void Sound_Init(void){
// write this
  DAC5_Init();          // Port B is DAC
  Index = 0;
  SoundPtr = 0;
  SoundCount = 0;
  // SysTick_IntArm(10909, 1); // 11kHz ≈ 1/11025 → 10909 cycles for 48MHz
  SysTick_IntArm(7276, 1);  // For 11kHz interrupts with priority 1
  // SysTick_IntArm(7256, 1);  // For 11kHz interrupts with priority 1
  // SysTick_IntArm(Frequency, 1); // 11kHz ≈ 1/11025 → 10909 cycles for 48MHz
 
}
extern "C" void SysTick_Handler(void);
void SysTick_Handler(void){ // called at 11 kHz
  // output one value to DAC if a sound is active
  // output one value to DAC if a sound is active
  DAC5_Out(SoundPtr[Index++]);
  if(Index >= SoundCount) Index = 0;
  
}

//******* Sound_Start ************
// This function does not output to the DAC. 
// Rather, it sets a pointer and counter, and then enables the SysTick interrupt.
// It starts the sound, and the SysTick ISR does the output
// feel free to change the parameters
// Sound should play once and stop
// Input: pt is a pointer to an array of DAC outputs
//        count is the length of the array
// Output: none
// special cases: as you wish to implement
void Sound_Start(const uint8_t *pt, uint32_t count){
// write this
  Sound_Init();  // initialize sound
  if (pt == nullptr || count == 0) return; 
  SoundPtr   = pt;
  SoundCount  = count;
  Index = 0;
  //SysTick->LOAD = count - 1; //this resets the Systic to stop
  SysTick->VAL = 0;
  SysTick->CTRL |= 0x07;
}


void Sound_Stop(void){
  DAC5_Out(0);
  SysTick->LOAD= 0;
  // either set LOAD to 0 or clear bit 1 in CTRL
 
}
void Sound_Crash(void){
  // Sound_Start(surfers_crash, 11264); // example length; adjust based on actual sound
  
  Sound_Start(short_crash, 2816);

}

void Sound_TestTone(void){
  // Sound_Start(D0, 2000); // Play a tone
  
  Sound_Start(SinWave, 32); // Play a tone
}

void Sound_BG(void){
  // Sound_Start(SurfersBG2, 41689);
  Sound_Start(SurfersBG, 42635);
}

void Sound_Killed(void){
// write this
  // Sound_Start(invaderkilled, 4080);
  
  Sound_Start(short_crash, 2816);

}
