# Painel de Controle Interativo com Acesso Concorrente

SecurePass: acesso inteligente, controle garantido - Um projeto baseado no **RP2040 (BitDogLab)** com **FreeRTOS**, que simula o controle de entrada e saída de usuários em um espaço físico, utilizando botões, LED RGB, buzzer e display OLED.

---

## Descrição

O sistema possui três tarefas principais:
- **Entrada**: acionada pelo botão A (GPIO 5).
- **Saída**: acionada pelo botão B (GPIO 6).
- **Reset**: acionada pelo botão do joystick (GPIO 22).

As tarefas controlam o número de usuários ativos, com feedback via display OLED, LED RGB e buzzer.

---

## Funcionalidades

- Contagem de usuários com limite configurável (`MAX`).
- Indicação de status por LED RGB:
  - Azul: vazio
  - Verde: ocupação moderada
  - Amarelo: 1 vaga restante
  - Vermelho: capacidade máxima
- Beep sonoro curto (entrada negada) e duplo (reset).
- Display com mensagens informativas.
- Uso de FreeRTOS com semáforos e mutex.

---

## Componentes Utilizados

- RP2040 (BitDogLab)
- Display OLED (SSD1306 via I2C)
- Buzzer (GPIO 21)
- LED RGB (GPIOs 11, 12, 13)
- Botões físicos (GPIOs 5, 6, 22)
- FreeRTOS

---

## Como Usar
1. Pré-requisitos
2. Clonar o repositório
3. Compilar o projeto
4. Gravar o arquivo .uf2 na placa
5. Operar o sistema na prática
6. Feedback do sistema

---

## Autor
### Matheus Nepomuceno Souza
