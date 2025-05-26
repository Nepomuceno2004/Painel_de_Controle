#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "lib/ssd1306.h"
#include "lib/buzzer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

#define BOTAO_A 5           // pino do botão A
#define BOTAO_B 6           // pino do botão B
#define LED_PIN_GREEN 11    // verde
#define LED_PIN_BLUE 12     // azul
#define LED_PIN_RED 13      // vermelho
#define BUZZER_PIN 21       // pino do buzzer
#define JOYSTICK_BTN_PIN 22 // pino do botão do joystick

// semáforos utilizados
SemaphoreHandle_t xContadorSemA; // controla as solicitações de entrada
SemaphoreHandle_t xContadorSemB; // controla as solicitações de saída
SemaphoreHandle_t xDisplayMutex; // controla as solicitaçõe de reset
SemaphoreHandle_t xResetSem;     // controla as solicitações de acesso ao display

ssd1306_t ssd;                // variavel do display
uint16_t usuariosNoLocal = 0; // armazena a quantidade de usuários no local
uint32_t last_time;           // armazena o tempo do último clique nos botões
uint8_t MAX = 8;              // número máixmo de pessoas no espaço

// Função responsável por resetar o sistema
void vTaskReset(void *params)
{
    char buffer[32]; // Buffer para armazenar texto que será exibido no display

    while (true)
    {
        // Espera até o semáforo de reset ser liberado
        if (xSemaphoreTake(xResetSem, portMAX_DELAY) == pdTRUE)
        {
            // Reseta a contagem de usuários presentes
            usuariosNoLocal = 0;

            // Desliga todos os LEDs
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);

            // Liga o LED azul indicando que não há usuários no local
            gpio_put(LED_PIN_BLUE, true);

            // Emite dois beeps com o buzzer para indicar o reset
            buzzer_play(BUZZER_PIN, 2000, 120); 
            vTaskDelay(pdMS_TO_TICKS(100));     
            buzzer_play(BUZZER_PIN, 2500, 120);

            // Tenta obter o mutex para acessar o display com segurança
            if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
            {
                // Limpa o display e exibe mensagem de reset
                ssd1306_fill(&ssd, 0);
                sprintf(buffer, "Usuarios: %d", usuariosNoLocal);
                ssd1306_draw_string(&ssd, "Reset ", 5, 10);
                ssd1306_draw_string(&ssd, "Detectado!", 5, 19);
                ssd1306_draw_string(&ssd, buffer, 5, 44);
                ssd1306_send_data(&ssd);

                // Mensagem de debug
                printf("Tarefa 3 ativa\n");

                vTaskDelay(1000 / portTICK_PERIOD_MS);

                // Exibe mensagem de tela de espera no display
                ssd1306_fill(&ssd, 0);
                ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                ssd1306_send_data(&ssd);

                // Libera o mutex do display
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }
}

// Função responsável por lidar com a entrada de usuários no local
void vTaskEntrada(void *params)
{
    char buffer[32];

    while (true)
    {
        // Espera pelo semáforo de entrada
        if (xSemaphoreTake(xContadorSemA, portMAX_DELAY) == pdTRUE)
        {
            // Desliga todos os LEDs inicialmente
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);

            // Se ainda há vagas no local
            if (usuariosNoLocal < MAX)
            {
                usuariosNoLocal++; // Incrementa o número de usuários presentes

                if (usuariosNoLocal < MAX - 1)
                {
                    // Situação normal: ainda há várias vagas - LED verde
                    gpio_put(LED_PIN_GREEN, true);
                }
                else if (usuariosNoLocal == MAX - 1)
                {
                    // Apenas uma vaga restante - LED amarelo (verde + vermelho)
                    gpio_put(LED_PIN_GREEN, true);
                    gpio_put(LED_PIN_RED, true);
                }
                else if (usuariosNoLocal == MAX)
                {
                    // Capacidade máxima atingida - LED vermelho + buzzer
                    gpio_put(LED_PIN_RED, true);
                    buzzer_play(BUZZER_PIN, 3000, 150);
                }

                // Tenta acessar o display
                if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
                {
                    // Atualiza o display com a mensagem de entrada detectada
                    ssd1306_fill(&ssd, 0); 
                    sprintf(buffer, "Usuarios: %d", usuariosNoLocal);
                    ssd1306_draw_string(&ssd, "Entrada ", 5, 10);
                    ssd1306_draw_string(&ssd, "Detectada!", 5, 19);
                    ssd1306_draw_string(&ssd, buffer, 5, 44);
                    ssd1306_send_data(&ssd);
                    printf("Tarefa 1 ativa\n");

                    vTaskDelay(1000 / portTICK_PERIOD_MS);

                    // Retorna para a tela padrão de espera
                    ssd1306_fill(&ssd, 0);
                    ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                    ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                    ssd1306_send_data(&ssd);

                    // Libera o acesso ao display
                    xSemaphoreGive(xDisplayMutex);
                }
            }
            else // Caso o local já esteja cheio
            {
                // Capacidade máxima - LED vermelho + buzzer
                gpio_put(LED_PIN_RED, true);
                buzzer_play(BUZZER_PIN, 3000, 150);

                if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
                {
                    ssd1306_fill(&ssd, 0);
                    sprintf(buffer, "Usuarios: %d", usuariosNoLocal);
                    ssd1306_draw_string(&ssd, "Espaco ", 5, 10);
                    ssd1306_draw_string(&ssd, "Lotado!", 5, 19);
                    ssd1306_draw_string(&ssd, buffer, 5, 44);
                    ssd1306_send_data(&ssd);
                    printf("Tarefa 1 ativa\n");

                    // Tempo de exibição
                    vTaskDelay(1000 / portTICK_PERIOD_MS);

                    // Tela de espera padrão
                    ssd1306_fill(&ssd, 0);
                    ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                    ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                    ssd1306_send_data(&ssd);

                    xSemaphoreGive(xDisplayMutex);
                }
            }
        }
    }
}

