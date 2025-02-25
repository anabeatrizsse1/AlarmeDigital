#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <inc/ssd1306.h>

// Definição de pinos
#define BUTTON_HOUR 5
#define BUTTON_MINUTE 6
#define LED_ALARM 12
#define BUZZER 10
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C

// Variáveis de tempo
int horas = 12, minutos = 0, segundos = 0;
int alarme_hora = 12, alarme_minuto = 1;


bool alarme_ativo = false;
int buzzer_timer = 0;
int ms_counter = 0;
bool modo_ajuste_alarme = false;  // Indica se o usuário está configurando o alarme

// Inicialização do display
ssd1306_t display;

void init_display() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&display, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&display);
    ssd1306_send_data(&display);
}

// Atualiza o display
void update_display() {
    char time_str[10];
    char alarm_str[20];

    sprintf(time_str, "%02d:%02d:%02d", horas, minutos, segundos);
    
    if (modo_ajuste_alarme) {
        sprintf(alarm_str, "AJUSTE: %02d:%02d", alarme_hora, alarme_minuto);
    } else {
        sprintf(alarm_str, "ALARME: %02d:%02d", alarme_hora, alarme_minuto);
    }

    ssd1306_fill(&display, false);  
    ssd1306_text(&display, 2, 2, time_str, true);   
    ssd1306_text(&display, 2, 50, alarm_str, true);  
    ssd1306_send_data(&display);
}

// Ajusta o relógio ou o alarme
void check_buttons() {
    if (!gpio_get(BUTTON_HOUR) && !gpio_get(BUTTON_MINUTE)) {
        modo_ajuste_alarme = !modo_ajuste_alarme;  
        sleep_ms(500); 
    }

    if (modo_ajuste_alarme) {
        if (!gpio_get(BUTTON_HOUR)) {
            alarme_hora = (alarme_hora + 1) % 24;
            sleep_ms(200);
        }
        if (!gpio_get(BUTTON_MINUTE)) {
            alarme_minuto = (alarme_minuto + 1) % 60;
            sleep_ms(200);
        }
    } else {
        if (!gpio_get(BUTTON_HOUR)) {
            horas = (horas + 1) % 24;
            sleep_ms(200);
        }
        if (!gpio_get(BUTTON_MINUTE)) {
            minutos = (minutos + 1) % 60;
            sleep_ms(200);
        }
    }
}

// Configuração dos pinos
void setup() {
    stdio_init_all();
    sleep_ms(2000);
    init_display();

    gpio_init(BUTTON_HOUR);
    gpio_set_dir(BUTTON_HOUR, GPIO_IN);
    gpio_pull_up(BUTTON_HOUR);

    gpio_init(BUTTON_MINUTE);
    gpio_set_dir(BUTTON_MINUTE, GPIO_IN);
    gpio_pull_up(BUTTON_MINUTE);

    gpio_init(LED_ALARM);
    gpio_set_dir(LED_ALARM, GPIO_OUT);

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
}

// Loop principal
int main() {
    setup();

    while (true) {
        sleep_ms(250);
        ms_counter += 250;

        // Atualiza segundos
        if (ms_counter >= 1000) {
            ms_counter = 0;
            segundos++;
            if (segundos >= 60) {
                segundos = 0;
                minutos++;
                if (minutos >= 60) {
                    minutos = 0;
                    horas = (horas + 1) % 24;
                }
            }
        }

        check_buttons();

        // Ativa o alarme no momento certo
        if (horas == alarme_hora && minutos == alarme_minuto && segundos == 0) {
            alarme_ativo = true;
            buzzer_timer = 240;  
        }

        // Controle do alarme
        if (alarme_ativo) {
            gpio_put(LED_ALARM, 1);

            if (buzzer_timer > 0) {
                gpio_put(BUZZER, buzzer_timer % 2);  
                buzzer_timer--;
            } else {
                gpio_put(BUZZER, 0);
                alarme_ativo = false;
            }
        } else {
            gpio_put(LED_ALARM, 0);
        }

        update_display();
        printf("Hora atual: %02d:%02d:%02d | Alarme: %02d:%02d %s\n", 
               horas, minutos, segundos, 
               alarme_hora, alarme_minuto, 
               modo_ajuste_alarme ? "(AJUSTANDO)" : "");
    }

    return 0;
}
