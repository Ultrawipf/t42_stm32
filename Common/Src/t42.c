/*
 * t42.c
 *
 *  Created on: May 23, 2025
 *      Author: Yannick Richter
 *
 *  Game logic partially based on previous Tennis for two implementation from https://www.evilmadscientist.com/2008/resurrecting-tennis-for-two-a-video-game-from-1958/
 *  Original base project: 2007 Windell H. Oskay
 *  Distributed under the terms of the GNU General Public License (See LICENSE file)
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
volatile uint8_t bufferUpdBusy = 0;

GPIO_PinState lastLBtn = 1;
GPIO_PinState lastRBtn = 1;


#define g 0.8           //gravitational acceleration (should be positive.)
#define ts 0.12        // TimeStep TODO tune x16 due to higher resolution

#define historyLength 36

uint16_t xOldList[historyLength]; // Replace with dac buffer
uint16_t yOldList[historyLength];

float xOld; // a few x & y position values
float yOld; // a few x & y position values


float VxOld; //  x & y velocity values
float VyOld; //  x & y velocity values

float Xnew, Ynew, VxNew, VyNew;

uint8_t  deadball = 0;
uint8_t floorbounced = 0;

uint16_t Langle, Rangle;

uint16_t xp = 0;
uint16_t yp = 0;


uint8_t NewBall = 101;

unsigned int NewBallDelay = 0;

//Dummy variables:
uint8_t m = 0;

uint8_t server = LEFT;

uint8_t ballside;
uint8_t Lused = 0;
uint8_t Rused = 0;



void updateGame(){
	if(gameUpdateFlag){
		gameUpdateFlag = 0;
		updateGameState();
	}
	if(timUpdateFlag){
		timUpdateFlag = 0;
		if((drawState++ % 4) == 0){ // Alternate between buffers
			drawDynamic();
		}else{
			drawField();
		}
	}
}

void makeBallTrail(uint16_t length){
	uint16_t i = length;
	dyndacbuflen_cur = 0;
	uint16_t lastX = xOldList[length-1],lastY = yOldList[length-1];
	while(--i){
		uint16_t cursteps = (i/6) + 1;
		for(uint16_t j = 0;j<cursteps;j++){
			if(dyndacbuflen_cur >= DYN_DACBUFSIZE){
				return;
			}

			dyndacbufX[dyndacbuflen_cur] = (xOldList[i]+(lastX*(cursteps-1))) / cursteps;
			dyndacbufY[dyndacbuflen_cur] = (yOldList[i]+(lastY*(cursteps-1))) / cursteps;
			lastX = dyndacbufX[dyndacbuflen_cur];
			lastY = dyndacbufY[dyndacbuflen_cur];
			dyndacbuflen_cur++;

		}
	}
}

// Mostly old implementation
void updateGameState(){

	HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, !ballside && !deadball);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, ballside && !deadball);

	if (ballside != (xOld >= NETPOS))
	{
		ballside = (xOld >= NETPOS);

		if (ballside)
			Rused = 0;
		else
			Lused = 0;

		//CheckNet = 1;
		floorbounced = 0;
	}

	if (NewBall > 5) // IF ball has run out of energy, make a new ball!
	{
		NewBall = 0;
		deadball = 0;
		NewBallDelay = 1;
		server = ((ballside^(floorbounced&1)) != 0) ? LEFT : RIGHT; // New serve logic
		floorbounced = 0;
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

	if (!ballside)
	{
		Langle = 0xfff - getAnalogInput(0);

	}
	else
	{
		Rangle = 0xfff - getAnalogInput(1);
	}
	if (NewBallDelay)
	{
		if (ballside ? HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == 0 : HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == 0)
			NewBallDelay = 5000;

		NewBallDelay++;

		if (NewBallDelay > 2000)  // was 5000
			NewBallDelay = 0;


		VxNew = VxOld;
		VyNew = VyOld;
		Xnew = xOld;
		Ynew = yOld;
	}
	else
	{
		Xnew = xOld + VxOld;
		Ynew = yOld + VyOld - 0.5 * g * ts * ts;

		VyNew = VyOld - g * ts;
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
			VyNew *= 0.1;
			Xnew = MAXDACVAL;
			deadball = 1;
			NewBall = 100;
		}

		if (Ynew <= MINDACVAL)
		{
			Ynew = MINDACVAL;


			if (VyNew * VyNew < 10)
				NewBall++;

			if (VyNew < 0){
				VyNew *= -0.75;
				if(floorbounced){
					if(!deadball){
						deadball = 1;
						NewBall = 100; // Immediately respawn
					}
				}else{
					floorbounced = 1;
				}
			}
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
					Xnew = NETPOS + 2;
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
					Xnew = NETPOS - 2;
					deadball = 1;
				}
			}
		}
		GPIO_PinState rBtn = HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin);
		GPIO_PinState lBtn = HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin);
		float angleOfs = 2.07f;

		if (xOld < NETPOS - 10)
		{
			//	    	  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin,1);

			if (lBtn == 0 && lastLBtn != 0)
			{
				if ((Lused == 0) && (deadball == 0))
				{
					float angleF = Langle;
					float strength = MIN(angleF < 2048 ? HITSTRENGTH * (4096.0f-angleF)/1024.0f : HITSTRENGTH,30.0f);
					VxNew = (strength) * g * cos((float)0.001f * angleF - angleOfs);//costable[Langle];
					VyNew = g + strength * g * sin((float)0.001f * (float)Langle - angleOfs);

					Lused = 1;
					NewBall = 0;
					floorbounced = 0;
				}
			}

		}
		else if (xOld > NETPOS + 10)  // Ball on right side of screen
		{
			//	    	  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,1);

			if ( rBtn == 0 && lastRBtn != 0)
			{
				if ((Rused == 0) && (deadball == 0))
				{
					float angleF = 4096-Rangle;
					float strength = MIN(angleF < 2048 ? HITSTRENGTH * (4096.0f-angleF)/1024.0f : HITSTRENGTH,30.0f);
					VxNew = -strength * g * cos((float)0.001f * (float)Rangle - angleOfs);
					VyNew = g + -strength * g * sin((float)0.001f * (float)Rangle - angleOfs);

					Rused = 1;
					NewBall = 0;
					floorbounced = 0;
				}
			}

		}
		lastRBtn = rBtn;
		lastLBtn = lBtn;
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
	HAL_TIM_Base_Stop(&TIM_DAC); // Stop and reset DMA
	HAL_DACEx_DualSetValue(&DAC_OUT, DAC_ALIGN_12B_R, dacIdleX, dacIdleY); // Idle value to prevent visual glitches during off time
	HAL_DAC_Stop_DMA(&DAC_OUT, DAC_CHANNEL_1);
	HAL_DAC_Stop_DMA(&DAC_OUT, DAC_CHANNEL_2);
	}

// Helper to output DMA data to DACs
void outputDacs(const uint16_t* bufx,const uint16_t* bufy,uint32_t len){
	//stopDacs();

	// Upper 12b Chan2 lower 12b chan1
	HAL_DAC_Start_DMA(&DAC_OUT, DAC_CHANNEL_1, (uint32_t*)bufx, len, DAC_ALIGN_12B_R);
	HAL_DAC_Start_DMA(&DAC_OUT, DAC_CHANNEL_2, (uint32_t*)bufy, len, DAC_ALIGN_12B_R);
	// Start timer for DAC
	HAL_TIM_Base_Start(&TIM_DAC);
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
	if((curBufX == bufX && curBufY == bufY)){
		return; // Do nothing to prevent glitch
	}
	bufferUpdBusy = 1;
	//curBufLen = MIN(curBufLen,len);
	curBufCounter = 0;
	curBufX = bufX;
	curBufY = bufY;
	curBufLen = len;
	bufferUpdBusy = 0;
}

void loadNewDmaData(uint32_t dstbegin,uint32_t dstend){
	if(bufferUpdBusy){
		return; // Just cycle last buffer again
	}
	dacBusy = 1;
	for(uint32_t i = 0;i<dstend-dstbegin;i++){
		uint32_t j = curBufCounter++ % curBufLen; // Circular
		dacbufX[dstbegin+i] = curBufX[j];
		dacbufY[dstbegin+i] = curBufY[j];
	}
	dacBusy = 0;
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac){
	loadNewDmaData(DACBUFSIZE/2,DACBUFSIZE);
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac){
	loadNewDmaData(0,DACBUFSIZE/2);
}




void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){
	if(htim == &TIM_FRAME){
		// Change frame
		timUpdateFlag = 1;
	}else if(htim == &TIM_GAME){
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
	HAL_ADC_Start_DMA(&ADC_POTS, adcvals, 2);
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
	TIM_FRAME.Instance->CNT = TIM_FRAME.Instance->ARR / 2; // Offset a bit
	HAL_TIM_Base_Start_IT(&TIM_FRAME); // Game update timers
	HAL_TIM_Base_Start_IT(&TIM_GAME); // Game update timer
}
