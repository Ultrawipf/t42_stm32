/*
 * t42.c
 *
 *  Created on: May 23, 2025
 *      Author: Yannick
 */


#include "t42.h"
#include "t42_constants.h"
#include "math.h"


uint32_t adcvals[2];

uint16_t dyndacbufX[DYN_DACBUFSIZE] = {0};
uint16_t dyndacbufY[DYN_DACBUFSIZE] = {0};

const uint16_t* curBufX;
const uint16_t* curBufY;
uint32_t curBufLen = 0;
uint32_t curBufCounter = 0;
uint16_t dacbufX[DACBUFSIZE] = {0};
uint16_t dacbufY[DACBUFSIZE] = {0};


uint32_t dyndacbuflen_cur = 1;
const uint32_t trailLength = 10; //

uint32_t dacIdleX = 0;
uint32_t dacIdleY = 0;

const uint16_t servePosRight = MAXDACVAL - (MAXDACVAL/10);
const uint16_t servePosLeft = MINDACVAL + (MAXDACVAL/10);
const uint16_t serveHeight = (MINDACVAL + NETHEIGHT);

#define LEFT 0
#define RIGHT 1


volatile uint8_t drawState = 0;

volatile uint8_t timUpdateFlag = 0;
volatile uint8_t gameUpdateFlag = 0;
volatile uint8_t dacBusy = 0;


// Old implementation
#define g 0.8           //gravitational acceleration (should be positive.)
#define ts 0.15        // TimeStep TODO tune

#define historyLength 10

//float sintable[64];
//float costable[64];

uint16_t xOldList[historyLength]; // Replace with dac buffer
uint16_t yOldList[historyLength];

float xOld; // a few x & y position values
float yOld; // a few x & y position values


float VxOld; //  x & y velocity values
float VyOld; //  x & y velocity values

float Xnew, Ynew, VxNew, VyNew;

uint8_t  deadball = 0;

uint16_t Langle, Rangle;

uint16_t xp = 0;
uint16_t yp = 0;


uint8_t NewBall = 101;

unsigned int NewBallDelay = 0;

//Dummy variables:
uint8_t k = 0;
uint8_t m = 0;

uint8_t server = LEFT;
//uint8_t CheckNet = 0;

uint8_t ballside;
uint8_t Lused = 0;
uint8_t Rused = 0;




void updateGame(){

	if(timUpdateFlag){
		timUpdateFlag = 0;
		if((drawState++ % 4) == 0){ // Alternate between buffers
			drawDynamic();
		}else{
			drawField();
		}
	}
	if(gameUpdateFlag){
		gameUpdateFlag = 0;
		updateGameState();
	}


}

void makeBallTrail(uint16_t length){
	uint16_t i = 0;
	dyndacbuflen_cur = 0;
	while(i<length){
		for(uint16_t j = 0;j<=i/2;j++){
			if(dyndacbuflen_cur >= DYN_DACBUFSIZE)
				return;

			dyndacbufX[dyndacbuflen_cur] = xOldList[i];
			dyndacbufY[dyndacbuflen_cur] = yOldList[i];
			dyndacbuflen_cur++;
		}
		++i;
	}
}

