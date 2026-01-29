// Lab9HMain.cpp
// Runs on MSPM0G3507
// Lab 9 ECE319H
// Your name
// Last Modified: 12/26/2024

#include <cfenv>
#include <cstdint>
#include <machine/_stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/SlidePot.h"
#include "../inc/DAC5.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"

extern "C" void __disable_irq(void);
extern "C" void __enable_irq(void);
extern "C" void TIMG12_IRQHandler(void);
void Draw(void);
void Crash(void);
void GameBegin(void);
void GameOver(void);
void GameEnd(void);
void GameWin(void);
void GameDelay(void);
void redrawBG(void);

#define GREEN_LED 0x08000000
#define RED_LED 0x04000000

uint32_t Score = 0;
uint32_t Lives = 3;
char Score_Str[5];

void convertToString(uint32_t n){
    for (int i = 3; i >= 0; i--) {
        Score_Str[i] = (n % 10) + '0'; 
        n /= 10;
    }

    Score_Str[4] = '\0'; // null-terminate the string
}

// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}


SlidePot Sensor(1420,23); // copy calibration from Lab 7
uint32_t Flag = 0;

typedef enum {Avoid=0, Jump=1, Duck=2, Empty=4, Empty1=4, Empty2=4, Empty3=4} Obstacle_t;

struct sprite {
  uint32_t active = 0;
  Obstacle_t type;
  int32_t x;
  int32_t y;
  int32_t xold, yold;
  int32_t h,w;
  int32_t vx,vy;
  int32_t frame;
  int32_t lane; 
  uint32_t changelane;
  uint32_t jump;
  uint32_t duck;
  uint32_t runactive;
  const unsigned short *obstacleImage;
  const unsigned short *run[12];
  const unsigned short *jumpanim[3];
  const unsigned short *duckanim[9];
};
typedef struct sprite sprite_t;

sprite_t mainc = {
  .y = 154,
  .h = 26,
  .w = 27,
  .frame = 0,
  .runactive = 1,
  .jump = 0,
  .duck = 0,
  .run = {mainchar1, mainchar1, mainchar1, mainchar2, mainchar2, mainchar2, mainchar3, mainchar3, mainchar3, mainchar4, mainchar4, mainchar4},
  .jumpanim = {jump1, jump1, jump1},
  .duckanim = {duck1, duck1, duck1, duck2, duck2, duck2, duck1, duck1, duck1},
};

uint32_t lastYposlane[3] = {159, 159, 159};
sprite_t obstacles[10];
uint32_t obstacleIndex = 0;
uint32_t jumpframe; 
uint32_t duckframe;


// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){uint32_t pos,msg;
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// game engine goes here

    Score++;
    Flag = 1;
    // 1) sample slide pot
    uint32_t currData = Sensor.In();
    mainc.changelane = Sensor.Save(currData);
    // 2) read input switches
    if(mainc.jump==0 || mainc.duck==0) {
      uint32_t switchinput = Switch_In();
      mainc.jump = (switchinput & 0x1);
      if(mainc.jump) {
        jumpframe = 0;
        mainc.runactive = 0;
      }
      mainc.duck = (switchinput & 0x8)>>3;
      if(mainc.duck) {
        duckframe = 0;
        mainc.runactive = 0;
      }
    }
    // 3) move sprites
    for(int i = 0; i<3; i++) {
      if(lastYposlane[i]>70) {
        Obstacle_t randObs = (Obstacle_t) Random(5);
        if(!(randObs==Empty)) {
        if(!obstacles[obstacleIndex].active) {
          //obstacles[obstacleIndex] = new sprite_t();
          obstacles[obstacleIndex].type = randObs;
          obstacles[obstacleIndex].active = 1;
          obstacles[obstacleIndex].lane = i;
          // obstacles[obstacleIndex].lane = 1;
          obstacles[obstacleIndex].y = 0;
          if(randObs==0) {
            obstacles[obstacleIndex].x = (20+42*i)-13;
            obstacles[obstacleIndex].h = 26;
            obstacles[obstacleIndex].w = 27;
            obstacles[obstacleIndex].obstacleImage = mainchar;
          }
          else if(randObs==1) {
            obstacles[obstacleIndex].x = (20+42*i)-15;
            obstacles[obstacleIndex].h = 21;
            obstacles[obstacleIndex].w = 30;
            obstacles[obstacleIndex].obstacleImage = scooter;
          }
          else if(randObs==2) {
            obstacles[obstacleIndex].x = (20+42*i)-18;
            obstacles[obstacleIndex].h = 30;
            obstacles[obstacleIndex].w = 37;
            obstacles[obstacleIndex].obstacleImage = clubtent;
          }
          obstacleIndex = (obstacleIndex+1)%10;
        }
        }
      } 
    }
    int lowest[3] = {159,159,159};
    for(int j = 0; j<10; j++) {
      if(obstacles[j].active) {
        if((obstacles[j].y-obstacles[j].h)<lowest[obstacles[j].lane])
          lowest[obstacles[j].lane] = obstacles[j].y;
        // if(obstacles[j].lane==mainc.lane) {
        //   if(obstacles[j].type==0 && obstacles[j].y>134)
        //     Crash();
        //   if(obstacles[j].type==1 && obstacles[j].y>134 && Switch_In()!=1)
        //     Crash();
        //   if(obstacles[j].type==2 && obstacles[j].y>134 && Switch_In()!=8)
        //     Crash();
        // }
        if((obstacles[j].y-obstacles[j].h)>=159) {
          obstacles[j].active = 0;
        }
        else {
          obstacles[j].y++;
        }
      }
    }   
    lastYposlane[0] = lowest[0];
    lastYposlane[1] = lowest[1];
    lastYposlane[2] = lowest[2];

    // 4) start sounds
    // 5) set semaphore
    
    // 6) check for collisions here, 
      //check if obstacles and mc coords allign
      /*
    for (int i = 0; i <10; i++) {
      if((mainc.lane == obstacles[i].lane)&& 
          (mainc.x == obstacles[i].x)){

          Crash();
          SysTick_Wait10ms(100);
      }
    }*/
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}



uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};
// use main1 to observe special characters
int main1(void){ // main1
    char l;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
  ST7735_FillScreen(0x0000);            // set screen to black
  for(int myPhrase=0; myPhrase<= 2; myPhrase++){
    for(int myL=0; myL<= 3; myL++){
         ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         ST7735_OutString((char *)Phrases[myPhrase][myL]);
      ST7735_OutChar(13);
    }
  }
  Clock_Delay1ms(3000);
  ST7735_FillScreen(0x0000);       // set screen to black
  l = 128;
  while(1){
    Clock_Delay1ms(2000);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }
}

// use main2 to observe graphics
int main2(void){ // main2
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  // ST7735_DrawBitmap(22, 159, PlayerShip0, 18,8); // player ship bottom
  // ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
  // ST7735_DrawBitmap(42, 159, PlayerShip1, 18,8); // player ship bottom
  // ST7735_DrawBitmap(62, 159, PlayerShip2, 18,8); // player ship bottom
  // ST7735_DrawBitmap(82, 159, PlayerShip3, 18,8); // player ship bottom
  // ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
  // ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
  // ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
  // ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
  // ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);

  for(uint32_t t=500;t>0;t=t-5){
    SmallFont_OutVertical(t,104,6); // top left
    Clock_Delay1ms(50);              // delay 50 msec
  }
  ST7735_FillScreen(0x0000);   // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString((char *)"GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString((char *)"Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString((char *)"Earthling!");
  ST7735_SetCursor(2, 4);
  ST7735_OutUDec(1234);
  while(1){
  }
}

int main2a(void){ // main2a - test sprites  
  uint32_t last=0,now;
  Score = 0;  Lives = 3;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  SysTick_Init();
  Sensor.Init();
  Sound_Init();
  
    // initialize interrupts on TimerG12 at 30 Hz
  TimerG12_IntArm(2666667,2);
  // initialize all data structures
  
  __enable_irq();
  ST7735_FillScreen(SPEEDWAY_ORANGE); //0x20e4 blue but "orange"
  

  // ST7735_DrawBitmap(0, 159, speedwayBG1, 128, 160);
  // ST7735_DrawBitmap(20, 135, StartButton, 99, 37);
  ST7735_SetTextColor(0x00);
  // ST7735_OutString((char *)"TERMIN\x82" "Z!");

  
  // while(now!=4){
  //   now = Switch_In();
  //     if(now==4){ now = 0; break;}
  // }
  
  // GameBegin();
  // Crash();
  // Crash();
  // while(now!=4){
  //   now = Switch_In();
  //     if(now==4){ now = 0; break;}
  // }
  // Crash();
  while(1){
    
    // Crash();
    
    // while(now!=4){
    //   now = Switch_In();
    //     if(now==4){ now = 0; break;}
    // }
    if(Flag) {
    //uint32_t Position = Sensor.Distance();
      Flag = 0;
      Draw();
    // wait for semaphore
       // clear semaphore
       // update ST7735R
    // check for end game or level switch
    }
    if(Switch_In()==4){
      GameEnd();
      Clock_Delay1ms(500);
      break;
    }
    if((Switch_In() == 2) & (last == 0)){
      Sound_Stop();
      last = 1;
    }else if((Switch_In() == 2) & (last == 1)){
      Sound_BG();
      last = 0;
    }
  
  }
}

