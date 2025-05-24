#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "lib/ssd1306.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

#define BOTAO_A 5
#define BOTAO_B 6
#define MAX_USUARIOS 10
#define LED_PIN_GREEN 11
#define LED_PIN_BLUE 12
#define LED_PIN_RED 13
#define JOYSTICK_BTN_PIN 22

SemaphoreHandle_t xContadorSemA;
SemaphoreHandle_t xContadorSemB;
SemaphoreHandle_t xDisplayMutex;
SemaphoreHandle_t xResetSem;

ssd1306_t ssd;
uint16_t eventosProcessados = 0;
uint32_t last_time;

void vTaskReset(void *params)
{
    char buffer[32];

    while (true)
    {
        if (xSemaphoreTake(xResetSem, portMAX_DELAY) == pdTRUE)
        {
            eventosProcessados = 0;

            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);

            // Nenhum usuário logado → Azul
            gpio_put(LED_PIN_BLUE, true);

            if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
            {
                // Atualiza display com a nova contagem
                ssd1306_fill(&ssd, 0);
                sprintf(buffer, "Usuarios: %d", eventosProcessados);
                ssd1306_draw_string(&ssd, "Reset ", 5, 10);
                ssd1306_draw_string(&ssd, "Detectado!", 5, 19);
                ssd1306_draw_string(&ssd, buffer, 5, 44);
                ssd1306_send_data(&ssd);
                printf("Tarefa 3 ativa\n");

                // Simula tempo de processamento
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                // Retorna à tela de espera
                ssd1306_fill(&ssd, 0);
                ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                ssd1306_send_data(&ssd);

                xSemaphoreGive(xDisplayMutex);
            }
        }
    }
}

void vTaskEntrada(void *params)
{
    char buffer[32];

    while (true)
    {
        // Aguarda semáforo (um evento)
        if (xSemaphoreTake(xContadorSemA, portMAX_DELAY) == pdTRUE && eventosProcessados < MAX_USUARIOS)
        {
            eventosProcessados++;

            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);

            if (eventosProcessados < MAX_USUARIOS - 1)
            {
                // Usuários ativos de 1 até MAX-2 → Verde
                gpio_put(LED_PIN_GREEN, true);
            }
            else if (eventosProcessados == MAX_USUARIOS - 1)
            {
                // Apenas 1 vaga restante → Amarelo
                gpio_put(LED_PIN_GREEN, true);
                gpio_put(LED_PIN_RED, true);
            }
            else if (eventosProcessados == MAX_USUARIOS)
            {
                // Capacidade máxima → Vermelho
                gpio_put(LED_PIN_RED, true);
            }

            if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
            {
                // Atualiza display com a nova contagem
                ssd1306_fill(&ssd, 0);
                sprintf(buffer, "Usuarios: %d", eventosProcessados);
                ssd1306_draw_string(&ssd, "Entrada ", 5, 10);
                ssd1306_draw_string(&ssd, "Detectada!", 5, 19);
                ssd1306_draw_string(&ssd, buffer, 5, 44);
                ssd1306_send_data(&ssd);
                printf("Tarefa 1 ativa\n");

                // Simula tempo de processamento
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                // Retorna à tela de espera
                ssd1306_fill(&ssd, 0);
                ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                ssd1306_send_data(&ssd);

                xSemaphoreGive(xDisplayMutex);
            }
        }
    }
}

void vTaskSaida(void *params)
{
    char buffer[32];

    while (true)
    {
        // Aguarda semáforo (um evento)
        if (xSemaphoreTake(xContadorSemB, portMAX_DELAY) == pdTRUE && eventosProcessados > 0)
        {
            eventosProcessados--;

            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);
            if (eventosProcessados == 0)
            {
                // Nenhum usuário logado → Azul
                gpio_put(LED_PIN_BLUE, true);
            }
            else if (eventosProcessados < MAX_USUARIOS - 1)
            {
                // Usuários ativos de 1 até MAX-2 → Verde
                gpio_put(LED_PIN_GREEN, true);
            }
            else if (eventosProcessados == MAX_USUARIOS - 1)
            {
                // Apenas 1 vaga restante → Amarelo
                gpio_put(LED_PIN_GREEN, true);
                gpio_put(LED_PIN_RED, true);
            }

            if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
            {
                // Atualiza display com a nova contagem
                ssd1306_fill(&ssd, 0);
                sprintf(buffer, "Usuarios: %d", eventosProcessados);
                ssd1306_draw_string(&ssd, "Saida ", 5, 10);
                ssd1306_draw_string(&ssd, "Detectada!", 5, 19);
                ssd1306_draw_string(&ssd, buffer, 5, 44);
                ssd1306_send_data(&ssd);
                printf("Tarefa 2 ativa\n");

                // Simula tempo de processamento
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                // Retorna à tela de espera
                ssd1306_fill(&ssd, 0);
                ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
                ssd1306_draw_string(&ssd, "  evento...", 5, 34);
                ssd1306_send_data(&ssd);

                xSemaphoreGive(xDisplayMutex);
            }
        }
    }
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time > 200)
    {

        if (gpio == BOTAO_A)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xContadorSemA, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else if (gpio == BOTAO_B)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xContadorSemB, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else if (gpio == JOYSTICK_BTN_PIN)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xResetSem, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        last_time = current_time;
    }
}

int main()
{
    stdio_init_all();

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_init(JOYSTICK_BTN_PIN);
    gpio_set_dir(JOYSTICK_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN_PIN);

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    gpio_put(LED_PIN_GREEN, false);
    gpio_put(LED_PIN_BLUE, false);
    gpio_put(LED_PIN_RED, false);

    // Nenhum usuário logado → Azul
    gpio_put(LED_PIN_BLUE, true);

    // Inicializa I2C e display OLED
    initDisplay(&ssd);

    // Tela inicial
    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "Aguardando ", 5, 25);
    ssd1306_draw_string(&ssd, "  evento...", 5, 34);
    ssd1306_send_data(&ssd);

    xContadorSemA = xSemaphoreCreateCounting(10, 0);
    xContadorSemB = xSemaphoreCreateCounting(10, 0);
    xResetSem = xSemaphoreCreateBinary();
    xDisplayMutex = xSemaphoreCreateMutex();

    // Cria as tarefas
    xTaskCreate(vTaskEntrada, "Entrada", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "Saida", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "Reset", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
