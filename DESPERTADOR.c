#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <inc/ssd1306.h>

// Definicao de pinos para botoes, LED, buzzer e I2C
#define BUTTON_HOUR 5      // Botao para ajuste da hora
#define BUTTON_MINUTE 6    // Botao para ajuste dos minutos
#define LED_ALARM 12       // LED indicador do alarme
#define BUZZER 10          // Buzzer que toca quando o alarme dispara
#define I2C_PORT i2c1      // Porta I2C usada para comunicacao com o display OLED
#define I2C_SDA 14         // Pino SDA do I2C
#define I2C_SCL 15         // Pino SCL do I2C
#define OLED_ADDR 0x3C     // Endereco do display OLED

// Variaveis de tempo
int horas = 12, minutos = 0, segundos = 0;   // Relogio inicializado em 12:00:00
int alarme_hora = 12, alarme_minuto = 1;     // Alarme configurado para 12:01

// Variaveis auxiliares
bool alarme_ativo = false;  // Indica se o alarme esta ativado
int buzzer_timer = 0;       // Contador do tempo do buzzer
int ms_counter = 0;         // Contador de milissegundos para atualizar os segundos
bool modo_ajuste_alarme = false;  // Indica se o usuario esta configurando o alarme

// Objeto do display OLED
ssd1306_t display;

// Funcao para inicializar o display OLED
void init_display() {
    i2c_init(I2C_PORT, 400 * 1000);  // Inicializa o barramento I2C com 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);  // Ativa pull-up interno do SDA
    gpio_pull_up(I2C_SCL);  // Ativa pull-up interno do SCL

    ssd1306_init(&display, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);  // Inicializa o display
    ssd1306_config(&display);  
    ssd1306_send_data(&display);
}

// Funcao para atualizar o display OLED
void update_display() {
    char time_str[10];  
    char alarm_str[20];

    sprintf(time_str, "%02d:%02d:%02d", horas, minutos, segundos);  // Formata a hora para exibicao

    // Se estiver ajustando o alarme, exibe "AJUSTE", senao exibe "ALARME"
    if (modo_ajuste_alarme) {
        sprintf(alarm_str, "AJUSTE: %02d:%02d", alarme_hora, alarme_minuto);
    } else {
        sprintf(alarm_str, "ALARME: %02d:%02d", alarme_hora, alarme_minuto);
    }

    ssd1306_fill(&display, false);  // Limpa o display
    ssd1306_text(&display, 2, 2, time_str, true);  // Exibe a hora no topo
    ssd1306_text(&display, 2, 50, alarm_str, true);  // Exibe o alarme na parte inferior
    ssd1306_send_data(&display);
}

// Funcao para verificar os botoes e ajustar o relogio ou o alarme
void check_buttons() {
    // Se ambos os botoes forem pressionados, alterna entre ajuste do relogio e do alarme
    if (!gpio_get(BUTTON_HOUR) && !gpio_get(BUTTON_MINUTE)) {
        modo_ajuste_alarme = !modo_ajuste_alarme;  
        sleep_ms(500); // Pequeno atraso para evitar multiplas mudancas
    }

    // Se estiver no modo de ajuste do alarme
    if (modo_ajuste_alarme) {
        if (!gpio_get(BUTTON_HOUR)) {
            alarme_hora = (alarme_hora + 1) % 24;  // Incrementa a hora do alarme
            sleep_ms(200);
        }
        if (!gpio_get(BUTTON_MINUTE)) {
            alarme_minuto = (alarme_minuto + 1) % 60;  // Incrementa os minutos do alarme
            sleep_ms(200);
        }
    } else { // Se estiver no modo de ajuste do relogio
        if (!gpio_get(BUTTON_HOUR)) {
            horas = (horas + 1) % 24;  // Incrementa a hora do relogio
            sleep_ms(200);
        }
        if (!gpio_get(BUTTON_MINUTE)) {
            minutos = (minutos + 1) % 60;  // Incrementa os minutos do relogio
            sleep_ms(200);
        }
    }
}

// Funcao de configuracao dos pinos GPIO
void setup() {
    stdio_init_all();
    sleep_ms(2000);  // Pequeno atraso para inicializacao completa
    init_display();  // Inicializa o display OLED

    // Configuracao dos botoes como entrada com pull-up
    gpio_init(BUTTON_HOUR);
    gpio_set_dir(BUTTON_HOUR, GPIO_IN);
    gpio_pull_up(BUTTON_HOUR);

    gpio_init(BUTTON_MINUTE);
    gpio_set_dir(BUTTON_MINUTE, GPIO_IN);
    gpio_pull_up(BUTTON_MINUTE);

    // Configuracao do LED e buzzer como saida
    gpio_init(LED_ALARM);
    gpio_set_dir(LED_ALARM, GPIO_OUT);

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
}

// Loop principal do programa
int main() {
    setup();  // Chama a configuracao inicial

    while (true) {
        sleep_ms(250);  // Delay de 250ms para controle de tempo
        ms_counter += 250;

        // Atualiza os segundos a cada 1 segundo (1000ms)
        if (ms_counter >= 1000) {
            ms_counter = 0;
            segundos++;
            if (segundos >= 60) {
                segundos = 0;
                minutos++; // atualiza os minutos
                if (minutos >= 60) { 
                    minutos = 0;
                    horas = (horas + 1) % 24; // Atualiza as horas 
                }
            }
        }

        check_buttons();  // Verifica se ha ajustes a serem feitos

        // Verifica se o horario atual coincide com o alarme
        if (horas == alarme_hora && minutos == alarme_minuto && segundos == 0) {
            alarme_ativo = true;
            buzzer_timer = 240;  // Duracao do buzzer (240 iteracoes)
        }

        // Controle do LED e do buzzer quando o alarme toca
        if (alarme_ativo) {
            gpio_put(LED_ALARM, 1);  // Acende o LED

            if (buzzer_timer > 0) {
                gpio_put(BUZZER, buzzer_timer % 2);  // Alterna o buzzer para piscar
                buzzer_timer--;
            } else {
                gpio_put(BUZZER, 0);  // Desliga o buzzer apos o tempo definido
                alarme_ativo = false;
            }
        } else {
            gpio_put(LED_ALARM, 0);  // Desliga o LED quando o alarme nao esta ativo
        }

        update_display();  // Atualiza o display com a hora e o alarme

        // Exibe informacoes no console para depuracao
        printf("Hora atual: %02d:%02d:%02d | Alarme: %02d:%02d %s\n", 
               horas, minutos, segundos, 
               alarme_hora, alarme_minuto, 
               modo_ajuste_alarme ? "(AJUSTANDO)" : "");
    }

    return 0;  
}
