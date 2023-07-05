// Inclusão de bibliotecas que o código depende
#include "onewire.h"
#include "digital.h"
#include "delay.h"

// O construtor da classe ONEWIRE. Ele recebe um número de pino como argumento e armazena esse número.
ONEWIRE::ONEWIRE(gpio_num_t p)
{
    DEBUG("ONEWIRE:construtor\n"); // Gera uma mensagem de depuração.
    PIN_DADOS = p;                 // Atribui o valor do pino à variável PIN_DADOS.
}

// Este método configura o pino como saída e define seu valor para LOW (0).
void ONEWIRE::low(void)
{
    digital.pinMode(PIN_DADOS, OUTPUT);   // Configura o pino como OUTPUT.
    digital.digitalWrite(PIN_DADOS, LOW); // Escreve o valor LOW no pino.
}

// Este método configura o pino como entrada, tornando-o um high-impedance input.
void ONEWIRE::high(void)
{
    digital.pinMode(PIN_DADOS, INPUT); // Configura o pino como INPUT.
}

uint8_t ONEWIRE::reset(void)
{
    uint8_t resposta;
    low();
    delay_us(500);
    high();
    delay_us(60); // Aumente este delay
    resposta = digital.digitalRead(PIN_DADOS);
    delay_us(600);    // Ajuste este delay se necessário
    return !resposta; // Inverta a resposta se necessário
}

// Este método lê um bit do barramento 1-Wire.
uint8_t ONEWIRE::readBit(void)
{
    uint8_t c;                          // Declaração de uma variável para armazenar o bit.
    low();                              // Chama a função low().
    delay_us(5);                        // Delay de 5 microssegundos.
    high();                             // Chama a função high().
    c = digital.digitalRead(PIN_DADOS); // Lê o valor do pino.
    delay_us(70);                       // Delay de 70 microssegundos.
    return c;                           // Retorna o bit lido.
}

// Este método lê um byte (8 bits) do barramento 1-Wire.
uint8_t ONEWIRE::readByte(void)
{
    uint8_t valor, x, c; // Declaração das variáveis.

    valor = 0;              // Inicia a variável valor com 0.
    for (x = 0; x < 8; x++) // Loop para ler 8 bits.
    {
        low();                              // Chama a função low().
        delay_us(5);                        // Delay de 5 microssegundos.
        high();                             // Chama a função high().
        c = digital.digitalRead(PIN_DADOS); // Lê o valor do pino.
        valor = (valor) | (c << x);         // Realiza uma operação OR bit a bit entre o valor existente e o bit lido.
        delay_us(70);                       // Delay de 70 microssegundos.
    }
    return valor; // Retorna o byte lido.
}

// Este método escreve um bit no barramento 1-Wire.
void ONEWIRE::escreve_bit(uint8_t meu_bit)
{
    if (meu_bit) // Se o bit a ser escrito for 1.
    {
        low();        // Chama a função low().
        delay_us(5);  // Delay de 5 microssegundos.
        high();       // Chama a função high().
        delay_us(60); // Delay de 60 microssegundos.
    }
    else // Se o bit a ser escrito for 0.
    {
        low();        // Chama a função low().
        delay_us(80); // Delay de 80 microssegundos.
        high();       // Chama a função high().
    }
    delay_us(5); // Delay de 5 microssegundos.
}

// Este método escreve um byte (8 bits) no barramento 1-Wire.
void ONEWIRE::writeByte(uint8_t v)
{
    DEBUG("ONEWIRE:writeByte\n"); // Gera uma mensagem de depuração.

    uint8_t x;              // Declaração da variável.
    for (x = 0; x < 8; x++) // Loop para escrever 8 bits.
    {
        escreve_bit((v >> x) & 1); // Chama a função escreve_bit com cada bit do byte.
    }
}
