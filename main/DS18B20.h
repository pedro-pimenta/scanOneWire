#include <inttypes.h>
#include "onewire.h"

#include "digital.h"
#include "debug.h"
#include <string>

// definitions
#define FALSE 0
#define TRUE 1

class DS18B20
{

private:
	ONEWIRE *onewire;
	char CRC(char end[]);
	void capturaBit(int posicao, char v[], int valor);

public:
	DS18B20(gpio_num_t pino);
	float readTemp(void);
	float readTargetTemp(char end[]);

	void init(void);
	void init(char v[]);
	int fazScanPrimeiro();
	int fazScanProximo();
	int fazScan();
	unsigned char docrc8(unsigned char value);
	unsigned char address[8];
	int LastDiscrepancy = 0;
	int LastFamilyDiscrepancy = 0;
	int LastDeviceFlag = 0;
	unsigned char crc8 = 0;
};
