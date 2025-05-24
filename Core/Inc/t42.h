/*
 * t42.c
 *
 *  Created on: May 23, 2025
 *      Author: Yannick
 */

#ifndef INC_T42_C_
#define INC_T42_C_

#include "main.h"
#define DYN_DACBUFSIZE 128
#define DACBUFSIZE 512


#define HITSTRENGTH 14


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

void setDacBuffer(const uint16_t* bufX,const uint16_t* bufY,uint32_t len);
void loadNewDmaData(uint32_t dstbegin,uint32_t dstend);


void makeBallTrail(uint16_t length);

uint16_t getAnalogInput(uint8_t chan);

#endif /* INC_T42_C_ */
