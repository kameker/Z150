/*
    z150 - прошивка хронографа страйкбольного Z150
    made by ivan505
*/

/*
WM0 - меньше нуля
WMm - меньше минимального  MIN_V
WMm - больше максимального MAX_V
NS   - скорость нормальная
WDE  - энергия больше максимально допущенной
*/

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include "orc.hpp"
#include "custom_symbols.hpp"

#define TIME_LED 100        // время мерцания
#define Z_IN       3        // 1-ый индекс дисплея с котрого начинается кастомное символьно число
#define FIRST_IN   8        // 1-ый индекс дисплея с котрого начинается кастомное символьно число
#define SECOND_IN 12        // 2-ой индекс дисплея с котрого начинается кастомное символьно число
#define THIRD_IN  16        // 3-ий индекс дисплея с котрого начинается кастомное символьно число

#define SIZE_NUM_INT 6      // кол-во разрядов для вывода целого числа
#define SIZE_NUM_FLOAT 8    // кол-во разрядов для вывода целого числа

uint8_t SENSOR_PIN_1 = 7;   // пин 1-го датчика
uint8_t SENSOR_PIN_2 = 8;   // пин 2-го датчика

last4rounds l4r = {0,0,0,0};
queue_n quen;
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ДАННЫЕ
double summ_v;
unsigned int rounds_per_session = 0;
double new_v = 0;
double avg_v_last_n;       // средняя скорость выстрела за SHOT_N шаров
double avg_v_last_session; // средняя скорость выстрела за сессию
const char *w_code = "NS";
bool start = false;
bool finish = false;
volatile unsigned long start_time = 0;
volatile bool shot_detected = false;

// НАСТРОЙКИ
double DISTANCE         = 0.1; // расстояние                            (м)      DIST
double MIN_V            =  80; // минимальная скорость выстрела         (м/с)    MN_V
double MAX_V            = 150; // максимальная скорость выстрела        (м/с)    MX_V
double EXPECTED_V       = 120; // ожидаемая скорость                    (м/с)    ED_V
double MASS             = 0.3; // вес шара                              (г)      MASS
double MAX_E            = 3.0; // максимальная энергия выстрела         (джоуль) MX_E
unsigned int SHOT_N     = 100; // количество выстрелов для замеров      (штук)   ST_N

// вывод символов
void print_symbols(LiquidCrystal_I2C lcdt, const char* symbols, int row, int col){
    lcdt.setCursor(col, row);
    lcdt.printstr(symbols);
}

void print_symbols(LiquidCrystal_I2C lcdt, double num_symbols, int row, int col){
    char symbols[SIZE_NUM_INT];
    if (num_symbols < 100) col++;
    if (num_symbols < 10) col++;
    dtostrf(num_symbols, 0, 0, symbols); 
    print_symbols(lcdt, symbols, row, col);
}

// вывод настроек
void print_settings(LiquidCrystal_I2C lcdt){
    char distance[SIZE_NUM_INT + 2];
    char min_v[SIZE_NUM_INT];
    char max_v[SIZE_NUM_INT];
    char expected_v[SIZE_NUM_INT];
    char mass[SIZE_NUM_FLOAT];
    char max_e[SIZE_NUM_FLOAT];
    char shot_n[SIZE_NUM_INT];

    print_symbols(lcdt, "DIST", 0, 0);
    print_symbols(lcdt, "MN_V", 1, 0);
    print_symbols(lcdt, "MX_V", 2, 0);
    print_symbols(lcdt, "ED_V", 3, 0);
    
    print_symbols(lcdt, "MASS", 0, 9);
    print_symbols(lcdt, "MX_E", 1, 9);
    print_symbols(lcdt, "ST_N", 2, 9);
    //print_symbols(lcdt, "ED_V", 3, 10);

    dtostrf(DISTANCE, 0, 1, distance);    // 1 знак после запятой
    dtostrf(MIN_V, 0, 0, min_v);          // 0 знаков после запятой
    dtostrf(MAX_V, 0, 0, max_v);
    dtostrf(EXPECTED_V, 0, 0, expected_v);
    dtostrf(MASS, 0, 1, mass);
    dtostrf(MAX_E, 0, 1, max_e);
    dtostrf(SHOT_N, 0, 0, shot_n); 

    print_symbols(lcdt, distance, 0, 5);
    print_symbols(lcdt, min_v, 1, 5);
    print_symbols(lcdt, max_v, 2, 5);
    print_symbols(lcdt, expected_v, 3, 5);
    print_symbols(lcdt, mass, 0, 14);
    print_symbols(lcdt, max_e, 1, 14);
    print_symbols(lcdt, shot_n, 2, 14);
}

// выводит символ числа на позицию в дисплее
void print_custom_sybmol_num(int num, int lcd_i){
    if (num < 0 || num > 9 || (lcd_i != FIRST_IN && lcd_i != SECOND_IN && lcd_i != THIRD_IN)) return;
    if (num == 0 && lcd_i == FIRST_IN) return;
    if (new_v == 0 && lcd_i == SECOND_IN) return;
    for (int row = 0; row < 4; row++){
        for (int col = 0; col < 4; col++){
            int val = custom_nums[num][row][col];
            lcd.setCursor(lcd_i + col, row);
            if (val == 8) {
                lcd.write(255);  // полный блок 
            } else if (val == 9) {
                lcd.write(' ');   // пробел
            } else {
                lcd.write(val);   
            }
        }
    }
}

