#ifndef __DS18B20__
#define __DS18B20__

#include "DS18B20.h"
#include "delay.h"

#define FALSE 0
#define TRUE  1
// global search state
typedef struct Node{
  char v;
  Node *left;
  Node *right;
}Node;

Node *newNode(char x){
  Node *node = new Node;
  node->v = x;
  node->left = node->right = NULL;
  return node;
}

Node *root = NULL;

void DS18B20::capturaBit (int posicao,char v[], int valor) // posicao de 0 a 63
{
	unsigned char pbyte = posicao / 8; // 0 a 7
	unsigned char pbit  = posicao % 8; // 0 a 7
	
	if (valor==1) v[pbyte] |= (1 << pbit); // seta o bit
	else v[pbyte] &= ~(1<< pbit); // reseta o bit
}


void DS18B20::fazScan (void)
{
	char v[8]={0,0,0,0,0,0,0,0};
	uint8_t normal, complemento;
	
	onewire->reset();
	onewire->writeByte(SEARCH_ROM);
	for (int x=0;x<64;x++)
	{
		normal = onewire->readBit();
		complemento = onewire->readBit();
		if (normal==0 && complemento==0)
		{	
			// bits conflitantes na posicao

			onewire->escreve_bit(0);
			capturaBit(x,v,0);

			if(x == 0){
				root = newNode(0);
				printf("root = %d\n",root->v);
			}
			else{
				Node *temp = root;
				if(temp->left == NULL){
					temp->left = newNode(0);
				}
			}
		}
		if (normal==0 && complemento==1)
		{
			// o bit é 0
			
			capturaBit(x,v,0);
			onewire->escreve_bit(0);

			if(x == 0){
				root = newNode(0);
				printf("root = %d\n",root->v);
			}
			else{
				Node *temp = root;
				if(temp->left == NULL){
					temp->left = newNode(0);
				}
			}
		}
		if (normal==1 && complemento==0)
		{
			// o bit é 1

			capturaBit(x,v,1);
			onewire->escreve_bit(1);

			if(x == 0){
				root = newNode(1);
				printf("root = %d\n",root->v);
			}
			else{
				Node *temp = root;
				if(temp->right == NULL){
					temp->right = newNode(1);
				}
			}
		}
		if (normal==1 && complemento==1)
		{
			// nao existem escravos no barramento
			printf("Nao existem escravos no barramento\n");
			return;
		}
		
	}
	printf("Codigo da Familia: %d\n",v[0]);
		printf("Numero de Serie  : %d %d %d %d %d %d\n",v[6],v[5],v[4],v[3],v[2],v[1]);
	printf("CRC=             : %d\n",v[7]);
	
	printf("Endereco completo: %d %d %d %d %d %d %d %d\n",v[7],v[6],v[5],v[4],v[3],v[2],v[1],v[0]);

}

void DS18B20::init (void)
{
	uint8_t serial[6],crc;
	delay_ms(500);

	printf("INIT\n");
	if (onewire->reset()==0) printf("Detectou escravo na linha\n");
	else printf("Nao detectou escravo\n");

	onewire->writeByte(READ_ROM);
	printf("Codigo da Familia: %d\n",onewire->readByte());
	for (uint8_t x=0;x<6;x++) serial[x] = onewire->readByte();

    printf("Numero de Serie  : %d %d %d %d %d %d\n",serial[0],serial[1],serial[2],serial[3],serial[4],serial[5]);

 	crc = onewire->readByte();
 	printf("CRC=             : %d\n",crc);

 	delay_ms(1000);


}
void DS18B20::init (char v[])
{

	delay_ms(500);

	printf("INIT\n");
	if (onewire->reset()==0) printf("Detectou escravo na linha\n");
	else printf("Nao detectou escravo\n");

	onewire->writeByte(READ_ROM);
	v[7] = onewire->readByte(); // family number
	printf("Codigo da Familia: %d\n",v[7]);
	
	for (uint8_t x=6;x>=1;x--) 
		v[x] = onewire->readByte(); //serial

    printf("Numero de Serie  : %d %d %d %d %d %d\n",v[1],v[2],v[3],v[4],v[5],v[6]);

 	v[0] = onewire->readByte();
 	printf("CRC=             : %d\n",v[0]);

 	delay_ms(1000);


}



// Usa o pino GPIO16 para fazer a comunicacao
DS18B20::DS18B20 (gpio_num_t pino)
{
	DEBUG("DS18B20:Construtor\n");
	onewire = new ONEWIRE(pino);
}

char DS18B20::CRC (char end[])
{
	// calcula o CRC e retorna o resultado
	return 0;
}
float DS18B20::readTargetTemp (char vetor_64bits[])
{
	float temperatura;

	uint8_t a,b,inteira;
	float frac;

	onewire->reset();
	onewire->writeByte(MATCH_ROM);
	for (int x=7;x>=0;x--)
	{
		onewire->writeByte(vetor_64bits[x]);
	}
	onewire->writeByte(INICIA_CONVERSAO_TEMP);
	delay_ms(1000);
	onewire->reset();
	onewire->writeByte(MATCH_ROM);
	for (int x=7;x>=0;x--)
	{
		onewire->writeByte(vetor_64bits[x]);
	}
	onewire->writeByte(READ_TEMP_MEMORY);
	a = onewire->readByte();
    b = onewire->readByte();
    inteira = (b<<4)|(a >> 4);


    	frac=((a & 1)*(0.0625))+ (((a>>1) & 1)*(0.125)) + (((a>>2) & 1)*(0.25)) + (((a>>3) & 1)*(0.5));
     
            temperatura = inteira + frac;
  return temperatura;     


	
}
float DS18B20::readTemp (void)
{
	float temperatura;

	uint8_t a,b,inteira;
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
    inteira = (b<<4)|(a >> 4);


    	frac=((a & 1)*(0.0625))+ (((a>>1) & 1)*(0.125)) + (((a>>2) & 1)*(0.25)) + (((a>>3) & 1)*(0.5));
     
            temperatura = inteira + frac;
  return temperatura;     


	
}

#endif
