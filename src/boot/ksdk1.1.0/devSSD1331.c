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
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 13),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

static int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	OSA_TimeDelay(10);
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
writeColour(void)
{
	writeCommand(0x00);	
	writeCommand(0xFF);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0xFF);
	writeCommand(0x00);
}

void 
drawVertSegment(uint8_t topLeftX, uint8_t topLeftY)
{
	writeCommand(kSSD1331CommandDRAWRECT);
	writeCommand(topLeftX);
	writeCommand(topLeftY);
	writeCommand(topLeftX+SEGMENT_WIDTH-1);
	writeCommand(topLeftY+SEGMENT_LENGTH-1);
	writeColour();
	
}

void 
drawHoSegment(uint8_t topLeftX, uint8_t topLeftY)
{
	writeCommand(kSSD1331CommandDRAWRECT);
	writeCommand(topLeftX);
	writeCommand(topLeftY);
	writeCommand(topLeftX+SEGMENT_LENGTH-1);
	writeCommand(topLeftY+SEGMENT_WIDTH-1);
	writeColour();
	
}

void 
drawSegment(uint8_t segmentNum)
{
	uint8_t vertSegments[] = {1,2,4,5};
	uint8_t hoSegments[] = {0,3,6};
	uint8_t col1 = 0;
	uint8_t col2 = 2;
	uint8_t col3 = 12;
	uint8_t row1 = 0;
	uint8_t row2 = 2;
	uint8_t row3 = 12;
	uint8_t row4 = 14;
	uint8_t row5 = 24;
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
	PORT_HAL_SetMuxMode(PORTB_BASE, 13u, kPortMuxAsGpio);
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

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0x50);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);

	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);



	/*
	 *	Any post-initialization drawing commands go here.
	 */
	writeCommand(kSSD1331CommandDRAWRECT);		//0x22
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);
	writeCommand(0x00);
	writeCommand(0xFF);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0xFF);
	writeCommand(0x00);

	// Wait 1 second
	OSA_TimeDelay(1000);

	/*
	* Clear Screen
	*/

	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);

	// Draw line

	// drawVertSegment(0,SEGMENT_WIDTH+SEGMENT_GAP);
	// drawHoSegment(SEGMENT_WIDTH+SEGMENT_GAP,0);
	drawSegment(0);
	OSA_TimeDelay(500);
	drawSegment(1);
	OSA_TimeDelay(500);
	drawSegment(2);
	OSA_TimeDelay(500);
	drawSegment(3);
	OSA_TimeDelay(500);
	drawSegment(4);
	OSA_TimeDelay(500);
	drawSegment(5);
	OSA_TimeDelay(500);
	drawSegment(6);
	

	return 0;
}