// use main3 to test switches and LEDs
int main3(void){ // main3
  uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  SysTick_Init();

  // LED_On(GREEN);
  while(1){
    // write code to test switches and LEDs

    // LED TESTING ::
    // LED_On(0x08000000);  //  Green
    // SysTick_Wait10ms(10); 
    // LED_Off(0x08000000);
    // SysTick_Wait10ms(10);
    // LED_On(0x04000000);     //red

    //turned on interchangeably
    // SysTick_Wait10ms(50);
    // LED_Toggle(RED);
    // LED_Toggle(GREEN); 

    //Switch testing
    // LED_Toggle(RED);
    // LED_Toggle(GREEN); 
    LED_Off(GREEN);
    LED_Off(RED);
    now = Switch_In();
    /*  ENTER (PB24) = 256(before shift) = 8(after shift)(0x08)
    *   OK (PA16) = 1 (0x01)
    *   NEXT (PA17) = 2(0x02)
    *   START (PA18) = 4(0x04)
    */

    if(now != last){
      LED_On(RED);
      SysTick_Wait10ms(50);
    }

    Clock_Delay(800000);  // 10ms, to debounce switch
     
  }
}


#define D0 8513
#define GF0 6757
#define A0 5682
#define B0 5062

// use main4 to test sound outputs
int main4(void){ uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(ADC0,6,0); // ADC1 channel 6 is PB20, TExaS scope
  
  // SysTick_Init();
  __enable_irq();
  while(1){
    now = Switch_In(); // one of your buttons
    if((last == 0)&&(now == 1)){
      // Sound_Shoot(); // call one of your sounds
      
      Sound_Crash();
      // Clock_Delay1ms(10);
      // Sound_Stop();
    }
    if((last == 0)&&(now == 2)){
      Sound_Killed(); // call one of your sounds
      // Sound_Start(surfers_crash, 11264);
      // Clock_Delay1ms(1000);
      Sound_Stop();
    }
    if((last == 0)&&(now == 4)){
      Sound_BGAlt(); // call one of your sounds
      // Sound_TestTone();
      // Clock_Delay1ms(1000);
    }
    if((last == 0)&&(now == 8)){
      Sound_BG(); // call one of your sounds
      // Clock_Delay1ms(1000);
    }
    // modify this to test all your sounds
  }
}
// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
int main(void){ // final main5
uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  // ST7735_FillScreen(ST7735_BLACK);
  ST7735_FillScreen(SPEEDWAY_ORANGE); 
  Sensor.Init(); // PB18 = ADC1 channel 5, slidepot
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  TimerG12_IntArm(2666667,2);
  // initialize all data structures
  for(uint32_t i = 0; i<10; i++) {
    //obstacles[i] = new sprite_t();
    obstacles[i].active = 0;
  }
  __enable_irq();
  GameBegin();
  // while(Switch_In()==4){}; // wait for S2 to be touched
  // while(Switch_In()!=4){}; // wait for S2 to be released
  // ST7735_DrawBitmap(0, 159, Speedway, 128, 160);
  Clock_Delay1ms(500);
  LED_Off(GREEN_LED);
  while(1){
    
    // ST7735_DrawBitmap(0, 159, speedwayBG1, 128, 160);
    // Clock_Delay1ms(10);
    // ST7735_DrawBitmap(0, 159, speedwayBG2, 128, 160);
    // Clock_Delay1ms(10);
      
    //Score
    ST7735_SetTextColor(0x00);
    SmallFont_OutVertical(Score,104,6); // top left
    // Clock_Delay1ms(50);              // delay 50 msec
    
    if(Score == 9999){
      GameWin();
    }
    //Sensor.Sync();
    if(Flag) {
    //uint32_t Position = Sensor.Distance();
      Flag = 0;
      Draw();
    // wait for semaphore
       // clear semaphore
       // update ST7735R
    // check for end game or level switch
    }
    if(Switch_In()==4){
      GameEnd();
      Clock_Delay1ms(500);
      break;
    }
    if((Switch_In() == 2) & (last == 0)){
      Sound_Stop();
      last = 1;
    }else if((Switch_In() == 2) & (last == 1)){
      Sound_BG();
      last = 0;
    }
  }
}