// Função responsável por tratar a saída de usuários do local
void vTaskSaida(void *params)
{
    char buffer[32];

    while (true)
    {
        // Aguarda o semáforo de saída
        if (xSemaphoreTake(xContadorSemB, portMAX_DELAY) == pdTRUE)
        {
            // Desliga todos os LEDs
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);

            if (usuariosNoLocal > 0)
            {
                usuariosNoLocal--; // Decrementa o número de usuários no local

                if (usuariosNoLocal == 0)
                {
                    // Nenhum usuário - LED azul
                    gpio_put(LED_PIN_BLUE, true);
                }
                else if (usuariosNoLocal < MAX - 1)
                {
                    // Ainda há várias vagas - LED verde
                    gpio_put(LED_PIN_GREEN, true);
                }
                else if (usuariosNoLocal == MAX - 1)
                {
                    // Apenas 1 vaga disponível - LED amarelo (verde + vermelho)
                    gpio_put(LED_PIN_GREEN, true);
                    gpio_put(LED_PIN_RED, true);
                }

                // Atualiza o display com a saída detectada
                if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
                {
                    ssd1306_fill(&ssd, 0);
                    sprintf(buffer, "Usuarios: %d", usuariosNoLocal);
                    ssd1306_draw_string(&ssd, "Saida ", 5, 10);
                    ssd1306_draw_string(&ssd, "Detectada!", 5, 19);
                    ssd1306_draw_string(&ssd, buffer, 5, 44);
                    ssd1306_send_data(&ssd);
                    printf("Tarefa 2 ativa\n");

                    // Tempo de exibição da mensagem
                    vTaskDelay(1000 / portTICK_PERIOD_MS);

                    // Tela de espera padrão
                    ssd1306_fill(&ssd, 0);
                    ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                    ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                    ssd1306_send_data(&ssd);

                    // Libera o mutex do display
                    xSemaphoreGive(xDisplayMutex);
                }
            }
            else
            {
                // Se não há usuários no local, mostra que o espaço está vazio
                gpio_put(LED_PIN_BLUE, true); // LED azul

                if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
                {
                    ssd1306_fill(&ssd, 0);
                    sprintf(buffer, "Usuarios: %d", usuariosNoLocal);
                    ssd1306_draw_string(&ssd, "Espaco ", 5, 10);
                    ssd1306_draw_string(&ssd, "Vazio!", 5, 19);
                    ssd1306_draw_string(&ssd, buffer, 5, 44);
                    ssd1306_send_data(&ssd);
                    
                    printf("Tarefa 2 ativa\n");

                    // Tempo de exibição da mensagem
                    vTaskDelay(1000 / portTICK_PERIOD_MS);

                    // Tela de espera padrão
                    ssd1306_fill(&ssd, 0);
                    ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                    ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                    ssd1306_send_data(&ssd);

                    xSemaphoreGive(xDisplayMutex);
                }
            }
        }
    }
}

// Função de tratamento de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time > 200)
    {
        // Botão A pressionado - sinaliza tarefa de entrada
        if (gpio == BOTAO_A)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            // Libera o semáforo de entrada a partir da ISR
            xSemaphoreGiveFromISR(xContadorSemA, &xHigherPriorityTaskWoken);
            // Solicita troca de contexto se necessário
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        // Botão B pressionado - sinaliza tarefa de saída
        else if (gpio == BOTAO_B)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xContadorSemB, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        // Botão do joystick pressionado - sinaliza tarefa de reset
        else if (gpio == JOYSTICK_BTN_PIN)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xResetSem, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        // Atualiza o tempo da última interrupção
        last_time = current_time;
    }
}

int main()
{
    // Inicializa a saída padrão (UART)
    stdio_init_all();

    // --- Configuração dos botões de entrada (BOTAO_A e BOTAO_B) e do botão do joystick ---
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A); // Habilita resistor pull-up

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_init(JOYSTICK_BTN_PIN);
    gpio_set_dir(JOYSTICK_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN_PIN);

    // --- Configura interrupções para os botões na borda de descida ---
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // --- Inicializa os LEDs como saída ---
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    // --- Garante que todos os LEDs comecem apagados ---
    gpio_put(LED_PIN_GREEN, false);
    gpio_put(LED_PIN_BLUE, false);
    gpio_put(LED_PIN_RED, false);

    // Indica estado inicial (sem usuários) com LED azul aceso
    gpio_put(LED_PIN_BLUE, true);

    // Inicializa o buzzer
    buzzer_init(BUZZER_PIN);

    // Inicializa o display OLED via I2C
    initDisplay(&ssd);

    // Mostra mensagem de "aguardando evento" no display
    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
    ssd1306_draw_string(&ssd, "  evento...", 5, 34);
    ssd1306_send_data(&ssd);

    // --- Criação dos semáforos ---
    xContadorSemA = xSemaphoreCreateCounting(10, 0); // Para eventos de entrada
    xContadorSemB = xSemaphoreCreateCounting(10, 0); // Para eventos de saída
    xResetSem = xSemaphoreCreateBinary();            // Para evento de reset
    xDisplayMutex = xSemaphoreCreateMutex();         // Protege acesso ao display

    // --- Criação das tarefas do FreeRTOS ---
    xTaskCreate(vTaskEntrada, "Entrada", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "Saida", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "Reset", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    // Inicia o escalonador do FreeRTOS
    vTaskStartScheduler();

    // Caso o escalonador não inicie corretamente entra em modo de pânico
    panic_unsupported();
}