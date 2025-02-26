// Inclui as bibliotecas necessárias
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "inc/ssd1306.h"
#include "inc/ws2812.h"
#include "inc/notes.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Definições de hardware e constantes
#define DEBOUNCE_TIME_MS 250  // Tempo de debounce para os botões
#define BUTTON_A_PIN 5        // Pino do botão A
#define BUTTON_B_PIN 6        // Pino do botão B
#define JOYSTICK_BUTTON_PIN 22 // Pino do botão do joystick
#define BUZZER_PIN 10         // Pino do buzzer
#define LED_RED_PIN 13        // Pino do LED vermelho
#define LED_GREEN_PIN 11      // Pino do LED verde
#define LED_BLUE_PIN 12       // Pino do LED azul
#define MIC_PIN 28            // Pino do microfone
#define I2C_PORT i2c1         // Porta I2C para o display OLED
#define I2C_SDA 14            // Pino SDA do I2C
#define I2C_SCL 15            // Pino SCL do I2C
#define OLED_ADDRESS 0x3C     // Endereço do display OLED

PIO pio = pio0;  // Use pio0 ou pio1, dependendo do seu setup
uint sm = 0;     // Variável para a máquina de estados
absolute_time_t last_press_time_A = {0};     // Último tempo de pressionamento do botão A
absolute_time_t last_press_time_B = {0};     // Último tempo de pressionamento do botão B
absolute_time_t last_press_time_JOY = {0};   // Último tempo de pressionamento do botão do joystick
const char *note_names[] = {"C", "D", "E", "F", "G", "A", "B"}; // Nomes das notas (apenas notas naturais)
int8_t semitones_from_A4[] = {-9, -7, -5, -4, -2, 0, 2}; // Número de semitons em relação a A para cada nota natural (C, D, E, F, G, A, B)
uint8_t selected_note_index = false;         // Índice da nota selecionada

// Variáveis para o afinador
#define SAMPLE_RATE 4000        // Taxa de amostragem (4 kHz)
#define BUFFER_SIZE 512         // Tamanho do buffer para armazenar amostras
#define FREQ_TOLERANCE 6        // Tolerância para considerar a nota afinada (em Hz)
#define VOLUME_THRESHOLD 150    // Limiar de volume para detecção de som
#define SMOOTHING_FACTOR 0.1    // Fator de suavização para a frequência detectada
#define CALIBRATION_FACTOR 1    // Fator de calibração para a frequência
float detected_freq = 0.0;      // Frequência detectada pelo microfone

// Estados do sistema
typedef enum {
    MODE_SELECTION,  // Modo de seleção de função
    TUNER_MODE,      // Modo afinador
    DIAPASON_MODE    // Modo diapasão
} SystemState;
SystemState current_state = MODE_SELECTION;  // Estado atual do sistema

// Função de callback para os botões
void button_callback(uint gpio, uint32_t events) {
    absolute_time_t now = get_absolute_time();

    // Verifica qual botão foi pressionado
    switch (gpio) {
        case BUTTON_A_PIN:
            if (absolute_time_diff_us(last_press_time_A, now) / 1000 >= DEBOUNCE_TIME_MS) {
                last_press_time_A = now;

                if (current_state == MODE_SELECTION) {
                    if (selected_note_index == false) {
                        current_state = TUNER_MODE;  // Muda para o modo afinador
                    } else {
                        current_state = DIAPASON_MODE;  // Muda para o modo diapasão
                    }
                }
            }
            break;

        case BUTTON_B_PIN:
            if (absolute_time_diff_us(last_press_time_B, now) / 1000 >= DEBOUNCE_TIME_MS) {
                last_press_time_B = now;
                current_state = MODE_SELECTION;  // Volta para o modo de seleção
            }
            break;

        case JOYSTICK_BUTTON_PIN:
            if (absolute_time_diff_us(last_press_time_JOY, now) / 1000 >= DEBOUNCE_TIME_MS) {
                last_press_time_JOY = now;

                if (current_state == MODE_SELECTION) {
                    selected_note_index = (selected_note_index == false) ? 1 : 0;  // Alterna entre opções
                }
            }
            break;
    }
}

// Toca a nota A (440Hz)
void play_diapason() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);

    uint32_t sys_clock = 125000000;  // Clock do sistema: 125MHz
    float clkdiv = 100.0;            // Divisor de clock para gerar 440Hz
    uint16_t wrap_value = (sys_clock / clkdiv) / 430;  // Calcula o valor de wrap para 440Hz

    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap_value);

    // Define o duty cycle para 0,8% (volume baixo)
    pwm_set_chan_level(slice_num, channel, wrap_value / 125);  

    pwm_set_enabled(slice_num, true);  // Habilita o PWM
}