void Draw(void) {
  for(int i = 0; i<10; i++) {
    if(obstacles[i].active) {
      ST7735_DrawBitmap(obstacles[i].x, obstacles[i].y, obstacles[i].obstacleImage, obstacles[i].w, obstacles[i].h);
    }
  }
  if(mainc.jump) {
    ST7735_DrawBitmap(mainc.x, mainc.y, mainc.jumpanim[jumpframe], mainc.w, mainc.h);
    jumpframe++;
    if(jumpframe>=3) {
      mainc.jump = 0;
      mainc.runactive = 1;
    }
    // ST7735_DrawBitmap(mainc.x, mainc.y-5, jump1, mainc.w, mainc.h);
    // Clock_Delay1ms(50); 
    // ST7735_FillRect(mainc.x, 118, mainc.w, mainc.h, ST7735_BLACK); 
    // ST7735_DrawBitmap(mainc.x, mainc.y-5, jump1, mainc.w, mainc.h);
    // Clock_Delay1ms(50); 
    // ST7735_FillRect(mainc.x, 123, mainc.w, mainc.h, ST7735_BLACK); 
    // ST7735_DrawBitmap(mainc.x, mainc.y, jump1, mainc.w, mainc.h);
    // Clock_Delay1ms(50); 
    // // redrawBG();
  }
  if(mainc.duck) {
    ST7735_DrawBitmap(mainc.x, mainc.y, mainc.duckanim[duckframe], mainc.w, mainc.h);
    duckframe++;
    if(jumpframe>=9) {
      mainc.duck = 0;
      mainc.runactive = 1;
    }
    // Clock_Delay1ms(50);
    // ST7735_DrawBitmap(mainc.x, mainc.y, duck1, mainc.w, mainc.h);
    // Clock_Delay1ms(50);
    // ST7735_DrawBitmap(mainc.x, mainc.y, duck2, mainc.w, mainc.h);
    // Clock_Delay1ms(50);
    // ST7735_DrawBitmap(mainc.x, mainc.y, duck1, mainc.w, mainc.h);
    // Clock_Delay1ms(50);
  }
  if(mainc.changelane) {
    if(Sensor.Distance()<=480) {
      mainc.xold = mainc.x;
      mainc.x = 7;
      mainc.lane = 0;
    }
    else if(Sensor.Distance()<=961) {
      mainc.xold = mainc.x;
      mainc.x = 49;
      mainc.lane = 1;
    }
    else {
      mainc.xold = mainc.x;
      mainc.x = 91;
      mainc.lane = 2;
    }
  }
  if(mainc.runactive) {
    ST7735_FillRect(mainc.xold, 128, mainc.w, mainc.h, ST7735_BLACK);
    ST7735_DrawBitmap(mainc.x, mainc.y, mainc.run[mainc.frame], mainc.w, mainc.h);
    mainc.frame = (mainc.frame + 1) % 12;
  }

}


