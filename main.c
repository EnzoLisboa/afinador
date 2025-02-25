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

PIO pio = pio0;  // Use pio0 ou pio1, dependendo do seu setup
uint sm = 0;      // Variável para a máquina de estados

// Definições
#define DEBOUNCE_TIME_MS 250

#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define JOYSTICK_BUTTON_PIN 22
#define BUZZER_PIN 10
#define LED_RED_PIN 13
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 12
#define MIC_PIN 28  // Pino do microfone (GPIO28, ADC2)

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDRESS 0x3C

// Estados do sistema
typedef enum {
    MODE_SELECTION,
    TUNER_MODE,
    DIAPASON_MODE
} SystemState;

SystemState current_state = MODE_SELECTION;
uint8_t selected_note_index = false;
absolute_time_t last_press_time_A = {0};
absolute_time_t last_press_time_B = {0};
absolute_time_t last_press_time_JOY = {0};

// Variáveis para o afinador
#define SAMPLE_RATE 8000        // Taxa de amostragem (8 kHz)
#define BUFFER_SIZE 256         // Tamanho do buffer para armazenar amostras
#define FREQ_TOLERANCE 5.0      // Tolerância para considerar a nota afinada (em Hz)
#define VOLUME_THRESHOLD 125    // Limiar de volume (ajuste conforme necessário)
#define SMOOTHING_FACTOR 0.1    // Fator de suavização (0.1 = 10% de influência da nova leitura)
#define CALIBRATION_FACTOR 1.965  // Fator de calibração (ajuste conforme necessário)
float detected_freq = 0.0;      // Frequência detectada
bool is_tuned = false;          // Indica se a nota está afinada

// Nomes das notas (apenas notas naturais)
const char *note_names[] = {"C", "D", "E", "F", "G", "A", "B"};

// Número de semitons em relação a A4 para cada nota natural
int8_t semitones_from_A4[] = {-9, -7, -5, -4, -2, 0, 2}; // C, D, E, F, G, A, B

// Função para calcular a frequência de uma nota com base no número de semitons em relação a A4
float calculate_note_frequency(int8_t n) {
    return pow(2.0, n / 12.0) * 440.0;
}

// Função para desligar todos os LEDs RGB
void clear_leds() {
    gpio_put(LED_RED_PIN, false);
    gpio_put(LED_GREEN_PIN, false);
    gpio_put(LED_BLUE_PIN, false);
}

// Função de callback única para todos os botões
void button_callback(uint gpio, uint32_t events) {
    absolute_time_t now = get_absolute_time();

    // Verifica qual botão foi pressionado
    switch (gpio) {
        case BUTTON_A_PIN:
            if (absolute_time_diff_us(last_press_time_A, now) / 1000 >= DEBOUNCE_TIME_MS) {
                last_press_time_A = now;

                if (current_state == MODE_SELECTION) {
                    if (selected_note_index == false) {
                        current_state = TUNER_MODE;
                    } else {
                        current_state = DIAPASON_MODE;
                    }
                }
            }
            break;

        case BUTTON_B_PIN:
            if (absolute_time_diff_us(last_press_time_B, now) / 1000 >= DEBOUNCE_TIME_MS) {
                last_press_time_B = now;
                current_state = MODE_SELECTION;
            }
            break;

        case JOYSTICK_BUTTON_PIN:
            if (absolute_time_diff_us(last_press_time_JOY, now) / 1000 >= DEBOUNCE_TIME_MS) {
                last_press_time_JOY = now;

                if (current_state == MODE_SELECTION) {
                    selected_note_index = (selected_note_index == false) ? 1 : 0;
                }
            }
            break;
    }
}

// Inicializa o PWM no pino do buzzer
void init_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);  // Define o pino do buzzer como PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice_num, 8.0);  // Reduz o clock (125MHz / 8 = 15.625MHz)
}

