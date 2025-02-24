#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>  // Para tipos como uint32_t, uint8_t, etc.
#include <stdbool.h> // Para o tipo bool
#include "pico/stdlib.h" // Para funções como sleep_ms, set_sys_clock_khz, etc.
#include "hardware/pio.h" // Para funções e tipos relacionados ao PIO
#include "hardware/clocks.h" // Inclui a função clock_get_hz

// Definição de tipo da estrutura que irá controlar a cor dos LED's
typedef struct {
    double red;
    double green;
    double blue;
} LedConfig;

typedef LedConfig RGB;

// Definição de tipo da matriz de LEDs
typedef LedConfig LedMatrix[5][5];

// Declarações das funções
uint32_t generateColorBinary(double red, double green, double blue);
uint ws2812Init(PIO pio);
void displayPattern(LedMatrix pattern, PIO pio, uint sm);
void clearLedMatrix(LedMatrix ledMatrix);

#endif // WS2812_H