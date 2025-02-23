#include "neopixel.h"
#include "ws2812.pio.h"  // Arquivo gerado pelo PIO para controle dos LEDs Neopixel

// Buffer para armazenar os valores dos LEDs
uint32_t pixels[NUM_LEDS];


void ws2812_program_init(PIO pio, uint sm, uint offset, uint pin, uint freq, bool rgbw) {
    // Configuração básica do PIO para o WS2812
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// Função para inicializar a matriz de LEDs Neopixel
void neopixel_init(uint pin, uint num_leds) {
    // Configura o PIO para controlar os LEDs Neopixel
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, pin, 800000, false);
}

// Função para definir a cor de um LED
void neopixel_set_rgb(uint32_t pixel_num, uint8_t r, uint8_t g, uint8_t b) {
    if (pixel_num < NUM_LEDS) {
        pixels[pixel_num] = ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b);
    }
}

// Função para atualizar a matriz de LEDs
void neopixel_show() {
    for (int i = 0; i < NUM_LEDS; i++) {
        pio_sm_put_blocking(pio0, 0, pixels[i] << 8u);
    }
}

// Função para limpar a matriz de LEDs (apagar todos os LEDs)
void neopixel_clear() {
    for (int i = 0; i < NUM_LEDS; i++) {
        pixels[i] = 0;  // Define todos os LEDs como apagados
    }
    neopixel_show();  // Atualiza a matriz de LEDs
}