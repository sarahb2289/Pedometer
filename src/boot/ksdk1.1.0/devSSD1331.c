#include <stdint.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

#define SEGMENT_WIDTH 2
#define SEGMENT_LENGTH 10
#define SEGMENT_GAP 1
#define XOFFSET 0
#define YOFFSET 10
#define CHARWIDTH 18


volatile uint8_t	inBuffer[32];
volatile uint8_t	payloadBytes[32];



/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMISO		= GPIO_MAKE_PIN(HW_GPIOA, 6),
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 1),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

int
writetoOLED(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	// OSA_TimeDelay(1);
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}

void
clearScreen(void)
{
	// Clear Screen
	writetoOLED(kSSD1331CommandCLEAR);
	writetoOLED(0x00);
	writetoOLED(0x00);
	writetoOLED(0x5F);
	writetoOLED(0x3F);
}

void
writeColour(void)
{
	writetoOLED(0x00);	
	writetoOLED(0xFF);
	writetoOLED(0x00);
	writetoOLED(0x00);
	writetoOLED(0xFF);
	writetoOLED(0x00);
}

void 
drawVertSegment(uint8_t topLeftX, uint8_t topLeftY)
{
	writetoOLED(kSSD1331CommandDRAWRECT);
	writetoOLED(topLeftX);
	writetoOLED(topLeftY);
	writetoOLED(topLeftX+SEGMENT_WIDTH-1);
	writetoOLED(topLeftY+SEGMENT_LENGTH-1);
	writeColour();
	
}

void 
drawHoSegment(uint8_t topLeftX, uint8_t topLeftY)
{
	writetoOLED(kSSD1331CommandDRAWRECT);
	writetoOLED(topLeftX);
	writetoOLED(topLeftY);
	writetoOLED(topLeftX+SEGMENT_LENGTH-1);
	writetoOLED(topLeftY+SEGMENT_WIDTH-1);
	writeColour();
	
}

void 
drawSegment(uint8_t segmentNum,uint8_t xoffset,uint8_t yoffset)
{
	uint8_t vertSegments[] = {1,2,4,5};
	uint8_t hoSegments[] = {0,3,6};
	uint8_t col1 = xoffset;
	uint8_t col2 = xoffset+SEGMENT_WIDTH;
	uint8_t col3 = xoffset+SEGMENT_WIDTH+SEGMENT_LENGTH;
	uint8_t row1 = yoffset;
	uint8_t row2 = yoffset+SEGMENT_WIDTH;
	uint8_t row3 = yoffset+SEGMENT_WIDTH+SEGMENT_LENGTH;
	uint8_t row4 = yoffset+2*SEGMENT_WIDTH+SEGMENT_LENGTH;
	uint8_t row5 = yoffset+2*SEGMENT_WIDTH+2*SEGMENT_LENGTH;
	if (segmentNum==0) {
		drawHoSegment(col2,row1);
	}
	if (segmentNum==1) {
		drawVertSegment(col3,row2);
	}
	if (segmentNum==2) {
		drawVertSegment(col3,row4);
	}
	if (segmentNum==3) {
		drawHoSegment(col2,row5);
	}
	if (segmentNum==4) {
		drawVertSegment(col1,row4);
	}
	if (segmentNum==5) {
		drawVertSegment(col1,row2);
	}
	if (segmentNum==6) {
		drawHoSegment(col2,row3);
	}
}

