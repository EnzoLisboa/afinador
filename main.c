#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/neopixel.h"
#include "inc/notes.h"
#include <stdio.h>
#include <string.h>


// Definições
#define DEBOUNCE_TIME_MS 50  // Tempo mínimo entre leituras de botão
#define NEOPIXEL_PIN 7       
#define NUM_LEDS 25         
#define BUTTON_A_PIN 5      
#define BUTTON_B_PIN 6      
#define JOYSTICK_Y_PIN 26   
#define BUZZER_PIN 21       
#define MIC_PIN 28          
#define LED_RED_PIN 13      
#define LED_GREEN_PIN 11    
#define LED_BLUE_PIN 12     
#define I2C_PORT i2c1       
#define I2C_SDA 14          
#define I2C_SCL 15          
#define OLED_ADDRESS 0x3C   

// Estados do sistema
typedef enum {
    MODE_SELECTION,
    TUNER_MODE,
    DIAPASON_MODE,
    DIAPASON_PLAYING
} SystemState;

SystemState current_state = MODE_SELECTION;
uint8_t selected_note_index = 0;
absolute_time_t last_press_time_A = {0};
absolute_time_t last_press_time_B = {0};

// Função de debounce para os botões
bool is_button_pressed(uint pin, absolute_time_t *last_press_time) {
    if (gpio_get(pin) == 0) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(*last_press_time, now) / 1000 >= DEBOUNCE_TIME_MS) {
            *last_press_time = now;
            return true;
        }
    }
    return false;
}

// Função para inicializar os componentes
void init_components() {
    neopixel_init(NEOPIXEL_PIN, NUM_LEDS);
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);

    adc_init();
    adc_gpio_init(JOYSTICK_Y_PIN);
    adc_select_input(0);

    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, true);

    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// Função para exibir uma nota na matriz de LEDs
void display_note(const uint8_t note[25], uint8_t r, uint8_t g, uint8_t b) {
    neopixel_clear();
    for (int i = 0; i < NUM_LEDS; i++) {
        if (note[i] == 1) {
            neopixel_set_rgb(i, r, g, b);
        }
    }
    neopixel_show();
}

// Função para tocar uma nota no buzzer
void play_note(float frequency, uint32_t duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint wrap = 125000000 / (uint32_t)frequency;  // 125 MHz / frequência
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, wrap / 2);  // Ativa o sinal PWM
    sleep_ms(duration_ms);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);  // Desliga o PWM
}

// Função para capturar a frequência do microfone (simplificado)
float capture_frequency() {
    return 440.0;  // Frequência do Lá (440 Hz)
}

// Função para determinar a nota com base na frequência
const char *get_note_from_frequency(float frequency) {
    return "A";  // Simula a detecção da nota "A"
}

// Função para atualizar o LED RGB com base na diferença de frequência
void update_rgb_led(float target_frequency, float captured_frequency) {
    float difference = captured_frequency - target_frequency;
    if (difference > 10) {
        gpio_put(LED_RED_PIN, 0);
        gpio_put(LED_GREEN_PIN, 0);
        gpio_put(LED_BLUE_PIN, 1);  // Mais agudo (azul)
    } else if (difference < -10) {
        gpio_put(LED_RED_PIN, 1);
        gpio_put(LED_GREEN_PIN, 0);
        gpio_put(LED_BLUE_PIN, 0);  // Mais grave (vermelho)
    } else {
        gpio_put(LED_RED_PIN, 0);
        gpio_put(LED_GREEN_PIN, 1);
        gpio_put(LED_BLUE_PIN, 0);  // Afinado (verde)
    }
}

// Função para exibir as descrições dos botões no display OLED
void display_button_descriptions(ssd1306_t *ssd, const char *line1, const char *line2) {
    ssd1306_draw_string(ssd, line1, 0, 48);
    ssd1306_draw_string(ssd, line2, 0, 56);
    ssd1306_show(ssd);
}

// Função principal
int main() {
    stdio_init_all();
    printf("Inicializando...\n");
    init_components();
    printf("Componentes inicializados.\n");

    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, OLED_ADDRESS, I2C_PORT);

    while (true) {
        switch (current_state) {
            case MODE_SELECTION:
                ssd1306_clear(&ssd);
                ssd1306_draw_string(&ssd, "1: Afinador", 0, 0);
                ssd1306_draw_string(&ssd, "2: Diapasao", 0, 16);
                display_button_descriptions(&ssd, "A - Selecionar", "B - Voltar");
                ssd1306_show(&ssd);

                uint16_t joystick_y = adc_read();
                if (joystick_y < 1000) {
                    selected_note_index = 0;  // Seleciona Afinador
                } else if (joystick_y > 3000) {
                    selected_note_index = 1;  // Seleciona Diapasão
                }

                if (is_button_pressed(BUTTON_A_PIN, &last_press_time_A)) {
                    current_state = (selected_note_index == 0) ? TUNER_MODE : DIAPASON_MODE;
                    ssd1306_clear(&ssd);
                    if (current_state == TUNER_MODE) {
                        ssd1306_draw_string(&ssd, "Modo Afinador", 0, 0);
                        display_button_descriptions(&ssd, "B - Voltar", "");
                    } else {
                        ssd1306_draw_string(&ssd, "Modo Diapasao", 0, 0);
                        display_button_descriptions(&ssd, "A - Emitir", "B - Voltar");
                        display_note(note_A, 255, 255, 0);  // Exibe a nota Lá na matriz de LEDs
                    }
                    ssd1306_show(&ssd);
                }
                break;

            case TUNER_MODE:
                float frequency = capture_frequency();
                const char *note = get_note_from_frequency(frequency);
                update_rgb_led(440.0, frequency);  // Comparação com o Lá (440 Hz)

                if (strcmp(note, "A") == 0) display_note(note_A, 255, 255, 0);
                // Adicionar mais notas conforme necessário

                if (is_button_pressed(BUTTON_B_PIN, &last_press_time_B)) {
                    current_state = MODE_SELECTION;
                }
                break;

            case DIAPASON_MODE:
                display_note(note_A, 255, 255, 0);  // Exibe o padrão da nota Lá

                if (is_button_pressed(BUTTON_A_PIN, &last_press_time_A)) {
                    current_state = DIAPASON_PLAYING;
                    ssd1306_clear(&ssd);
                    ssd1306_draw_string(&ssd, "Emitindo...", 0, 0);
                    display_button_descriptions(&ssd, "B - Parar", "");
                    ssd1306_show(&ssd);
                    play_note(440.0, 1000);  // Toca o Lá
                    current_state = DIAPASON_MODE;
                }

                if (is_button_pressed(BUTTON_B_PIN, &last_press_time_B)) {
                    current_state = MODE_SELECTION;
                }
                break;

            case DIAPASON_PLAYING:
                if (is_button_pressed(BUTTON_B_PIN, &last_press_time_B)) {
                    current_state = DIAPASON_MODE;
                    ssd1306_clear(&ssd);
                    ssd1306_draw_string(&ssd, "Modo Diapasao", 0, 0);
                    display_button_descriptions(&ssd, "A - Emitir", "B - Voltar");
                    ssd1306_show(&ssd);
                }
                break;
        }
        sleep_ms(100);  // Delay para evitar leituras rápidas demais
    }

    return 0;
}
