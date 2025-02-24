# Afinador e Diapasão

Este projeto implementa um afinador e diapasão utilizando um microcontrolador, uma tela OLED, uma matriz de LEDs 5x5 e um LED RGB.

## Funcionalidades
- **Modo Afinador**: O microcontrolador escuta a nota tocada e exibe a afinação na matriz de LEDs.
  - O LED RGB indica:
    - **Verde**: Nota afinada
    - **Amarelo**: Nota está grave
    - **Vermelho**: Nota está aguda
- **Modo Diapasão**: Emite um som de referência em 440Hz e exibe a nota correspondente na matriz de LEDs.
- **Interface com Tela OLED**: Mostra as opções e estados do sistema.

## Controles
- **Botão A**: Seleciona a opção
- **Botão B**: Retorna ao menu anterior
- **Botão do Joystick**: Alterna entre as opções

## Componentes Utilizados
- Microcontrolador (ex.: Raspberry Pi Pico W)
- Tela OLED 128x64
- Matriz de LEDs 5x5
- LED RGB
- Buzzer
- Microfone

## Como Funciona
1. No menu principal, use o **botão do joystick** para alternar entre "Afinador" e "Diapasão".
2. Pressione **Botão A** para selecionar o modo desejado.
3. No **Modo Afinador**, toque uma nota musical, e o sistema indicará se está afinada ou não.
4. No **Modo Diapasão**, o buzzer emitirá a nota **Lá 440Hz**.
5. Pressione **Botão B** para voltar ao menu principal.

## Estrutura do Código
O código principal está dividido em:
- `main.c`: Lógica principal do sistema.
- `ws2812.c/h`: Controle da matriz de LEDs.
- `ssd1306.c/h`: Controle da tela OLED.
- `notes.c/h`: Manipulação das notas musicais.

## Compilação e Upload
1. Compile o código usando um ambiente compatível (ex.: Raspberry Pi Pico SDK).
2. Carregue o firmware no microcontrolador.
3. Conecte os componentes conforme o esquema de ligação.
4. Execute o sistema e utilize a interface para interagir com as funções do afinador e diapasão.