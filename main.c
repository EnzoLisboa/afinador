#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "inc/ssd1306.h"
#include "inc/ws2812.h"
#include "inc/notes.h"
#include <stdio.h>
#include <string.h>

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
#define MIC_PIN 28

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
}

// Função principal
int main() {
    stdio_init_all();
    printf("Inicializando...\n");
    init_components();
    printf("Componentes inicializados.\n");
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

            case TUNER_MODE:
                clearLedMatrix(ledMatrix);
                displayPattern(ledMatrix, pio0, sm);
                stop_diapason(); // Para o buzzer ao sair do modo Diapasão
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, "Modo Afinador", 18, 4);
                ssd1306_send_data(&ssd);
                break;

            case DIAPASON_MODE:
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, "Modo Diapasao", 18, 4);
                ssd1306_draw_string(&ssd, "440Hz", 49, 20);
                ssd1306_send_data(&ssd);
                play_diapason(); // Inicia o som de 440Hz
                getNote(5, ledMatrix);
                displayPattern(ledMatrix, pio0, sm);
                break;
        }
        sleep_ms(100);
    }
    return 0;
}