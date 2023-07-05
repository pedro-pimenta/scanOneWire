/* Detectou escravo na linha
Codigo da Familia: 40
Numero de Serie  : 63 161 4 5 0 0
CRC=             : 18
*/
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "serial.h"
#include "DS18B20.h"
#include "delay.h"
#include <inttypes.h>
#include "digital.h"
#include <vector>

extern "C" void app_main();

void printFloat(float f)
{
  char str[20];
  sprintf(str, "%d.%04d\n", (int)f, (int)(f * 10000) % 10000);
  printf(str);
}

unsigned char ROM_NO[8];

void app_main()
{
  delay_ms(2000);
  serial.begin(9600);
  printf("\n\nRODANDO\n\n");

  int i;
  // ROM_NO é um vetor de 8 bytes

  DS18B20 meuSensor = DS18B20(PIN16);

  // find ALL devices
  printf("\nFIND ALL\n");
  int cnt = 0;
  int rslt = meuSensor.fazScanPrimeiro();
  while (rslt)
  {
    // print device found
    for (i = 7; i >= 0; i--)
      printf("%02X", ROM_NO[i]);
    printf("  %d\n", ++cnt);

    rslt = meuSensor.fazScanProximo();
  }
}
