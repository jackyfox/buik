﻿/**  
 *  Прошивка блока управления импульсным клапаном
 *  -- buik01.c --
 */

#include <avr/io.h>
#include <util/delay.h>


#define switch_led() PORTB ^= (1 << PB0)  // Инвертируем сигнал на PB0 (переключаем СД)
#define green_led()  PORTB |= (1 << PB0)  // Устанавливаем PB0 «1» (включаем зеленый СД)
#define red_led()    PORTB &= ~(1 << PB0) // Устанавливаем PB0 «0» (включаем красный СД)

// Семантические синонимы пинов
#define LED_PIN      PB0 // Светодиоды
#define L2_PWROFF    PB1 // Второй порог или отключение питания
#define VALVE_ONOFF  PB2 // Состояние клапана (открыт/закрыт)
#define VALVE_CLOSE  PB3 // Сигнал на закрытие клапана
#define VALVE_BREAK  PB4 // Обрыв клапана

// Сокращенные написания временных задержек
#define wait40()   do { _delay_loop_2(5200); } while(0)
#define wait350()  do { _delay_loop_2(45850); } while(0)
#define wait500()  do { _delay_loop_2(65500); _delay_loop_2(65500); } while(0)
#define wait650()  do { _delay_loop_2(65500); _delay_loop_2(19650); } while(0)
#define veryfast() do { _delay_loop_2(1000); } while(0)

void ioinit(void)
{
    /** 
     * Настраиваем входы и выходы
     * PB0 (pin 5) на выход (1) Управление светодиодами
     * PB1 (pin 6) на вход  (0) Порог или отключено питание
     * PB2 (pin 7) на вход  (0) Состояние клапана
     * PB3 (pin 2) ны выход (1) Сигнал закрытия клапана
     * PB4 (pin 3) на вход  (0) Обрыв клапана
     */

    DDRB = ((1 << LED_PIN)|(1 << VALVE_CLOSE));
    //DDRB = (1 << LED_PIN);
    PORTB |= (1 << VALVE_CLOSE);  // Единица на порт
    
   
    // Отключаем аналоговый компаратор
    ADCSRB |= (1 << ACD);
}

int main(void) 
{	
    uint8_t i;                  // Счетчик цикла
    uint8_t close_attempts = 0; // Счетчик попыток закрытия клапана
    uint8_t fake_close = 0;     // Счетчик на проверке ложного закрытия клапана
    
    // Начальная настройка
    ioinit();
    
    green_led();

    // Включение. Светодиоды мерцают с полусекундной задержкой в течении пяти секунд
    for (i = 0; i < 10; ++i)
    {
        switch_led();
        wait500();
    }

    // По умолчанию всё хорошо. Горит зелёный СД
    green_led();

    for(;;)
    { 	// Основной цикл
        while(!(PINB & (1 << VALVE_BREAK))) // !(PINB & (1 << VALVE_BREAK))
        {
            /** 
             * Проверка обрыва клапана. Наличие клапана [1 - подключен], [0 - оборван]
             * Мигаем светодиодами так быстро, что они сливаются
             * Сообщаем тем самым об обрыве клапана
             */
            switch_led();
            veryfast();
        } // while - обрыв клапана
        
        if (PINB & (1 << VALVE_ONOFF)) // PINB & (1 << VALVE_ONOFF)
        {	
            /** 
             * Состояние клапана [1 - открыт], [0 - закрыт]
             * Клапан открыт. Горит зеленый СД
             */
            green_led(); 
        }
        else
        {
            /**
             * Клапан закрыт. Горит красный СД
             */
            red_led();
            // Сбрасываем счетчики попыток ложного закрытия и закрытия клапана
            fake_close = 0;
            close_attempts = 0;
            // Так как проверять наличие сигнала на закрытие клапана при закрытом кланане
            // не нужно, то возвращаемся в начало, к проверке обрыва
            continue;
        } // if VALVE_ONOFF
        
        if (!(PINB & (1 << L2_PWROFF))) // !(PINB & (1 << L2_PWROFF))
        {
            /**
             * Сигнал второго порога или отключения питания [0 - есть 2-й порог или пропало питание]
             */
            
            // Счетчик ложного сигнала на закрытие клапана.
            // Немного подождем, вдруг ложная тревога.
            // Ждем сорок милисекунд
            
            if (fake_close++ <= 1)
            {
                wait40();
                continue;
            }
            
            // И если сигнал всё ещё есть, закрываем клапан
            if (close_attempts++ <= 3) 
            {
                /**
                 * Проверяем, не было ли попыток закрытия.
                 * Если попыток закрытия было меньше 3-х, и клапан открыт,
                 * подаем импульс длиной 0.35 секунды
                 */
                
                PORTB &= ~(1 << VALVE_CLOSE); // Ноль на порт
                wait350();
                PORTB |= (1 << VALVE_CLOSE);  // Единица на порт
                wait650();
            }
        }
        else
        {
            /**
             * Сигнал второго порога или отключения питания пропал.
             * Можно сбрасывать счетчик ложных срабатываний
             */
             
            fake_close = 0;
        } // if L2_PWROFF
    } // for(;;) - основной цикл

    return 0;

};
