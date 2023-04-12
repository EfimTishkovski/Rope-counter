/*
 * Test_01.c
 *
 * Created: 20.02.2021 12:12:02  ReCreated: 02.01.2023 21:50:00
 * Author : Администратор
 * Программа для счётчика с семисегментным индикатором
 * Порт D 0-7  DP GFEDCBA
 * Порт С 0-2  аноды индикаторов  С0-1 С1-2 С2-3
 * Порт B B1-сигнал с датчика, B0-сброс
 */ 
# define F_CPU 8000000L

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


unsigned int digit_100 = 0;
unsigned int digit_10 = 0;
unsigned int digit_1 = 0;
unsigned char dp = 0;          // точка после цифры
const float k = 1.45;          // Коэфициент для пересщёта длины


//------------------------------------------------
// Функция отображения цифры на индикаторе
// drop - точка
void segchar (unsigned char num, unsigned char drop)
{
	if (drop==1)
	{
		switch(num)
		{
			case 1: PORTD = 0b01100001;break;   // 1.
			case 2: PORTD = 0b11011011;break;   // 2.
			case 3: PORTD = 0b11110011;break;   // 3.
			case 4: PORTD = 0b01100111;break;   // 4.
			case 5: PORTD = 0b10110111;break;   // 5.
			case 6: PORTD = 0b10111111;break;   // 6.
			case 7: PORTD = 0b11100001;break;   // 7.
			case 8: PORTD = 0b11111111;break;   // 8.
			case 9: PORTD = 0b11110111;break;   // 9.
			case 0: PORTD = 0b11111101;break;   // 0.

		}
	}
	if (drop==0)
	{
		switch(num)
		{
			case 1: PORTD = 0b01100000;break;   // 1
			case 2: PORTD = 0b11011010;break;   // 2
			case 3: PORTD = 0b11110010;break;   // 3
			case 4: PORTD = 0b01100110;break;   // 4
			case 5: PORTD = 0b10110110;break;   // 5
			case 6: PORTD = 0b10111110;break;   // 6
			case 7: PORTD = 0b11100000;break;   // 7
			case 8: PORTD = 0b11111110;break;   // 8
			case 9: PORTD = 0b11110110;break;   // 9
			case 0: PORTD = 0b11111100;break;   // 0

		}
	}
}

//------------------------------------------------
// Функция работы таймера
void timer_ini(void)
{
	TCCR1B |= (1<<WGM12);  // Устанавливаем режим CTC (сброс по совпадению)
	TIMSK |= (1<<OCIE1A);  // устанавливаем бит разрешения прерывания 1-го счётчика по совпадению с OCR1A(H и L)
	OCR1AH = 0b00001111;   // Записываем число в бит OCR1A (16 бит 2бита по 8 H и L) 8 000 000 / 256 = 31250
	OCR1AL = 0b01000010;   // корректировка конечное число 3096 
	TCCR1B |= (1<<CS11);   // Делитель  (число 8)
	
}
//------------------------------------------------

//Функция работы прерываний
//------------------------------------------------
unsigned char count = 0;
//------------------------------------------------
ISR(TIMER1_COMPA_vect)
{
	if (count==0) {PORTC&=~(1<<PORTC2);PORTC&=~(1<<PORTC1);PORTC|=(1<<PORTC0);segchar(digit_100, 0);}  // 001 сотни    1
	if (count==1) {PORTC&=~(1<<PORTC2);PORTC|=(1<<PORTC1);PORTC&=~(1<<PORTC0);segchar(digit_10, dp);}  // 010 десятки  2
	if (count==2) {PORTC|=(1<<PORTC2);PORTC&=~(1<<PORTC1);PORTC&=~(1<<PORTC0);segchar(digit_1, 0);}    // 100 единицы  3
	count++;
	if(count>2) count = 0;
}

//------------------------------------------------
// Функция разбивки числа на цифры для отображения
// Возможно переделать через switch
void show_number (float number)
{
	if (number < 100)
	{
		dp = 1;
		digit_100 = number / 10;                          // 10 
		digit_10 = number - (digit_100*10);               // 1
		digit_1 = (number - digit_100*10 - digit_10)*10;  // 0.1
	}
	if (number >= 100)
	{
		int number_buf = number;
		dp = 0;
		digit_100 = number_buf / 100;      // 100
		digit_10 = number_buf % 100 / 10;  // 10
		digit_1 = number_buf % 10;         // 1
	}
	if (number > 999)
	{
		dp = 1;
		digit_100 = 0;  // 10
		digit_10 = 0;   // 1
		digit_1 = 0;    // 0.1
	}
}
//-----------------------------------------------


// Главная
int main(void)
{
	unsigned char butstate = 0;        // Флаг сработки датчика
	unsigned char butstate_reset = 0;  // Флаг сработки кнопки сброса
	float main_counter = 0;            // Основной счётчик
    timer_ini();
	DDRD = 0xFF;               // Порт D весь на выход  0b A B C D E F G DP
	DDRC = 0b00000111;         // 0-2 на выход остальные ноги на вход
	DDRB = 0b00000000;         // или 0x00
	PORTB = 0b00000011;        // нога 0 кнопка сброса, при запуске 1; нога 1 датчик, при запуске 1
	sei();                     // функция разрешения глобальных прерываний  (Set Interput)
	
	
	// Инициализация при запуске
	
	show_number(888);        // Проверка индикатора на работоспособность
	_delay_ms(500);          // Пауза

	// Основной цикл
	
    while (1) 
    {
		show_number(main_counter);  // отображение значения
		
		// Обработка сигнала с датчика
		if (!(PINB&0b00000010)) //(работает по инверсии подключен к минусу)
		{
			if (butstate==0)
			{
				main_counter += k;
				butstate=1;
			}	
		}
		else
		{
			butstate=0;
		}
		
		// Сброс
		if (!(PINB&0b00000001)) //(работает по инверсии подключен к минусу)
		{
			if (butstate_reset==0)
			{
				main_counter = 0;
				butstate_reset=1;
			}
			
		}
		else
		{
			butstate_reset=0;
		}
	}
}

