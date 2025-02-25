#ifndef NOTES_H
#define NOTES_H

#include <stdint.h>
#include "ws2812.h" // Inclui a definição de LedConfig e LedMatrix

// Definição das matrizes para cada nota (5x5)
// Cada matriz é um array de 25 elementos (5x5), onde 1 representa um LED aceso e 0 representa um LED apagado.
const uint8_t notes[7][25] = {
    // Nota C
    { 0, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 0, 0, 0, 0,
      1, 0, 0, 0, 0,
      0, 1, 1, 1, 1 },

    // Nota D
    { 1, 1, 1, 1, 0,
      1, 0, 0, 0, 1,
      1, 0, 0, 0, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 0 },

    // Nota E
    { 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 0,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1 },

    // Nota F
    { 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 0,
      1, 0, 0, 0, 0,
      1, 0, 0, 0, 0 },

    // Nota G
    { 0, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 0, 1, 1, 1,
      1, 0, 0, 0, 1,
      0, 1, 1, 1, 0 },

    // Nota A
    { 0, 1, 1, 1, 0,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 1,
      1, 0, 0, 0, 1 },

    // Nota B
    { 1, 1, 1, 1, 0,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 0,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 0 }
};

// Função para retornar a cor de um LED aceso
static inline LedConfig ledOn() {
    LedConfig color = {0.3, 0.3, 0.3}; // Branco (RGB: 0.3, 0.3, 0.3)
    return color;
}

// Função para retornar a cor de um LED apagado
static inline LedConfig ledOff() {
    LedConfig color = {0.0, 0.0, 0.0}; // Preto (RGB: 0.0, 0.0, 0.0)
    return color;
}

// Função para converter uma matriz de nota em uma matriz de cores
void convertNoteToLedMatrix(const uint8_t note[25], LedMatrix ledMatrix) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (note[i * 5 + j] == 1) {
                ledMatrix[i][j] = ledOn(); // LED aceso
            } else {
                ledMatrix[i][j] = ledOff(); // LED apagado
            }
        }
    }
}

// Função para obter a matriz de cores de qualquer nota
void getNote(uint8_t noteIndex, LedMatrix ledMatrix) {
    if (noteIndex < 7) { // Verifica se o índice é válido (de 0 a 6)
        convertNoteToLedMatrix(notes[noteIndex], ledMatrix);
    }
}

#endif // NOTES_H