void 
drawChar(uint8_t charNum,uint8_t xoffset,uint8_t yoffset) {
	bool segMap[7];
	switch (charNum) {
		case 0:
		segMap[0] = 1;
		segMap[1] = 1;
		segMap[2] = 1;
		segMap[3] = 1;
		segMap[4] = 1;
		segMap[5] = 1;
		segMap[6] = 0;
		break;
		case 1:
		segMap[0] = 0;
		segMap[1] = 1;
		segMap[2] = 1;
		segMap[3] = 0;
		segMap[4] = 0;
		segMap[5] = 0;
		segMap[6] = 0;
		break;
		case 2:
		segMap[0] = 1;
		segMap[1] = 1;
		segMap[2] = 0;
		segMap[3] = 1;
		segMap[4] = 1;
		segMap[5] = 0;
		segMap[6] = 1;
		break;
		case 3:
		segMap[0] = 1;
		segMap[1] = 1;
		segMap[2] = 1;
		segMap[3] = 1;
		segMap[4] = 0;
		segMap[5] = 0;
		segMap[6] = 1;
		break;
		case 4:
		segMap[0] = 0;
		segMap[1] = 1;
		segMap[2] = 1;
		segMap[3] = 0;
		segMap[4] = 0;
		segMap[5] = 1;
		segMap[6] = 1;
		break;
		case 5:
		segMap[0] = 1;
		segMap[1] = 0;
		segMap[2] = 1;
		segMap[3] = 1;
		segMap[4] = 0;
		segMap[5] = 1;
		segMap[6] = 1;
		break;
		case 6:
		segMap[0] = 1;
		segMap[1] = 0;
		segMap[2] = 1;
		segMap[3] = 1;
		segMap[4] = 1;
		segMap[5] = 1;
		segMap[6] = 1;
		break;
		case 7:
		segMap[0] = 1;
		segMap[1] = 1;
		segMap[2] = 1;
		segMap[3] = 0;
		segMap[4] = 0;
		segMap[5] = 0;
		segMap[6] = 0;
		break;
		case 8:
		segMap[0] = 1;
		segMap[1] = 1;
		segMap[2] = 1;
		segMap[3] = 1;
		segMap[4] = 1;
		segMap[5] = 1;
		segMap[6] = 1;
		break;
		case 9:
		segMap[0] = 1;
		segMap[1] = 1;
		segMap[2] = 1;
		segMap[3] = 1;
		segMap[4] = 0;
		segMap[5] = 1;
		segMap[6] = 1;
		break;
	}

	if (segMap[0]) {
		drawSegment(0,xoffset,yoffset);
	}
	if (segMap[1]) {
		drawSegment(1,xoffset,yoffset);
	}
	if (segMap[2]) {
		drawSegment(2,xoffset,yoffset);
	}
	if (segMap[3]) {
		drawSegment(3,xoffset,yoffset);
	}
	if (segMap[4]) {
		drawSegment(4,xoffset,yoffset);
	}
	if (segMap[5]) {
		drawSegment(5,xoffset,yoffset);
	}
	if (segMap[6]) {
		drawSegment(6,xoffset,yoffset);
	}
}