// Para o som do buzzer
void stop_diapason() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);  // Desabilita o PWM
}

// Função para calcular a frequência de uma nota em relação a A4
float calculate_note_frequency(int8_t n) {
    return pow(2.0, n / 12.0) * 440.0;  // Fórmula para calcular a frequência da nota
}

// Função para calcular a frequência do sinal capturado
float calculate_frequency(uint16_t *buffer, uint32_t buffer_size, uint32_t sample_rate) {
    uint32_t zero_crossings = 0;

    // Conta os cruzamentos por zero no sinal
    for (uint32_t i = 1; i < buffer_size; i++) {
        if ((buffer[i - 1] < 2048 && buffer[i] >= 2048)) {  // Detecta cruzamento por zero (subindo)
            zero_crossings++;
        }
    }

    if (zero_crossings < 2) return 0.0;  // Se houver poucos cruzamentos, não há som detectado

    float frequency = (zero_crossings * sample_rate) / (buffer_size);  // Calcula a frequência
    return frequency * CALIBRATION_FACTOR;  // Aplica o fator de calibração
}

// Função para suavizar a frequência detectada
float smooth_frequency(float new_freq, float old_freq, float smoothing_factor) {
    if (new_freq == 0.0) return old_freq;  // Mantém a última frequência válida se não houver sinal
    return (smoothing_factor * new_freq) + ((1.0 - smoothing_factor) * old_freq);  // Suaviza a frequência
}

// Função para calcular a amplitude do sinal
uint16_t calculate_amplitude(uint16_t *buffer, uint32_t buffer_size) {
    uint16_t min = 4095, max = 0;

    // Encontra os valores mínimo e máximo no buffer
    for (uint32_t i = 0; i < buffer_size; i++) {
        if (buffer[i] < min) min = buffer[i];
        if (buffer[i] > max) max = buffer[i];
    }
    return max - min;  // Retorna a amplitude (diferença entre máximo e mínimo)
}

// Função para determinar a nota mais próxima da frequência detectada
uint8_t get_closest_note(float frequency) {
    uint8_t closest_note = 0;
    float min_diff = 1000.0;
    float new_frequency = frequency;

    // Normaliza a frequência para a oitava correta
    if (frequency > 500) { new_frequency /= 2; }
    if (frequency < 250 && frequency > 125) { new_frequency *= 2; }
    if (frequency < 125) { new_frequency *= 4; }

    // Encontra a nota mais próxima
    for (uint8_t i = 0; i < 7; i++) {
        float base_freq = calculate_note_frequency(semitones_from_A4[i]);
        float diff = fabs(new_frequency - base_freq);

        if (diff < min_diff) {
            min_diff = diff;
            closest_note = i;
        }
    }

    return closest_note;
}

// Função para desligar todos os LEDs RGB
void clear_leds() {
    gpio_put(LED_RED_PIN, false);
    gpio_put(LED_GREEN_PIN, false);
    gpio_put(LED_BLUE_PIN, false);
}

// Função para controlar os LEDs RGB conforme o estado de afinação
void update_leds(float detected_freq, uint8_t note_index) {
    float target_freq = calculate_note_frequency(semitones_from_A4[note_index]);
    float new_target_freq = target_freq;

    // Ajusta a frequência alvo para a oitava correta
    if (detected_freq < 125) { new_target_freq /= 4; }
    if (detected_freq < 250 && detected_freq > 125) { new_target_freq /= 2; }
    if (detected_freq > 500) { new_target_freq *= 2; }

    // Controla os LEDs conforme a diferença entre a frequência detectada e a alvo
    if (detected_freq < new_target_freq + FREQ_TOLERANCE && detected_freq > new_target_freq - FREQ_TOLERANCE) {
        // Afinado: Verde
        gpio_put(LED_RED_PIN, false);
        gpio_put(LED_GREEN_PIN, true);
        gpio_put(LED_BLUE_PIN, false);
    } else if (detected_freq < new_target_freq - FREQ_TOLERANCE) {
        // Grave: Amarelo (Vermelho + Verde)
        gpio_put(LED_RED_PIN, true);
        gpio_put(LED_GREEN_PIN, true);
        gpio_put(LED_BLUE_PIN, false);
    } else if (detected_freq > new_target_freq + FREQ_TOLERANCE) {
        // Agudo: Vermelho
        gpio_put(LED_RED_PIN, true);
        gpio_put(LED_GREEN_PIN, false);
        gpio_put(LED_BLUE_PIN, false);
    } else {
        // Desliga os LEDs se não estiver em nenhum dos estados acima
        clear_leds();
    }
}

