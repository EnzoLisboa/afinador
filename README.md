# Afinador e Diapasão

Este projeto implementa um afinador e diapasão utilizando um microcontrolador, uma tela OLED, uma matriz de LEDs 5x5 e um LED RGB. Ele foi desenvolvido para ajudar músicos a afinar seus instrumentos de forma prática e visual.

---

## Funcionalidades
- **Modo Afinador**: O microcontrolador escuta a nota tocada e exibe a afinação na matriz de LEDs.
  - O LED RGB indica:
    - **Verde**: Nota afinada.
    - **Amarelo**: Nota está grave.
    - **Vermelho**: Nota está aguda.
  - A frequência detectada é exibida na tela OLED.
- **Modo Diapasão**: 
  - Emite um som de referência em 440Hz (nota Lá).
  - Exibe a nota correspondente na matriz de LEDs.
  - A frequência de referência é mostrada na tela OLED.
- **Interface com Tela OLED**: Mostra as opções e estados do sistema.

---

## Controles
- **Botão A**: Seleciona a opção atual.
- **Botão B**: Retorna ao menu anterior.
- **Botão do Joystick**: Alterna entre as opções.

---

## Componentes Utilizados
- **Microcontrolador:** Raspberry Pi Pico W.
- **Tela OLED:** Display OLED 128x64 com interface I2C.
- **Matriz de LEDs:** Matriz 5x5 de LEDs WS2812.
- LED RGB
- **Buzzer:** Buzzer passivo para gerar som.
- **Microfone:** Sensor de áudio para detectar som.
- **Botões:** Para interação com o usuário

---

## Como Funciona

1. **Menu Principal**:
   - Use o **botão do joystick** para alternar entre "Afinador" e "Diapasão".
   - Pressione **Botão A** para selecionar o modo desejado.

2. **Modo Afinador**:
   - Toque uma nota musical no instrumento.
   - O sistema detecta a frequência e indica se a nota está afinada, grave ou aguda.
   - A nota detectada é exibida na matriz de LEDs, e a frequência é mostrada na tela OLED.

3. **Modo Diapasão**:
   - O buzzer emite a nota **Lá 440Hz** como referência.
   - A nota correspondente é exibida na matriz de LEDs.
   - A frequência de referência é mostrada na tela OLED.

4. **Retorno ao Menu**:
   - Pressione **Botão B** para voltar ao menu principal.

---

## Estrutura do Código

O código está organizado da seguinte forma:

- **`main.c`**:
  - Contém a lógica principal do sistema, incluindo a máquina de estados e a detecção de frequência.

- **`ws2812.c/h`**:
  - Controla a matriz de LEDs WS2812, exibindo padrões e notas.

- **`ssd1306.c/h`**:
  - Gerencia a comunicação com a tela OLED, exibindo textos e informações.

- **`notes.c/h`**:
  - Contém funções para manipulação de notas musicais, como cálculo de frequências e identificação de notas.

---

## Esquema de Ligação

| Componente        | Pino no Raspberry Pi Pico |
|-------------------|---------------------------|
| Tela OLED (SDA)   | GPIO14                    |
| Tela OLED (SCL)   | GPIO15                    |
| Matriz de LEDs    | GPIO16                    |
| LED RGB (Vermelho)| GPIO13                    |
| LED RGB (Verde)   | GPIO11                    |
| LED RGB (Azul)    | GPIO12                    |
| Buzzer            | GPIO10                    |
| Microfone         | GPIO28 (ADC2)             |
| Botão A           | GPIO5                     |
| Botão B           | GPIO6                     |
| Botão do Joystick | GPIO22                    |

---

## Compilação e Upload

1. **Configuração do Ambiente**:
   - Instale o [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) no seu sistema.
   - Configure o ambiente de desenvolvimento (ex.: VS Code com extensão para Pico).

2. **Compilação**:
   - Clone este repositório.
   - Navegue até a pasta do projeto e compile o código:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```

3. **Upload**:
   - Conecte o Raspberry Pi Pico ao computador no modo de bootloader (segure o botão BOOTSEL ao conectar o USB).
   - Copie o arquivo `.uf2` gerado para o dispositivo:
     ```bash
     cp afinador_diapasao.uf2 /media/SEU_USUARIO/RPI-RP2/
     ```

4. **Execução**:
   - Conecte os componentes conforme o esquema de ligação.
   - O sistema iniciará automaticamente no menu principal.