// Toca a nota A (440Hz)
void play_diapason() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);

    uint32_t sys_clock = 125000000;  // Clock do sistema: 125MHz
    float clkdiv = 100.0;            // Reduz o divisor para evitar arredondamentos
    uint16_t wrap_value = (sys_clock / clkdiv) / 430;  // 440Hz

    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap_value);

    // Reduz volume para 0,8% do duty cycle
    pwm_set_chan_level(slice_num, channel, wrap_value / 125);  

    pwm_set_enabled(slice_num, true);
}

// Para o som do buzzer
void stop_diapason() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);
}

// Função para calcular a frequência usando o método Zero-Crossing
float calculate_frequency(uint16_t *buffer, uint32_t buffer_size, uint32_t sample_rate) {
    uint32_t zero_crossings = 0;
    for (uint32_t i = 1; i < buffer_size; i++) {
        if ((buffer[i - 1] < 2048 && buffer[i] >= 2048)) { // Detecta cruzamentos por zero
            zero_crossings++;
        }
    }
    float frequency = (zero_crossings * sample_rate) / (2.0 * buffer_size); // Fórmula para calcular a frequência
    return frequency * CALIBRATION_FACTOR;  // Aplica o fator de calibração
}

// Função para suavizar a frequência detectada
float smooth_frequency(float new_freq, float old_freq, float smoothing_factor) {
    return (new_freq * smoothing_factor) + (old_freq * (1.0 - smoothing_factor));
}

// Função para calcular a amplitude do sinal
uint16_t calculate_amplitude(uint16_t *buffer, uint32_t buffer_size) {
    uint16_t min = 4095, max = 0;
    for (uint32_t i = 0; i < buffer_size; i++) {
        if (buffer[i] < min) min = buffer[i];
        if (buffer[i] > max) max = buffer[i];
    }
    return max - min; // Amplitude = diferença entre o valor máximo e mínimo
}

// Função para determinar a nota mais próxima
uint8_t get_closest_note(float frequency) {
    uint8_t closest_note = 0;
    float min_diff = 1000.0; // Valor inicial alto para comparação

    for (uint8_t i = 0; i < 7; i++) {
        // Calcula a frequência da nota atual
        float note_freq = calculate_note_frequency(semitones_from_A4[i]);

        // Calcula a diferença entre a frequência detectada e a frequência da nota
        float diff = fabs(frequency - note_freq);

        // Verifica se esta é a nota mais próxima até agora
        if (diff < min_diff) {
            min_diff = diff;
            closest_note = i;
        }
    }

    return closest_note;
}

// Função para controlar os LEDs RGB conforme o estado de afinação
void update_leds(float detected_freq, uint8_t note_index, bool is_tuned) {
    // Calcula a frequência da nota mais próxima
    float target_freq = calculate_note_frequency(semitones_from_A4[note_index]);

    if (is_tuned) {
        // Afinado: Verde
        gpio_put(LED_RED_PIN, false);
        gpio_put(LED_GREEN_PIN, true);
        gpio_put(LED_BLUE_PIN, false);
    } else if (detected_freq < target_freq - FREQ_TOLERANCE) {
        // Grave: Amarelo (Vermelho + Verde)
        gpio_put(LED_RED_PIN, true);
        gpio_put(LED_GREEN_PIN, true);
        gpio_put(LED_BLUE_PIN, false);
    } else if (detected_freq > target_freq + FREQ_TOLERANCE) {
        // Agudo: Vermelho
        gpio_put(LED_RED_PIN, true);
        gpio_put(LED_GREEN_PIN, false);
        gpio_put(LED_BLUE_PIN, false);
    } else {
        // Desligar LEDs se não estiver em nenhum dos estados acima
        gpio_put(LED_RED_PIN, false);
        gpio_put(LED_GREEN_PIN, false);
        gpio_put(LED_BLUE_PIN, false);
    }
}

