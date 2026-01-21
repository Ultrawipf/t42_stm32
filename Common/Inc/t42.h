/*
 * t42.c
 *
 *  Created on: May 23, 2025
 *      Author: Yannick Richter
 */

#ifndef INC_T42_C_
#define INC_T42_C_

#include "main.h"
#define DYN_DACBUFSIZE 190
#define DACBUFSIZE 380


#define HITSTRENGTH 25.0f
#define HITSTRENGTHSMASH 40.0f

#define AIRRESISTANCE 0.003
#define SMASHANGLE 2100

#define g 0.8           //gravitational acceleration (should be positive.)
#define ts 0.08        // TimeStep TODO tune x16 due to higher resolution

#define historyLength 37
#define BUFSWAPCNT 3 // How often to draw the field for one ball update
#define BALLTRAILDIV 4


#define MIN(a,b) (a < b ? a : b)

void drawField();
void drawDynamic();

void updateGame();
void updateGameState();
void setupGame();

void startDacs();
void startADC();

void stopDacs();
void outputDacs(const uint16_t* bufx,const uint16_t* bufy,uint32_t len);

void setDacBuffer(const uint16_t* bufX,const uint16_t* bufY,uint32_t len,uint32_t* bufcounter);
void loadNewDmaData(uint32_t dstbegin,uint32_t dstend);


void makeBallTrail(uint16_t length);

uint16_t getAnalogInput(uint8_t chan);

#endif /* INC_T42_C_ */