// Função para inicializar os componentes
void init_components() {
    // Configura os botões como entradas com pull-up
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // Configura os LEDs RGB como saídas
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);

    // Configura o I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o PIO e a máquina de estado para os LEDs WS2812
    sm = ws2812Init(pio0);

    // Inicializa o ADC para o microfone
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(2);  // Usa o canal ADC2 (GPIO28)

    // Inicializa o PWM para o buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);  // Configura o pino do buzzer como PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice_num, 8.0);  // Define o divisor de clock para o PWM
}

// Função principal
int main() {
    stdio_init_all();  // Inicializa a comunicação serial
    init_components();  // Inicializa os componentes do hardware

    // Inicializa o display OLED
    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, OLED_ADDRESS, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);  // Limpa o display

    LedMatrix ledMatrix;  // Matriz de LEDs para exibir a nota
    clearLedMatrix(ledMatrix);  // Limpa a matriz de LEDs

    while (true) {
        switch (current_state) {
            case MODE_SELECTION:
                // Modo de seleção de função
                clear_leds(); // Desliga os LEDs RGB
                clearLedMatrix(ledMatrix); // Limpa a matriz de LEDs
                displayPattern(ledMatrix, pio0, sm);  // Aplica o padrão limpo
                stop_diapason();  // Para o buzzer
                ssd1306_fill(&ssd, false);  // Limpa o display
                ssd1306_draw_string(&ssd, "1: Afinador", 4, 4);
                ssd1306_draw_string(&ssd, "2: Diapasao", 4, 20);
                if (selected_note_index == false) {
                    ssd1306_rect(&ssd, 0, 0, 128, 16, true, false);
                } else {
                    ssd1306_rect(&ssd, 16, 0, 128, 16, true, false);
                }
                ssd1306_send_data(&ssd); // Envia os dados para o display
                break;

            case TUNER_MODE: {
                // Modo afinador
                stop_diapason();  // Para o buzzer
                ssd1306_fill(&ssd, false);  // Limpa o display

                // Captura amostras do microfone
                uint16_t buffer[BUFFER_SIZE];
                for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
                    buffer[i] = adc_read();
                    sleep_us(1000000 / SAMPLE_RATE);  // Mantém a taxa de amostragem
                }

                // Calcula a amplitude do sinal
                uint16_t amplitude = calculate_amplitude(buffer, BUFFER_SIZE);

                // Verifica se o volume está acima do limiar
                if (amplitude >= VOLUME_THRESHOLD) {
                    // Calcula a frequência detectada
                    float new_freq = calculate_frequency(buffer, BUFFER_SIZE, SAMPLE_RATE);

                    // Suaviza a frequência detectada
                    detected_freq = smooth_frequency(new_freq, detected_freq, SMOOTHING_FACTOR);

                    // Determina a nota mais próxima
                    uint8_t note_index = get_closest_note(detected_freq);

                    // Exibe a nota na matriz de LEDs
                    printf("Frequência detectada: %.2f Hz\n", detected_freq);
                    getNote(note_index, ledMatrix);
                    displayPattern(ledMatrix, pio0, sm);

                    // Atualiza os LEDs RGB conforme o estado de afinação
                    update_leds(detected_freq, note_index);

                    // Exibe a frequência e a nota no display OLED
                    char freq_str[20];
                    snprintf(freq_str, sizeof(freq_str), "%.1f Hz", detected_freq);
                    ssd1306_draw_string(&ssd, "Modo Afinador", 16, 4);
                    ssd1306_draw_string(&ssd, freq_str, 32, 20);
                } else {
                    // Volume abaixo do limiar: ignora o sinal
                    clear_leds(); // Desliga os LEDs RGB
                    clearLedMatrix(ledMatrix); // Limpa a matriz de LEDs
                    displayPattern(ledMatrix, pio0, sm); // Aplica o padrão limpo
                    ssd1306_draw_string(&ssd, "Modo Afinador", 16, 4);
                    ssd1306_draw_string(&ssd, "Toque a nota", 17, 20);
                }
                ssd1306_send_data(&ssd); // Envia os dados para o display
                break;
            }

            case DIAPASON_MODE:
                // Modo diapasão
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, "Modo Diapasao", 18, 4);
                ssd1306_draw_string(&ssd, "440Hz", 49, 20);
                ssd1306_send_data(&ssd);
                play_diapason();  // Toca a nota A (440Hz)
                getNote(5, ledMatrix);  // Exibe a nota A na matriz de LEDs
                displayPattern(ledMatrix, pio0, sm);
                break;
        }
    }
    return 0;
}