// Mostly old implementation
void updateGameState(){

	 HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,!ballside);
	 HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,ballside);

	if (ballside != (xOld >= NETPOS))
	  {
	    ballside = (xOld >= NETPOS);

	    if (ballside)
	      Rused = 0;
	    else
	      Lused = 0;

	    //CheckNet = 1;
	  }

	  if (NewBall > 10) // IF ball has run out of energy, make a new ball!
	  {
	    NewBall = 0;
	    deadball = 0;
	    NewBallDelay = 1;
	    server = (ballside == 0) ? LEFT : RIGHT;

	    if (server == LEFT)
	    {
	      xOld = (float)servePosRight;
	      VxOld = 0;// (float) -2*g;
	      ballside = 1;
	      Rused = 0;
	      Lused = 1;
	    }
	    else
	    {
	      xOld = (float)servePosLeft;
	      VxOld = 0;// (float) 2*g;
	      ballside = 0;
	      Rused = 1;
	      Lused = 0;
	    }

	    yOld = (float)serveHeight;

	    m = 0;
	    while (m < historyLength)
	    {
	      xOldList[m] = xOld;
	      yOldList[m] = yOld;
	      m++;
	    }
	  }

	  if(!ballside){
	      Langle = 0xfff - getAnalogInput(0);

	    }else{
	      Rangle = 0xfff - getAnalogInput(1);
	    }
	  if (NewBallDelay)
	    {
	      if (HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == 0 || HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == 0)
	        NewBallDelay = 10000;

	      NewBallDelay++;

	      if (NewBallDelay > 5000)  // was 5000
	        NewBallDelay = 0;

//	      m = 0;
////	      while (m < 255)
////	      {
//	        YOUT(yp);
//	        XOUT(xp);
////	        m++;
////	      }

	      VxNew = VxOld;
	      VyNew = VyOld;
	      Xnew = xOld;
	      Ynew = yOld;
	    }
	    else
	    {
	      Xnew = xOld + VxOld;
	      Ynew = yOld + VyOld - 0.5*g*ts*ts;

	      VyNew = VyOld - g*ts;
	      VxNew = VxOld;

	      // Bounce at walls
	      if (Xnew < MINDACVAL)
	      {
	        VxNew *= -0.05;
	        VyNew *= 0.1;
	        Xnew = 0.1;
	        deadball = 1;
	        NewBall = 100;
	      }

	      if (Xnew > MAXDACVAL)
	      {
	        VxNew *= -0.05;
	        Xnew = MAXDACVAL;
	        deadball = 1;
	        NewBall = 100;
	      }

	      if (Ynew <= MINDACVAL)
	      {
	        Ynew = MINDACVAL;

	        if (VyNew*VyNew < 10)
	          NewBall++;

	        if (VyNew < 0)
	          VyNew *= -0.75;
	      }

	      if (Ynew >= MAXDACVAL)
	      {
	        Ynew = MAXDACVAL;
	        VyNew = 0;
	      }

	      if (ballside)
	      {
	        if (Xnew < NETPOS)
	        {
	          if (Ynew <= NETHEIGHT)
	          {
	            // Bounce off of net
	            VxNew *= -0.5;
	            VyNew *= 0.5;
	            Xnew = NETPOS+2;
	            deadball = 1;

	          }
	        }
	      }

	      if (ballside == 0)
	      {
	        if (Xnew > NETPOS)
	        {
	          if (Ynew <= NETHEIGHT)
	          {
	            // Bounce off of net
	            VxNew *= -0.5;
	            VyNew *= 0.5;
	            Xnew = NETPOS-2;
	            deadball = 1;
	          }
	        }
	      }

	      // Simple routine to detect button presses: works, if the presses are slow enough.

	      if (xOld < NETPOS-10)
	      {
//	    	  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,1);
	        if (HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == 0)
	        {
	          if ((Lused == 0) && (deadball == 0))
	          {
	            VxNew = HITSTRENGTH*g*cos((float) 0.001 * (float)Langle - (float) 2.07);//costable[Langle];
	            VyNew = g + HITSTRENGTH*g*sin((float) 0.001 * (float)Langle - (float) 2.07);

	            Lused = 1;
	            NewBall = 0;
	          }
	        }
	      }
	      else if (xOld > NETPOS+10)  // Ball on right side of screen
	      {
//	    	  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,1);
	        if (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == 0)
	        {
	          if ((Rused == 0) && (deadball == 0))
	          {
	            VxNew = -HITSTRENGTH*g*cos((float) 0.001 * (float)Rangle - (float) 2.07);
	            VyNew = g + -HITSTRENGTH*g*sin((float) 0.001 * (float)Rangle - (float) 2.07);

	            Rused = 1;
	            NewBall = 0;
	          }
	        }
	      }
	    }

	    //Figure out which point we're going to draw.
	    xp = floor(Xnew);
	    yp = floor(Ynew);
	    //yp = 511 - (int) floor(Ynew);
