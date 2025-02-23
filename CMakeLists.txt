# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Importa o Pico SDK
include(pico_sdk_import.cmake)

# Define o projeto
project(afinador C CXX ASM)

# Inicializa o SDK
pico_sdk_init()

# Adiciona a pasta 'inc' ao caminho de inclusão
include_directories(inc)

# Adiciona os arquivos das bibliotecas SSD1306 e Neopixel
file(GLOB LIBRARY_SOURCES "inc/ssd1306.c" "inc/neopixel.c")

# Adiciona o executável
add_executable(afinador main.c ${LIBRARY_SOURCES})

# Gera o cabeçalho PIO para controle dos LEDs Neopixel
pico_generate_pio_header(afinador ${CMAKE_CURRENT_SOURCE_DIR}/inc/ws2812.pio)

# Adiciona o código PIO ao projeto
pico_add_extra_outputs(afinador)

# Vincula o Pico SDK e as bibliotecas necessárias ao projeto
target_link_libraries(afinador pico_stdlib hardware_pio hardware_i2c hardware_pwm hardware_adc)

# Habilita a saída USB (opcional)
pico_enable_stdio_usb(afinador 1)