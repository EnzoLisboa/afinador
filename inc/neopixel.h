#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

// Definições
#define NEOPIXEL_PIN 7  // Pino onde a matriz de LEDs está conectada (GP7 na BitDogLab)
#define NUM_LEDS 25     // Número de LEDs na matriz (5x5)

// Inicializa a matriz de LEDs Neopixel
void neopixel_init(uint pin, uint num_leds);

// Define a cor de um LED específico
void neopixel_set_rgb(uint32_t pixel_num, uint8_t r, uint8_t g, uint8_t b);

// Atualiza a matriz de LEDs com as cores definidas
void neopixel_show();

// Limpa a matriz de LEDs (apaga todos os LEDs)
void neopixel_clear();

void ws2812_program_init(PIO pio, uint sm, uint offset, uint pin, uint freq, bool rgbw);


#endif // NEOPIXEL_H