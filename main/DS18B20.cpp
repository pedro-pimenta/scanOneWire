#ifndef __DS18B20__
#define __DS18B20__

#include "DS18B20.h"
#include "delay.h"
#include <vector>
#include <string>
#include <cstring>

// global search state

// TEST BUILD
static unsigned char dscrc_table[] = {
	0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current
// global 'crc8' value.
// Returns current global crc8 value
//
unsigned char DS18B20::docrc8(unsigned char value)
{
	// See Application Note 27

	// TEST BUILD
	crc8 = dscrc_table[crc8 ^ value];
	return crc8;
}

void DS18B20::capturaBit(int posicao, char v[], int valor)
{
	unsigned char pbyte = posicao / 8;
	unsigned char pbit = posicao % 8;

	if (valor == 1)
		v[pbyte] |= (1 << pbit);
	else
		v[pbyte] &= ~(1 << pbit);
}

// Find the 'first' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : no device present
//
int DS18B20::fazScanPrimeiro()
{
	// reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag = FALSE;
	LastFamilyDiscrepancy = 0;

	return fazScan();
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int DS18B20::fazScanProximo()
{
	// leave the search state alone
	return fazScan();
}
int DS18B20::fazScan()
{
	int id_bit_number;
	int last_zero, rom_number, search_result;
	int normal, complemento;
	unsigned char rom_mask, search_direction;

	id_bit_number = 1;
	last_zero = 0;
	rom_number = 0;
	rom_mask = 1;
	search_result = 0;

	// if the last call was not the last one
	if (!LastDeviceFlag)
	{
		// 1-Wire reset
		if (!onewire->reset())
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}

		// issue the search command
		onewire->writeByte(0xF0);

		// loop to do the search
		while (rom_number < 8)
		{
			// read a bit and its complement
			normal = onewire->readBit();
			complemento = onewire->readBit();

			// check for no devices on 1-wire
			if ((normal == 1) && (complemento == 1))
				break;
			else
			{
				// all devices coupled have 0 or 1
				if (normal != complemento)
					search_direction = normal; // bit write value for search
				else
				{
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy)
						search_direction = ((address[rom_number] & rom_mask) > 0);
					else
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);

					// if 0 was picked then record its position in LastZero
					if (search_direction == 0)
					{
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1)
					address[rom_number] |= rom_mask;
				else
					address[rom_number] &= ~rom_mask;

				// serial number search direction write bit
				onewire->escreve_bit(search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_mask == 0)
				{
					docrc8(address[rom_number]); // accumulate the CRC
					rom_number++;
					rom_mask = 1;
				}
			}
		}

		// if the search was successful then
		if (!((id_bit_number < 65) || (crc8 != 0)))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = TRUE;

			search_result = TRUE;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !address[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}

	if (search_result)
	{
		printf("Codigo da Familia: %d\n", address[0]);
		printf("Numero de Serie  : %d %d %d %d %d %d\n", address[1], address[2], address[3], address[4], address[5], address[6]);
		printf("CRC=             : %d\n", address[7]);
		printf("Endereco completo: %d %d %d %d %d %d %d %d\n", address[0], address[1], address[2], address[3], address[4], address[5], address[6], address[7]);
	}

	return search_result;
}
// {
// 	int id_bit_number;
// 	int last_zero, rom_byte_number, search_result;
// 	int id_bit, cmp_id_bit;
// 	unsigned char rom_byte_mask, search_direction;

// 	// initialize for search
// 	id_bit_number = 1;
// 	last_zero = 0;
// 	rom_byte_number = 0;
// 	rom_byte_mask = 1;
// 	search_result = 0;
// 	crc8 = 0;

// 	// if the last call was not the last one
// 	if (!LastDeviceFlag)
// 	{
// 		// 1-Wire reset
// 		if (!onewire->reset())
// 		{
// 			// reset the search
// 			LastDiscrepancy = -1;
// 			LastDeviceFlag = FALSE;
// 			LastFamilyDiscrepancy = 0;
// 			return FALSE;
// 		}

// 		// issue the search command
// 		onewire->writeByte(SEARCH_ROM);

// 		printf("Escreveu SEARCH_ROM\n");

// 		// loop to do the search
// 		do
// 		{
// 			// read a bit and its complement
// 			id_bit = onewire->readBit();
// 			cmp_id_bit = onewire->readBit();

// 			printf("Comeco do loop\n");
// 			printf("id_bit = %d cmp_id_bit = %d\n", id_bit, cmp_id_bit);

// 			// check for no devices on 1-Wire
// 			if ((id_bit == 1) && (cmp_id_bit == 1))
// 				break;
// 			else
// 			{
// 				// all devices coupled have 0 or 1
// 				if (id_bit != cmp_id_bit)
// 					search_direction = id_bit; // bit write value for search
// 				else
// 				{
// 					// if this discrepancy is before the Last Discrepancy
// 					// on a previous next then pick the same as last time
// 					if (id_bit_number < LastDiscrepancy)
// 						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
// 					else
// 						// if equal to last pick 1, if not then pick 0
// 						search_direction = (id_bit_number == LastDiscrepancy);

// 					// if 0 was picked then record its position in LastZero
// 					if (search_direction == 0)
// 					{
// 						last_zero = id_bit_number;
// 						// check for Last discrepancy in family
// 						if (last_zero < 9)
// 							LastFamilyDiscrepancy = last_zero;
// 					}

// 					// printf("search_direction = %d\n", search_direction);
// 					// printf("last_zero = %d\n", last_zero);
// 					// printf("LastFamilyDiscrepancy = %d\n", LastFamilyDiscrepancy);
// 					// printf("LastDiscrepancy = %d\n", LastDiscrepancy);
// 					// printf("rom_byte_number = %d\n", rom_byte_number);
// 					// printf("rom_byte_mask = %d\n", rom_byte_mask);
// 					// printf("ROM_NO[rom_byte_number] = %d\n", ROM_NO[rom_byte_number]);
// 				}

// 				// set or clear the bit in the ROM byte rom_byte_number
// 				// with mask rom_byte_mask
// 				if (search_direction == 1)
// 					ROM_NO[rom_byte_number] |= rom_byte_mask;
// 				else
// 					ROM_NO[rom_byte_number] &= ~rom_byte_mask;

// 				// serial number search direction write bit
// 				onewire->escreve_bit(search_direction);

// 				// increment the byte counter id_bit_number
// 				// and shift the mask rom_byte_mask
// 				id_bit_number++;
// 				rom_byte_mask <<= 1;

// 				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
// 				if (rom_byte_mask == 0)
// 				{
// 					docrc8(ROM_NO[rom_byte_number]); // accumulate the CRC
// 					rom_byte_number++;
// 					rom_byte_mask = 1;
// 				}
// 			}
// 		} while (rom_byte_number < 8); // loop until through all ROM bytes 0-7

// 		printf("Fim do loop\n");

// 		// if the search was successful then
// 		if (!((id_bit_number < 65) || (crc8 != 0)))
// 		{
// 			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
// 			LastDiscrepancy = last_zero;

// 			// check for last device
// 			if (LastDiscrepancy == 0)
// 				LastDeviceFlag = TRUE;

// 			search_result = TRUE;
// 			printf("search_result = TRUE\n");
// 		}
// 	}

// 	// if no device found then reset counters so next 'search' will be like a first
// 	if (!search_result || !ROM_NO[0])
// 	{
// 		LastDiscrepancy = 0;
// 		LastDeviceFlag = FALSE;
// 		LastFamilyDiscrepancy = 0;
// 		search_result = FALSE;
// 		printf("search_result = FALSE\n");
// 	}

// 	if (search_result)// 	if (search_result)
// 	{
// 		printf("Codigo da Familia: %d\n", ROM_NO[0]);
// 		printf("Numero de Serie  : %d %d %d %d %d %d\n", ROM_NO[1], ROM_NO[2], ROM_NO[3], ROM_NO[4], ROM_NO[5], ROM_NO[6]);
// 		printf("CRC=             : %d\n", ROM_NO[7]);
// 		printf("Endereco completo: %d %d %d %d %d %d %d %d\n", ROM_NO[0], ROM_NO[1], ROM_NO[2], ROM_NO[3], ROM_NO[4], ROM_NO[5], ROM_NO[6], ROM_NO[7]);
// 	}
// 	{
// 		printf("Codigo da Familia: %d\n", ROM_NO[0]);
// 		printf("Numero de Serie  : %d %d %d %d %d %d\n", ROM_NO[1], ROM_NO[2], ROM_NO[3], ROM_NO[4], ROM_NO[5], ROM_NO[6]);
// 		printf("CRC=             : %d\n", ROM_NO[7]);
// 		printf("Endereco completo: %d %d %d %d %d %d %d %d\n", ROM_NO[0], ROM_NO[1], ROM_NO[2], ROM_NO[3], ROM_NO[4], ROM_NO[5], ROM_NO[6], ROM_NO[7]);
// 	}

// 	return search_result;
// }

// void DS18B20::fazScan(void)
// {
// 	char v[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// 	uint8_t normal, complemento;

// 	onewire->reset();
// 	onewire->writeByte(SEARCH_ROM);

// 	for (int x = 0; x < 64; x++)
// 	{
// 		normal = onewire->readBit();
// 		complemento = onewire->readBit();
// 		if (normal == 0 && complemento == 0)
// 		{
// 			// bits conflitantes na posicao

// 			onewire->escreve_bit(0);
// 			capturaBit(x, v, 0);
// 		}
// 		if (normal == 0 && complemento == 1)
// 		{
// 			// o bit é 0

// 			capturaBit(x, v, 0);
// 			onewire->escreve_bit(0);
// 		}
// 		if (normal == 1 && complemento == 0)
// 		{
// 			// o bit é 1

// 			capturaBit(x, v, 1);
// 			onewire->escreve_bit(1);
// 		}
// 		if (normal == 1 && complemento == 1)
// 		{
// 			// nao existem escravos no barramento
// 			printf("Nao existem escravos no barramento\n");
// 			return;
// 		}
// 	}
// 	printf("Codigo da Familia: %d\n", v[0]);
// 	printf("Numero de Serie  : %d %d %d %d %d %d\n", v[6], v[5], v[4], v[3], v[2], v[1]);
// 	printf("CRC=             : %d\n", v[7]);

// 	printf("Endereco completo: %d %d %d %d %d %d %d %d\n", v[7], v[6], v[5], v[4], v[3], v[2], v[1], v[0]);
// }

void DS18B20::init(void)
{
	uint8_t serial[6], crc;
	delay_ms(500);

	printf("INIT\n");
	if (onewire->reset() == 0)
		printf("Detectou escravo na linha\n");
	else
		printf("Nao detectou escravo\n");

	onewire->writeByte(READ_ROM);
	printf("Codigo da Familia: %d\n", onewire->readByte());
	for (uint8_t x = 0; x < 6; x++)
		serial[x] = onewire->readByte();

	printf("Numero de Serie  : %d %d %d %d %d %d\n", serial[0], serial[1], serial[2], serial[3], serial[4], serial[5]);

	crc = onewire->readByte();
	printf("CRC=             : %d\n", crc);

	delay_ms(1000);
}

void DS18B20::init(char v[])
{

	delay_ms(500);

	printf("INIT\n");
	if (onewire->reset() == 0)
		printf("Detectou escravo na linha\n");
	else
		printf("Nao detectou escravo\n");

	onewire->writeByte(READ_ROM);
	v[7] = onewire->readByte(); // family number
	printf("Codigo da Familia: %d\n", v[7]);

	for (uint8_t x = 6; x >= 1; x--)
		v[x] = onewire->readByte(); // serial

	printf("Numero de Serie  : %d %d %d %d %d %d\n", v[1], v[2], v[3], v[4], v[5], v[6]);

	v[0] = onewire->readByte();
	printf("CRC=             : %d\n", v[0]);

	delay_ms(1000);
}

// Usa o pino GPIO16 para fazer a comunicacao
DS18B20::DS18B20(gpio_num_t pino)
{
	DEBUG("DS18B20:Construtor\n");
	onewire = new ONEWIRE(pino);
}

char DS18B20::CRC(char end[])
{
	// calcula o CRC e retorna o resultado
	return 0;
}

float DS18B20::readTargetTemp(char vetor_64bits[])
{
	float temperatura;

	uint8_t a, b, inteira;
	float frac;

	onewire->reset();
	onewire->writeByte(MATCH_ROM);
	for (int x = 7; x >= 0; x--)
	{
		onewire->writeByte(vetor_64bits[x]);
	}
	onewire->writeByte(INICIA_CONVERSAO_TEMP);
	delay_ms(1000);
	onewire->reset();
	onewire->writeByte(MATCH_ROM);
	for (int x = 7; x >= 0; x--)
	{
		onewire->writeByte(vetor_64bits[x]);
	}
	onewire->writeByte(READ_TEMP_MEMORY);
	a = onewire->readByte();
	b = onewire->readByte();
	inteira = (b << 4) | (a >> 4);

	frac = ((a & 1) * (0.0625)) + (((a >> 1) & 1) * (0.125)) + (((a >> 2) & 1) * (0.25)) + (((a >> 3) & 1) * (0.5));

	temperatura = inteira + frac;
	return temperatura;
}

float DS18B20::readTemp(void)
{
	float temperatura;

	uint8_t a, b, inteira;
	float frac;

	onewire->reset();
	onewire->writeByte(SKIP_ROM);
	onewire->writeByte(INICIA_CONVERSAO_TEMP);
	delay_ms(1000);
	onewire->reset();
	onewire->writeByte(SKIP_ROM);
	onewire->writeByte(READ_TEMP_MEMORY);
	a = onewire->readByte();
	b = onewire->readByte();
	inteira = (b << 4) | (a >> 4);

	frac = ((a & 1) * (0.0625)) + (((a >> 1) & 1) * (0.125)) + (((a >> 2) & 1) * (0.25)) + (((a >> 3) & 1) * (0.5));

	temperatura = inteira + frac;
	return temperatura;
}

#endif