int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	warpEnableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 1u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	// /*
	//  *	Drive /CS low.
	//  *
	//  *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	//  */
	// GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	// OSA_TimeDelay(10);
	// GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	// /*
	//  *	Drive DC low (command).
	//  */
	// GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writetoOLED(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writetoOLED(kSSD1331CommandSETREMAP);		// 0xA0
	writetoOLED(0x72);				// RGB Color
	writetoOLED(kSSD1331CommandSTARTLINE);		// 0xA1
	writetoOLED(0x0);
	writetoOLED(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writetoOLED(0x0);
	writetoOLED(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writetoOLED(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writetoOLED(0x3F);				// 0x3F 1/64 duty
	writetoOLED(kSSD1331CommandSETMASTER);		// 0xAD
	writetoOLED(0x8E);
	writetoOLED(kSSD1331CommandPOWERMODE);		// 0xB0
	writetoOLED(0x0B);
	writetoOLED(kSSD1331CommandPRECHARGE);		// 0xB1
	writetoOLED(0x31);
	writetoOLED(kSSD1331CommandCLOCKDIV);		// 0xB3
	writetoOLED(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writetoOLED(kSSD1331CommandPRECHARGEA);	// 0x8A
	writetoOLED(0x64);
	writetoOLED(kSSD1331CommandPRECHARGEB);	// 0x8B
	writetoOLED(0x78);
	writetoOLED(kSSD1331CommandPRECHARGEA);	// 0x8C
	writetoOLED(0x64);
	writetoOLED(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writetoOLED(0x3A);
	writetoOLED(kSSD1331CommandVCOMH);		// 0xBE
	writetoOLED(0x3E);
	writetoOLED(kSSD1331CommandMASTERCURRENT);	// 0x87
	writetoOLED(0x06);
	writetoOLED(kSSD1331CommandCONTRASTA);		// 0x81
	writetoOLED(0x91);
	writetoOLED(kSSD1331CommandCONTRASTB);		// 0x82
	writetoOLED(0x50);
	writetoOLED(kSSD1331CommandCONTRASTC);		// 0x83
	writetoOLED(0x7D);
	writetoOLED(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writetoOLED(kSSD1331CommandFILL);
	writetoOLED(0x01);

	/*
	 *	Clear Screen
	 */
	writetoOLED(kSSD1331CommandCLEAR);
	writetoOLED(0x00);
	writetoOLED(0x00);
	writetoOLED(0x5F);
	writetoOLED(0x3F);



	/*
	 *	Any post-initialization drawing commands go here.
	 */
	writetoOLED(kSSD1331CommandDRAWRECT);		//0x22
	writetoOLED(0x00);
	writetoOLED(0x00);
	writetoOLED(0x5F);
	writetoOLED(0x3F);
	writetoOLED(0x00);
	writetoOLED(0xFF);
	writetoOLED(0x00);
	writetoOLED(0x00);
	writetoOLED(0xFF);
	writetoOLED(0x00);

	// Wait 1 second
	OSA_TimeDelay(1000);

	/*
	* Clear Screen
	*/

	writetoOLED(kSSD1331CommandCLEAR);
	writetoOLED(0x00);
	writetoOLED(0x00);
	writetoOLED(0x5F);
	writetoOLED(0x3F);

	// Draw line

	// drawVertSegment(0,SEGMENT_WIDTH+SEGMENT_GAP);
	// drawHoSegment(SEGMENT_WIDTH+SEGMENT_GAP,0);
	// drawSegment(0);
	// drawSegment(1);
	// drawSegment(2);
	// drawSegment(3);
	// drawSegment(4);
	// drawSegment(5);
	// drawSegment(6);

	// uint32_t count = 0;
	
	// while (1) {

	// 	uint8_t digits[] = {0,0,0,0,0};
	// 	if (count>99999){
	// 		count = 0;
	// 	}
	// 	uint8_t digiti = 0;
	// 	uint32_t number = count;
	// 	while (number>0) {
	// 		digits[4-digiti] = number%10;
	// 		number /= 10;
	// 		digiti += 1;
	// 	} 
	// 	count += 1;

		
		
	// 	drawChar(digits[4],XOFFSET+4*CHARWIDTH,YOFFSET);
	// 	drawChar(digits[3],XOFFSET+3*CHARWIDTH,YOFFSET);
	// 	drawChar(digits[2],XOFFSET+2*CHARWIDTH,YOFFSET);
	// 	drawChar(digits[1],XOFFSET+CHARWIDTH,YOFFSET);
	// 	drawChar(digits[0],XOFFSET,YOFFSET);
	// 	OSA_TimeDelay(100);
	// 	// Clear Screen
	// 	writetoOLED(kSSD1331CommandCLEAR);
	// 	writetoOLED(0x00);
	// 	writetoOLED(0x00);
	// 	writetoOLED(0x5F);
	// 	writetoOLED(0x3F);

		// drawChar(1,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(2,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(3,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(4,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(5,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(6,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(7,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(8,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
		// drawChar(9,0,0);
		// OSA_TimeDelay(100);
		// // Clear Screen
		// writetoOLED(kSSD1331CommandCLEAR);
		// writetoOLED(0x00);
		// writetoOLED(0x00);
		// writetoOLED(0x5F);
		// writetoOLED(0x3F);
	
	
	// /*
	//  *	Drive /CS high
	//  */
	// GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	// }
	return 0;
}