void GameBegin(void){
  uint32_t now;
  Score = 0;
  Lives = 3;
  
  ST7735_DrawBitmap(0, 159, speedwayBG1, 128, 160);
  ST7735_DrawBitmap(20, 135, StartButton, 99, 37);
  ST7735_SetTextColor(0x00);
  
  ST7735_SetCursor(4, 4);
  ST7735_OutString((char *)"SPEEDWAY");
  ST7735_SetCursor(5, 5);
  ST7735_OutString((char *)"SURFERS");
  now = Switch_In();
  while(now!=4){
    now = Switch_In();
      if(now==4){ now = 0; break;}
  }
  Clock_Delay1ms(250);
  LED_On(GREEN_LED);
  ST7735_FillScreen(SPEEDWAY_ORANGE);
  ST7735_SetCursor(3, 4);
  ST7735_SetTextColor(0x00);
  ST7735_OutString((char *)"SELECT LANGUAGE!");
  ST7735_SetCursor(4, 6);
  ST7735_OutString((char *)" 1.English");
  ST7735_SetCursor(4, 7);
  ST7735_OutString((char *)" 2.Fran\x87" "ais");
  ST7735_SetCursor(5, 9);
  ST7735_OutString((char *)"Press Start");
  ST7735_SetCursor(2, 10);
  ST7735_OutString((char *)"Appuyez commencer");
  uint32_t check;
  while(now!=4){
    now = Switch_In();
    if(Flag){
      Flag = 0;
      if(Sensor.Distance()<=730) {
        ST7735_FillRect(0, 59, 24, 21, SPEEDWAY_ORANGE);  
        ST7735_SetCursor(3, 6);
        ST7735_OutString((char *)">");
        myLanguage=English;
      }
      else{
          ST7735_FillRect(0, 59, 24, 21, SPEEDWAY_ORANGE);  
        ST7735_SetCursor(3, 7);
        ST7735_OutString((char *)">");
        myLanguage=French;
      }  
      if(now==4){ now = 0; break;}
  }
  }
  Clock_Delay1ms(100);
  Sound_BG(); 
  ST7735_DrawBitmap(0, 159, speedwayBG1, 128, 160);
}
//called when player crashes
void Crash(void){
  if(Lives<0) GameOver();
  Sound_Crash();
  for (int i = 0; i<3; i++){ 
    LED_On(RED_LED);
    // SysTick_Wait10ms(5);
     Clock_Delay1ms(100);
    LED_Off(RED_LED);
     Clock_Delay1ms(100);
  } 
  Sound_Stop();
  Lives--;
  if(Lives==0){
    GameOver();
  }
}
void GameOver(void){
  ST7735_FillScreen(SPEEDWAY_ORANGE);
  ST7735_SetTextColor(0x00);
  ST7735_SetCursor(5, 4);
  if(myLanguage==0){
    ST7735_OutString((char *)"GAME OVER!");
    ST7735_SetCursor(6, 6);
    ST7735_OutString((char *)"SCORE:");
    ST7735_SetCursor(7, 7);
    ST7735_OutString((char *)Score_Str);
  }else{
    ST7735_OutString((char *)"Jeu Termin\x82" "!");
    ST7735_SetCursor(6, 6);
    ST7735_OutString((char *)"Score:");
    ST7735_SetCursor(7, 7);
    ST7735_OutString((char *)Score_Str);
  }
  LED_On(RED_LED);
}

//called when game ended
void GameEnd(void){
  // ST7735_DrawBitmap(0, 159, speedwayBG1, 128, 160);
  ST7735_FillScreen(SPEEDWAY_ORANGE); 
  convertToString(Score);
  ST7735_SetTextColor(0x00);
  ST7735_SetCursor(5, 4);
  if(myLanguage==0){
    ST7735_OutString((char *)"GAME ENDED!");
    ST7735_SetCursor(6, 6);
    ST7735_OutString((char *)"SCORE:");
    ST7735_SetCursor(7, 7);
    ST7735_OutString((char *)Score_Str);
  }else{
    ST7735_OutString((char *)"Termin\x82" "z!");
    ST7735_SetCursor(5, 6);
    ST7735_OutString((char *)"Le Score:");
    ST7735_SetCursor(7, 7);
    ST7735_OutString((char *)Score_Str);
  }
  Sound_Stop();
  // ST7735_DrawBitmap(20, 135, StartButton, 99, 37);  //replace w/ pause
  LED_On(RED_LED);
}


void GameWin(void){
  ST7735_FillScreen(SPEEDWAY_ORANGE);
  ST7735_SetTextColor(0x00);
  ST7735_SetCursor(5, 4);
  if(myLanguage==0){
    ST7735_OutString((char *)"GAME WON!");
    ST7735_SetCursor(6, 6);
    ST7735_OutString((char *)"SCORE:");
    ST7735_SetCursor(7, 7);
    ST7735_OutString((char *)"9999");
  }else{
    ST7735_OutString((char *)"GAGNEZ!");
    ST7735_SetCursor(5, 6);
    ST7735_OutString((char *)"Le Score:");
    ST7735_SetCursor(7, 7);
    ST7735_OutString((char *)"9999");
  }
  Sound_Stop();
  LED_On(GREEN_LED);
}