/*
 *  IR receiver, sender, USB wakeup, motherboard switch wakeup, wakeup timer,
 *  USB HID device, eeprom emulation
 *
 *  Copyright (C) 2014 Joerg Riechardt
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#include "stm32f10x.h"
#include "usb_hid.h"
#include "irmpmain.h"
#include "irsndmain.h"
#include "eeprom.h"

#define SND_MAX 2

extern USB_OTG_CORE_HANDLE USB_OTG_dev;
USB_HID_RXSTATUS_t check=RX_EMPTY;
uint8_t buf[HID_OUT_BUFFER_SIZE-1];
uint16_t VirtAddVarTab[NumbOfVar] = {	0x0000, 0x1111, 0x2222, 0x3333, 0x4444, \
					0x5555, 0x6666, 0x7777, 0x8888, 0x9999, \
					0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
uint32_t timestamp = 0;
uint32_t AlarmValue = 0xFFFFFFFF;
volatile unsigned int systicks = 0;

void delay_ms(unsigned int msec)
{
	systicks = 0;
	while (systicks <= msec);
}

void LED_Switch_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	/* PB12 (blue LED) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	/* Config PB7 (mb switch pin) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	/* wakeup reset pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	/* start with LED on */
	GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET);
}

void toggle_LED(void)
{
	GPIOB->ODR ^= GPIO_Pin_12;
}

/*
 * eeprom: after writing 0xABCD into virtual address 0x6666, you have CDAB 6666 in flash
 * 2 halfwords in reverse order, little endian
 */

/* buf[BufIdx ... (BufIdx+5)] -> eeprom[TabNr ... (TabNr+2)] */
void Store_buf_to_Eeprom(uint8_t BufIdx, uint8_t TabNr)
{
	/* reverse order */
	EE_WriteVariable(VirtAddVarTab[TabNr], ((buf[(BufIdx + 1)] << 8) | (buf[BufIdx])));
	EE_WriteVariable(VirtAddVarTab[(TabNr + 1)], ((buf[(BufIdx + 3)] << 8) | (buf[(BufIdx + 2)])));
	EE_WriteVariable(VirtAddVarTab[(TabNr + 2)], ((buf[(BufIdx + 5)] << 8) | (buf[(BufIdx + 4)])));
}

/* eeprom[TabNr ... (TabNr+2)] -> buf[0-5] */
void Restore_Eeprom_to_buf(uint8_t TabNr)
{
	uint16_t EE_Data;
	EE_ReadVariable(VirtAddVarTab[TabNr], &EE_Data);
	/* reverse order */
	memcpy(&buf[0], &EE_Data, 2);
	EE_ReadVariable(VirtAddVarTab[(TabNr + 1)], &EE_Data);
	memcpy(&buf[2], &EE_Data, 2);
	EE_ReadVariable(VirtAddVarTab[(TabNr + 2)], &EE_Data);
	memcpy(&buf[4], &EE_Data, 2);
}

/*
 * IRData -> buf[1-6]
 * irmplircd expects dummy as first byte (Report ID),
 * so start with buf[0], adapt endianness for irmplircd
 */
void IRData_to_buf(IRMP_DATA *IRData)
{
	buf[0] = IRData->protocol;
	buf[2] = ((IRData->address) >> 8) & 0xFF;
	buf[1] = (IRData->address) & 0xFF;
	buf[4] = ((IRData->command) >> 8) & 0xFF;
	buf[3] = (IRData->command) & 0xFF;
	buf[5] = IRData->flags;
}

/* buf[BufIdx...(BufIdx+5)] -> IRData */
void buf_to_IRData(uint8_t buf[6], uint8_t BufIdx, IRMP_DATA *IRData)
{
	IRData->protocol = buf[BufIdx];
	IRData->address = ((buf[(BufIdx + 1)] << 8) | (buf[(BufIdx + 2)]));
	IRData->command = ((buf[(BufIdx + 3)] << 8) | (buf[(BufIdx + 4)]));
	IRData->flags = buf[(BufIdx + 5)];
}

/* buf[0-5] <-> IRData */
uint8_t cmp_buf_IRData(uint8_t buf[6], IRMP_DATA *IRData)
{
	return	IRData->protocol == buf[0] && \
		IRData->address == ((buf[1] << 8) | buf[2]) && \
		IRData->command == ((buf[3] << 8) | buf[4]) && \
		IRData->flags == buf[5];
}

void Systick_Init(void)
{
	/* 1ms */
	SysTick_Config((SysCtlClockGet()/1000));
}