//	    XOUT(xp);
//	    YOUT(yp);
	    // Draw trail

	    m = 0;
	    while (m < (historyLength - 1))
	    {
	      xOldList[m] = xOldList[m + 1];
	      yOldList[m] = yOldList[m + 1];
	      m++;
	    }

	    xOldList[(historyLength - 1)] = xp;
	    yOldList[(historyLength - 1)] = yp;
	    makeBallTrail(historyLength);
	    //Age variables for the next iteration
	    VxOld = VxNew;
	    VyOld = VyNew;

	    xOld = Xnew;
	    yOld = Ynew;

}

// Stop and reset DMA
void stopDacs(){
	HAL_TIM_Base_Stop(&htim2); // Stop and reset DMA
	HAL_DACEx_DualSetValue(&hdac, DAC_ALIGN_12B_R, dacIdleX, dacIdleY); // Idle value to prevent visual glitches during off time
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_2);
	}

// Helper to output DMA data to DACs
void outputDacs(const uint16_t* bufx,const uint16_t* bufy,uint32_t len){
	//stopDacs();
	dacBusy = 1;
	// Upper 12b Chan2 lower 12b chan1
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)bufx, len, DAC_ALIGN_12B_R);
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t*)bufy, len, DAC_ALIGN_12B_R);
	// Start timer for DAC
	HAL_TIM_Base_Start(&htim2);
}


// Draws game field
void drawField(){
	setDacBuffer(fieldX,fieldY,FIELDLENGTH);

}

// Draws dynamic buffer (ball)
void drawDynamic(){
	setDacBuffer(dyndacbufX,dyndacbufY,dyndacbuflen_cur);
}


void setDacBuffer(const uint16_t* bufX,const uint16_t* bufY,uint32_t len){
	if(curBufX == bufX && curBufY == bufY){
		return; // Do nothing to prevent glitch
	}
	curBufCounter = 0;
	curBufX = bufX;
	curBufY = bufY;
	curBufLen = len;
}

void loadNewDmaData(uint32_t dstbegin,uint32_t dstend){
	for(uint32_t i = 0;i<dstend-dstbegin;i++){
		uint32_t j = curBufCounter++ % curBufLen; // Circular
		dacbufX[dstbegin+i] = curBufX[j];
		dacbufY[dstbegin+i] = curBufY[j];
	}

}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac){
	loadNewDmaData(DACBUFSIZE/2,DACBUFSIZE);
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac){
	loadNewDmaData(0,DACBUFSIZE/2);
}




void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){
	if(htim == &htim6){
		// Change frame
		timUpdateFlag = 1;
	}else if(htim == &htim7){
		gameUpdateFlag = 1;
	}
}

void startDacs(){
	for(int i = 0;i<DYN_DACBUFSIZE;i++){
		dyndacbufX[i] = 0;
		dyndacbufY[i] = 0;
	}
	outputDacs(dacbufX,dacbufY,DACBUFSIZE); // Start cyclic output
}

uint16_t getAnalogInput(uint8_t chan){
	return adcvals[MIN(chan,1)];
}

// ADC in circular continuous mode
void startADC(){
	HAL_ADC_Start_DMA(&hadc, adcvals, 2);
}

void setupGame(){



//	uint8_t m = 0;
//	while (m < sizeof(sintable)/sizeof(float))
//	{
//		sintable[m] = sin((float) 0.0647 * (float)m - (float) 2.07);
//		costable[m] = cos((float) 0.0647 * (float)m - (float) 2.07);
//		m++;
//	}

	startADC();
	drawField();
	startDacs();

	HAL_TIM_Base_Start_IT(&htim6); // Game update timers
	HAL_TIM_Base_Start_IT(&htim7); // Game update timer
}