// вывод 3-ых больших символов-цифр
void print_big_num_velocity(int num){
    print_custom_sybmol_num(num % 10, THIRD_IN);
    print_custom_sybmol_num(num % 100 / 10, SECOND_IN);
    print_custom_sybmol_num(num / 100, FIRST_IN);
}

// стандартный вывод
void default_print(LiquidCrystal_I2C lcdt){
    print_symbols(lcdt, l4r.v4, 0, 0);
    print_symbols(lcdt, l4r.v3, 1, 0);
    print_symbols(lcdt, l4r.v2, 2, 0);
    print_symbols(lcdt, l4r.v1, 3, 0);

    print_symbols(lcdt, rounds_per_session, 0, 5);
    print_symbols(lcdt, avg_v_last_n, 1, 5);
    print_symbols(lcdt, avg_v_last_session, 2, 5);
    print_symbols(lcdt, w_code, 3, 5);

    print_symbols(lcdt, "R", 0, 4);
    print_symbols(lcdt, "N", 1, 4);
    print_symbols(lcdt, "A", 2, 4);
    print_symbols(lcdt, "S", 3, 4);

    print_big_num_velocity(new_v);
}

// вывод большого символа буквы
void print_big_symbol_letter(char letter, int lcd_i){
    letter -= 122;
    if (lcd_i > THIRD_IN) return;
    
    for (int row = 0; row < 4; row++){
        for (int col = 0; col < 4; col++){
            int val = custom_letters[letter][row][col];
            lcd.setCursor(lcd_i + col, row);
            if (val == 8) {
                lcd.write(255);  // Полный блок 
            } else if (val == 9) {
                lcd.write(' ');   // Пробел
            } else {
                lcd.write(val);   
            }
        }
    }
}

// вывод во всю матрицу
void print_204(int matrix[4][20]){
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 20; j++){
            if (matrix[i][j] == 1){
                lcd.setCursor(j, i);
                lcd.write(255);
            }
        }
    }
}


void print_data_field(LiquidCrystal_I2C lcdt, char* code_e){
    print_symbols(lcdt, code_e, 3, 4);
}

// получить скорость выстрела(м/c)
double zamer_v(unsigned long time_s, unsigned long time_e){
    if (time_e - time_s < 0) return -1;
    return DISTANCE / (time_e - time_s);
}

// получить энергию в джоулях
double zamer_e(double v, double mass){
    double e = mass/1000 * v * v / 2;
    if (e > MAX_E){ 
        print_data_field(lcd, "WDE");
    }
    return e;
}

// получить среднюю скорость за последние SHOT_N выстрелов
double get_avg_v_last_n(queue_n *quen){
    if (quen->head == NULL) return 0;
    
    double all_t = 0;
    noda_n* temp = quen->head;
    int k = 0;
    
    while (temp != NULL && k < SHOT_N){
        all_t += temp->value;
        temp = temp->next;
        k++;
    }
    
    if (k == 0 || all_t == 0) return 0;
    return (k * DISTANCE) / all_t;
}

// получить средня скорость выстра за всю сессию
double get_avg_v_last_session(){
    if (rounds_per_session == 0) return 0;
    return summ_v / rounds_per_session;
}

// получить валидированный выстрел
double get_validate_shot_v(double shot_v){
    if (shot_v < 0){
        w_code = "WM0";
        return EXPECTED_V;
    }
    if (shot_v < MIN_V){
        w_code = "WMm";
        return EXPECTED_V;
    }
    if (shot_v > MAX_V){
        w_code = "WMn";
        return EXPECTED_V;
    }
    w_code = "NS";
    return shot_v;
}

// В прерывании или при обнаружении выстрела
void on_shot_start() {
    start_time = micros();
    start = true;
}

void on_shot_end() {
    unsigned long end_time = micros();
    if (start_time > 0) {
        new_v = zamer_v(start_time, end_time);
        finish = true;
        start_time = 0;    
    }
}

double get_new_v(){
    if (finish) {
        finish = false;
        return new_v;
    }
    return 0;
}

// старт программы
void setup() {
    lcd.init();
    lcd.backlight();
    register_custom_symbols(lcd);
    
    for (int i = 0 ; i < 3; i++){
        print_big_symbol_letter('z', Z_IN);
        print_big_num_velocity(150);
        delay(TIME_LED);
        lcd.clear();
        delay(TIME_LED);
    }
    print_settings(lcd);
    delay(TIME_LED*15);
    lcd.clear();
    new_queue_n(&quen, SHOT_N);
    default_print(lcd);
    pinMode(SENSOR_PIN_1, INPUT_PULLUP);
    pinMode(SENSOR_PIN_2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_1), on_shot_start, RISING);
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_2), on_shot_end, RISING);
}

// обновление стандартного экрана
void update(){
    new_v = get_new_v();
    new_v = get_validate_shot_v(new_v);
    rounds_per_session++;
    summ_v += new_v;
    qn_add(&quen, new_v);
    avg_v_last_n = get_avg_v_last_n(&quen);
    avg_v_last_session = get_avg_v_last_session();
    update_l4r(l4r, new_v);
    start = false;
    finish = false;
    default_print(lcd);
}

// главный цикл
void loop() {
    if (finish && start){
        update();
    }
}