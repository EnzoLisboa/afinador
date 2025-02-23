#ifndef NOTES_H
#define NOTES_H

#include <stdint.h>

// Definição das matrizes para cada nota (5x5)
// Cada matriz é um array de 25 elementos (5x5), onde 1 representa um LED aceso e 0 representa um LED apagado.

// Nota C
const uint8_t note_C[25] = {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 1
};

// Nota D
const uint8_t note_D[25] = {
    1, 1, 1, 1, 0,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 0
};

// Nota E
const uint8_t note_E[25] = {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 0,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 1
};

// Nota F
const uint8_t note_F[25] = {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 0,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 0
};

// Nota G
const uint8_t note_G[25] = {
    0, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    1, 0, 1, 1, 1,
    1, 0, 0, 0, 1,
    0, 1, 1, 1, 0
};

// Nota A
const uint8_t note_A[25] = {
    0, 1, 1, 1, 0,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1
};

// Nota B
const uint8_t note_B[25] = {
    1, 1, 1, 1, 0,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 0,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 0
};

#endif // NOTES_H