void SysTick_Handler(void)
{
	static uint16_t i = 0;
	systicks++;
	timestamp++;
	if (i == 1000) {
		if (AlarmValue)
			AlarmValue--;
		i = 0;
	} else {
		i++;
	}
}

/* Val -> buf[BufIdx...BufIdx+3] */
void uint32_to_buf(uint32_t Val, uint8_t BufIdx)
{
	buf[BufIdx] = ((Val) >> 24) & 0xFF;
	buf[BufIdx+1] = ((Val) >> 16) & 0xFF;
	buf[BufIdx+2] = ((Val) >> 8) & 0xFF;
	buf[BufIdx+3] = Val & 0xFF;
}

void Wakeup(void)
{
	AlarmValue = 0xFFFFFFFF;
	/* USB wakeup */
	USB_OTG_ActiveRemoteWakeup(&USB_OTG_dev);
	/* motherboard switch: PB7 short high */
	GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET);
	delay_ms(500);
	GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
}

/* put wakeup IRData into buf and wakeup eeprom */
void store_new_wakeup(void)
{
	IRMP_DATA wakeup_IRData;
	toggle_LED();
	systicks = 0;
	/* 5 seconds to press button on remote */
	delay_ms(5000);
	if (irmp_get_data(&wakeup_IRData)) {
		/* wakeup_IRData -> buf[0-5] */
		IRData_to_buf(&wakeup_IRData);
		/* set flags to 0 */
		buf[5] = 0;
		/* buf[0-5] -> eeprom[0-2] */
		Store_buf_to_Eeprom(0, 0);
		toggle_LED();
	}
}

