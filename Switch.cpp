/*
 * Switch.cpp
 *
 *  Created on: Nov 5, 2023
 *      Author:
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
// LaunchPad.h defines all the indices into the PINCM table
void Switch_Init(void){
    // write this
  IOMUX->SECCFG.PINCM[PB24INDEX] = 0x00040081; // input, no pull
  IOMUX->SECCFG.PINCM[PA18INDEX] = 0x00040081; // input, no pull
  IOMUX->SECCFG.PINCM[PA17INDEX] = 0x00040081; // input, no pull
  IOMUX->SECCFG.PINCM[PA16INDEX] = 0x00040081; // input, no pull
  /*  IOMUX->SECCFG.PINCM[PA28INDEX] = 0x00040081; // input, no pull
  IOMUX->SECCFG.PINCM[PA27INDEX] = 0x00040081; // input, no pull
  IOMUX->SECCFG.PINCM[PA16INDEX] = 0x00040081; // input, no pull
  IOMUX->SECCFG.PINCM[PA15INDEX] = 0x00040081; // input, no pull*/
}
// return current state of switches
uint32_t Switch_In(void){
    // write this
  // uint32_t data = GPIOA->DIN31_0;
  // data = ((data>>15)&0x03) | ((data&((1<<28)|(1<<27)))>>25);
  uint32_t data = GPIOA->DIN31_0 & 0x0070000;
  data |= (data | GPIOB->DIN31_0 & 0x1000000)>>5;
  return (data>>16); // return 0; //replace this your code
}