#ifndef __TASK_H
#define __TASK_H

// https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
#include "Arduino.h" // Biblioteca base do framework Arduino, necessária para funções básicas como Serial e delays.

hw_timer_t *timer = nullptr; // Configuração global do timer
QueueHandle_t taskQueue;     // Declaração de uma fila do FreeRTOS para armazenar as ações que devem ser feitas

struct CounterConfig
{
    uint16_t counter; // Ponteiro para o contador
    uint16_t limit;   // Limite do contador
    void (*task)();
};

CounterConfig *taskStruct = nullptr; // Ponteiro para o array de configurações
size_t taskCount = 0;                // Quantidade de configurações no array

// Função chamada pelo timer
void IRAM_ATTR timerCallback()
{
    for (size_t i = 0; i < taskCount; ++i)
    {
        CounterConfig &c = taskStruct[i];
        if (++c.counter == c.limit)
        {
            xQueueSendFromISR(taskQueue, &c.task, nullptr); // Envia o valor para a fila
            c.counter = 0;
        }
    }
}

// Configuração inicial do timer
void taskSetup(CounterConfig *tasks, size_t count, uint32_t frequency=1000, uint32_t bufferSize=1024)
{
    taskStruct = tasks; // Passa o ponteiro do array de configurações para a variável global
    taskCount = count;  // Armazena o tamanho do array
    // Configuração do timer
    timer = timerBegin(frequency);                           // Inicializa o timer com a frequência de amostragem.
    timerAttachInterrupt(timer, &timerCallback);             // Associa a função timerCallback como a ISR do timer.
    timerAlarm(timer, (80000000 / 80) / frequency, true, 0); //  // Set alarm to call onTimer (value in microseconds).

    // Inicializa a fila com capacidade para NUM_SAMPLES elementos, cada um do tamanho de uint16_t.
    taskQueue = xQueueCreate(bufferSize, sizeof(uint16_t));
    if (taskQueue == nullptr)
    {                                            // Verifica se a criação da fila falhou.
        Serial.println("Erro ao criar a fila!"); // Exibe uma mensagem de erro no monitor serial.
        return;                                  // Interrompe a execução da função caso a fila não tenha sido criada.
    }
}

void taskLoop()
{
    void (*taskMessage)(); // Declaração de um ponteiro para uma função, que será recebido da fila.
    // Verifica continuamente se há dados na fila
    while (xQueueReceive(taskQueue, &taskMessage, 0) == pdTRUE)
    {
        // Se xQueueReceive retorna pdTRUE, significa que há um item disponível na fila:
        // - O dado é removido da fila e armazenado em taskMessage.
        taskMessage();
    }
}

#endif