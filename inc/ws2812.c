#include "ws2812.h"
#include "ws2812.pio.h"

// Pino que realizará a comunicação do microcontrolador com a matriz
#define OUT_PIN 7

// Funções para matriz de LEDs
uint32_t generateColorBinary(double red, double green, double blue)
{
    unsigned char RED, GREEN, BLUE;
    RED = red * 255.0;
    GREEN = green * 255.0;
    BLUE = blue * 255.0;
    return (GREEN << 24) | (RED << 16) | (BLUE << 8);
}

uint ws2812Init(PIO pio) {
    bool ok;

    // Define o clock para 128 MHz, facilitando a divisão pelo clock
    ok = set_sys_clock_khz(128000, false);

    // Configurações da PIO
    uint offset = pio_add_program(pio, &ws2812_program);
    uint sm = pio_claim_unused_sm(pio, true);
    ws2812ProgramInit(pio, sm, offset, OUT_PIN);

    return sm;
}

void displayPattern(LedMatrix pattern, PIO pio, uint sm) {
    for (int row = 4; row >= 0; row--) {
        if (row % 2) {
            for (int col = 0; col < 5; col++) {
                uint32_t colorBinary = generateColorBinary(
                    pattern[row][col].red,
                    pattern[row][col].green,
                    pattern[row][col].blue
                );
                pio_sm_put_blocking(pio, sm, colorBinary);
            }
        } else {
            for (int col = 4; col >= 0; col--) {
                uint32_t colorBinary = generateColorBinary(
                    pattern[row][col].red,
                    pattern[row][col].green,
                    pattern[row][col].blue
                );
                pio_sm_put_blocking(pio, sm, colorBinary);
            }
        }
    }
}
void clearLedMatrix(LedMatrix matrix) {
    // Percorrer a matriz e configurar todos os LEDs como apagados
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            matrix[row][col].red = 0.0;
            matrix[row][col].green = 0.0;
            matrix[row][col].blue = 0.0;
        }
    }
}