// Função para inicializar os componentes
void init_components() {
    // Configurar botões como entradas com pull-up
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);

    // Configurar interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // Configurar LEDs RGB
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);

    // Configurar I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o PIO e a máquina de estado
    sm = ws2812Init(pio0);

    // Inicializa o ADC para o microfone (GPIO28, ADC2)
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(2);  // Usa o canal ADC2 (GPIO28)
}

// Função principal
int main() {
    stdio_init_all();
    init_components();
    init_buzzer();  // Inicializa o PWM para o buzzer

    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, OLED_ADDRESS, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display

    LedMatrix ledMatrix;  // Matriz de LEDs para armazenar a nota A
    clearLedMatrix(ledMatrix);  // Limpa a matriz de LEDs

    while (true) {
        switch (current_state) {
            case MODE_SELECTION:
                clear_leds();
                clearLedMatrix(ledMatrix);
                displayPattern(ledMatrix, pio0, sm);  // Aplica o padrão limpo
                stop_diapason(); // Para o buzzer ao sair do modo Diapasão
                ssd1306_fill(&ssd, false); // Limpa o display
                ssd1306_draw_string(&ssd, "1: Afinador", 4, 4);
                ssd1306_draw_string(&ssd, "2: Diapasao", 4, 20);
                if (selected_note_index == false) {
                    ssd1306_rect(&ssd, 0, 0, 128, 16, true, false);
                    ssd1306_send_data(&ssd);
                } else {
                    ssd1306_rect(&ssd, 16, 0, 128, 16, true, false);
                    ssd1306_send_data(&ssd);
                }
                break;

            case TUNER_MODE: {
                stop_diapason(); // Para o buzzer ao sair do modo Diapasão
                ssd1306_fill(&ssd, false);

                // Captura amostras do microfone
                uint16_t buffer[BUFFER_SIZE];
                for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
                    buffer[i] = adc_read();
                    sleep_us(1000000 / SAMPLE_RATE); // Mantém a taxa de amostragem
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
                    getNote(note_index, ledMatrix);
                    displayPattern(ledMatrix, pio0, sm);

                    // Verifica se a nota está afinada
                    float target_freq = calculate_note_frequency(semitones_from_A4[note_index]);
                    if (detected_freq >= target_freq - FREQ_TOLERANCE && detected_freq <= target_freq + FREQ_TOLERANCE) {
                        is_tuned = true;
                    } else {
                        is_tuned = false;
                    }

                    // Atualiza os LEDs RGB conforme o estado de afinação
                    update_leds(detected_freq, note_index, is_tuned);

                    // Exibe a frequência e a nota no display OLED
                    char freq_str[20];
                    snprintf(freq_str, sizeof(freq_str), "%.1f Hz", detected_freq); // Formata a frequência
                    ssd1306_draw_string(&ssd, "Modo Afinador", 16, 4); // Exibe "Modo Afinador" na linha 1
                    ssd1306_draw_string(&ssd, freq_str, 32, 20); // Exibe a frequência na linha 2
                    
                } else {
                    // Volume abaixo do limiar: ignora o sinal
                    clear_leds(); // Desliga os LEDs
                    clearLedMatrix(ledMatrix);
                    displayPattern(ledMatrix, pio0, sm);  // Aplica o padrão limpo
                    ssd1306_draw_string(&ssd, "Modo Afinador", 16, 4); // Exibe "Modo Afinador" na linha 1
                    ssd1306_draw_string(&ssd, "Toque a nota", 17, 20);
                }

                ssd1306_send_data(&ssd);
                break;
            }

            case DIAPASON_MODE:
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, "Modo Diapasao", 18, 4);
                ssd1306_draw_string(&ssd, "440Hz", 49, 20);
                ssd1306_send_data(&ssd);
                play_diapason(); // Inicia o som de 440Hz
                getNote(5, ledMatrix); // Exibe a nota A na matriz de LEDs
                displayPattern(ledMatrix, pio0, sm);
                break;
        }
    }
    return 0;
}