int main(void)
{
	uint8_t k, wakeup_buf[6], trigger_send_buf[6], send_buf[SND_MAX][6];
	IRMP_DATA myIRData, loopIRData, sendIRData, lastIRData = { 0, 0, 0, 0};

	LED_Switch_init();
	USB_HID_Init();
	IRMP_Init();
	IRSND_Init();
	FLASH_Unlock();
	EE_Init();
	Systick_Init();

	/* read wakeup IR-data from eeprom: eeprom[0-2] -> wakeup_buf */
	Restore_Eeprom_to_buf(0);
	memcpy(wakeup_buf, buf, sizeof(wakeup_buf));

	/* read trigger IR-data from eeprom: eeprom[3-5] -> trigger_send_buf */
	Restore_Eeprom_to_buf(3);
	memcpy(trigger_send_buf, buf, sizeof(trigger_send_buf));

	/* read IR-data to send from eeprom: eeprom[6-8] -> send_buf[0], eeprom[9-11] -> send_buf[1], etc */
	for (k=0; k<SND_MAX; k++) {
		Restore_Eeprom_to_buf(6+k*3);
		memcpy(send_buf[k], buf, sizeof(send_buf[k]));
	}

	while (1) {
		if (!AlarmValue)
			Wakeup();

		/* wakeup reset pin pulled low? */
		if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8)) {
			/* put wakeup IRData into buf and wakeup eeprom */
			store_new_wakeup();
			memcpy(wakeup_buf, buf, sizeof(wakeup_buf));
		}

		/* test if USB is connected to PC and data is received */
		if (USB_HID_GetStatus() == USB_HID_CONNECTED && USB_HID_ReceiveData(buf) == RX_READY) {

			switch (buf[0]) {
			/* set wakeup IRData */
			case 0xFF:
				/* buf[2-7] -> eeprom[0-2] */
				Store_buf_to_Eeprom(1, 0);
				memset(buf, 0, sizeof(buf));
				/* eeprom[0-2] -> buf[0-5] */
				Restore_Eeprom_to_buf(0);
				memcpy(wakeup_buf, buf, sizeof(wakeup_buf));
				break;


			/* set trigger_send IRData */
			case 0xFE:
				/* buf[2-7] -> eeprom[3-5] */
				Store_buf_to_Eeprom(1, 3);
				memset(buf, 0, sizeof(buf));
				/* eeprom[3-5] -> buf[0-5] */
				Restore_Eeprom_to_buf(3);
				memcpy(trigger_send_buf, buf, sizeof(trigger_send_buf));
				break;

			/* set send[0] IRData */
			case 0xFD:
				/* buf[2-7] -> eeprom[6-8] */
				Store_buf_to_Eeprom(1, 6);
				memset(buf, 0, sizeof(buf));
				/* eeprom[6-8] -> buf[0-5] */
				Restore_Eeprom_to_buf(6);
				memcpy(send_buf[0], buf, sizeof(send_buf[0]));
				break;

			/* set send[1] IRData */
			case 0xFC:
				/* buf[2-7] -> eeprom[9-11] */
				Store_buf_to_Eeprom(1, 9);
				memset(buf, 0, sizeof(buf));
				/* eeprom[9-11] -> buf[0-5] */
				Restore_Eeprom_to_buf(9);
				memcpy(send_buf[1], buf, sizeof(send_buf[1]));
				break;

			/* get wakeup IRData */
			case 0xFB:
				memset(buf, 0, sizeof(buf));
				memcpy(buf, wakeup_buf, sizeof(wakeup_buf));
				break;

			/* get trigger_send IRData */
			case 0xFA:
				memset(buf, 0, sizeof(buf));
				memcpy(buf, trigger_send_buf, sizeof(trigger_send_buf));
				break;

			/* get send[0] IRData */
			case 0xF9:
				memset(buf, 0, sizeof(buf));
				memcpy(buf, send_buf[0], sizeof(send_buf[0]));
				break;



			/* get send[1] IRData */
			case 0xF8:
				memset(buf, 0, sizeof(buf));
				memcpy(buf, send_buf[1], sizeof(send_buf[1]));
				break;

			/* IR send command */
			case 0xF4:
				/* buf[2-7] -> sendIRData */
				buf_to_IRData(buf, 1, &sendIRData);
				/* 0|1: don't|do wait until send finished */
				irsnd_send_data(&sendIRData, 1);
				memset(buf, 0, sizeof(buf));
				/* sendIRData -> buf[0-5] */
				IRData_to_buf(&sendIRData);
				/* timestamp -> buf[6-9] */
				uint32_to_buf(timestamp, 6);
				break;

			/* 4 halfwords in reverse order, little endian */
			/* set systick alarm */
			case 0xF3:
				/* buf[2-5] -> AlarmValue */
				AlarmValue = (buf[1]<<24)|(buf[2]<<16)|(buf[3]<<8)|buf[4];
				memset(buf, 0, sizeof(buf));
				/* AlarmValue -> buf[0-5] */
				uint32_to_buf(AlarmValue, 0);
				break;

			/* get systick alarm */
			case 0xF2:
				memset(buf, 0, sizeof(buf));
				/* AlarmValue -> buf[0-5] */
				uint32_to_buf(AlarmValue, 0);
				break;

			default:
				break;
			}

			/* send (modified) data (for verify) */
			USB_HID_SendData(0x02,buf, 10);
			toggle_LED();
		}

		/* poll IR-data */
		if (irmp_get_data(&myIRData)) {
			/* new IR-Data? */
			/* omit flags */
			if (!(	myIRData.protocol == lastIRData.protocol && \
				myIRData.address == lastIRData.address && \
				myIRData.command == lastIRData.command )) {

				toggle_LED();
				lastIRData.protocol = myIRData.protocol;
				lastIRData.address = myIRData.address;
				lastIRData.command = myIRData.command;
			}

			/* wakeup IR-data? */
			if (cmp_buf_IRData(wakeup_buf, &myIRData))
				Wakeup();

			/* trigger send IR-data? */
			if (cmp_buf_IRData(trigger_send_buf, &myIRData)) {
				for (k=0; k < SND_MAX; k++) {
					/* ?? 100 too small, 125 ok, RC5 is 114ms */
					delay_ms(115);
					/* send_buf[k] -> sendIRData */
					buf_to_IRData(send_buf[k], 0, &sendIRData);
					/* 0|1: don't|do wait until send finished */
					irsnd_send_data(&sendIRData, 1);
					/* ?? */
					delay_ms(300);
					/* receive sent by myself too, TODO */
					if (irmp_get_data(&loopIRData)) {
						memset(buf, 0, sizeof(buf));
						/* loopIRData -> buf[0-5] */
						IRData_to_buf(&loopIRData);
						/* timestamp -> buf[6-9] */
						uint32_to_buf(timestamp, 6);
						USB_HID_SendData(0x01,buf,10);
					}
				}
			}

			/* send IR-data via USB-HID */
			memset(buf, 0, sizeof(buf));
			/* myIRData -> buf[0-5] */
			IRData_to_buf(&myIRData);
			/* timestamp -> buf[6-9] */
			uint32_to_buf(timestamp, 6);
			USB_HID_SendData(0x01,buf, 10);
		}
	}
}