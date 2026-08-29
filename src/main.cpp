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
#include "orc.h"


#define TIME_LED 100 // время мерцания
#define Z_IN       3 // 1-ый индекс дисплея с котрого начинается кастомное символьно число
#define FIRST_IN   8 // 1-ый индекс дисплея с котрого начинается кастомное символьно число
#define SECOND_IN 12 // 2-ой индекс дисплея с котрого начинается кастомное символьно число
#define THIRD_IN  16 // 3-ий индекс дисплея с котрого начинается кастомное символьно число


last4rounds l4r = {0,0,0,0};
queue_n quen;
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ДАННЫЕ
int rounds_per_session = 0;
double new_v;
double avg_v_last_n;       // средняя скорость выстрела за SHOT_N шаров
double avg_v_last_session; // средняя скорость выстрела за сессию
char w_code[3];

// НАСТРОЙКИ
double DISTANCE   = 0.1; // расстояние                            (м)      DIST
double MIN_V      =  80; // минимальная скорость выстрела         (м/с)    MN_V
double MAX_V      = 150; // максимальная скорость выстрела        (м/с)    MX_V
double EXPECTED_V = 120; // ожидаемая скорость                    (м/с)    ED_V
double MASS       = 0.3; // вес шара                              (г)      MASS
double MAX_E      = 3.0; // максимальная энергия выстрела         (джоуль) MX_E
int    SHOT_N     = 100; // количество выстрелов для замеров      (штук)   ST_N

// символы для больших цифр
byte slash_r[] = {B00011,B00111,B00111,B01111,B11110,B11100,B11100,B11000};
byte slash_l[] = {B11000,B11100,B11100,B11110,B01111,B00111,B00111,B00011};
byte up[]      = {B11111,B11111,B11111,B11111,B00000,B00000,B00000,B00000};
byte down[]    = {B00000,B00000,B00000,B00000,B11111,B11111,B11111,B11111};
byte tri_rd[]  = {B00001,B00011,B00011,B00111,B00111,B01111,B01111,B11111};
byte tri_ru[]  = {B11111,B11110,B11110,B11100,B11100,B11000,B11000,B10000};
byte tri_ld[]  = {B10000,B11000,B11000,B11100,B11100,B11110,B11110,B11111};
byte tri_lu[]  = {B11111,B01111,B01111,B00111,B00111,B00011,B00011,B00001};

//символы цифр 4 на 4
int custom_nums[10][4][4] = {   
                                {   
                                    {4, 2, 2, 6},
                                    {8, 9, 9, 8},
                                    {8, 9, 9, 8},
                                    {7, 3, 3, 5}// 0
                                },
                                {   
                                    {9, 9, 0, 6},
                                    {9, 0, 9, 8},
                                    {9, 9, 9, 8},
                                    {9, 9, 9, 8}// 1
                                },
                                {   
                                    {4, 8, 8, 6},
                                    {9, 9, 4, 5},
                                    {9, 4, 5, 9},
                                    {4, 8, 8, 6}// 2
                                },
                                {   
                                    {4, 8, 8, 6},
                                    {9, 9, 4, 5},
                                    {9, 9, 7, 6},
                                    {7, 8, 8, 5}// 3
                                },
                                {   
                                    {9, 8, 9, 8},
                                    {9, 7, 3, 8},
                                    {9, 9, 9, 8},
                                    {9, 9, 9, 8}// 4
                                },
                                {   
                                    {4, 2, 2, 2},
                                    {7, 6, 9, 9},
                                    {9, 2, 2, 6},
                                    {3, 3, 3, 5}// 5
                                },
                                {   
                                    {0, 2, 2, 1},
                                    {1, 3, 3, 9},
                                    {0, 2, 2, 1},
                                    {1, 3, 3, 0}// 6
                                },
                                {   
                                    {4, 8, 8, 6},
                                    {9, 9, 4, 5},
                                    {9, 4, 5, 9},
                                    {4, 5, 9, 9}// 7
                                },
                                {   
                                    {0, 2, 2, 1},
                                    {1, 3, 3, 0},
                                    {0, 2, 2, 1},
                                    {1, 3, 3, 0}// 8
                                },
                                {   
                                    {9, 0, 2, 1},
                                    {9, 1, 3, 0},
                                    {9, 9, 0, 9},
                                    {9, 0, 9, 9}// 9
                                }
};


int custom_letters[1][4][4] = {   
                                {   
                                    {2, 2, 2, 5},
                                    {9, 9, 0, 9},
                                    {9, 0, 9, 9},
                                    {4, 3, 3, 3}// z
                                },

};

// регистрация кастомных символов
void register_custom_symbols(){
    lcd.createChar(0, slash_r);
    lcd.createChar(1, slash_l);
    lcd.createChar(2, up);
    lcd.createChar(3, down);
    lcd.createChar(4, tri_rd);
    lcd.createChar(5, tri_ru);
    lcd.createChar(6, tri_ld);
    lcd.createChar(7, tri_lu);
}

// вывод символов
void print_symbols(const char* symbols, int row, int col){
    lcd.setCursor(col, row);
    lcd.printstr(symbols);
}

void print_symbols(double num_symbols, int row, int col){
    char symbols[6];
    if (num_symbols < 100) col++;
    if (num_symbols < 10) col++;
    dtostrf(num_symbols, 0, 0, symbols); 
    print_symbols(symbols, row, col);
}

// вывод настроек
void print_settings(){
    char distance[8];
    char min_v[6];
    char max_v[6];
    char expected_v[6];
    char mass[8];
    char max_e[8];
    char shot_n[6];

    print_symbols("DIST", 0, 0);
    print_symbols("MN_V", 1, 0);
    print_symbols("MX_V", 2, 0);
    print_symbols("ED_V", 3, 0);
    
    print_symbols("MASS", 0, 9);
    print_symbols("MX_E", 1, 9);
    print_symbols("ST_N", 2, 9);
    //print_symbols("ED_V", 3, 10);

    dtostrf(DISTANCE, 0, 1, distance);    // 1 знак после запятой
    dtostrf(MIN_V, 0, 0, min_v);          // 0 знаков после запятой
    dtostrf(MAX_V, 0, 0, max_v);
    dtostrf(EXPECTED_V, 0, 0, expected_v);
    dtostrf(MASS, 0, 1, mass);
    dtostrf(MAX_E, 0, 1, max_e);
    dtostrf(SHOT_N, 0, 0, shot_n); 

    print_symbols(distance, 0, 5);
    print_symbols(min_v, 1, 5);
    print_symbols(max_v, 2, 5);
    print_symbols(expected_v, 3, 5);
    print_symbols(mass, 0, 14);
    print_symbols(max_e, 1, 14);
    print_symbols(shot_n, 2, 14);
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
void default_print(){
    print_symbols(l4r.v4, 0, 0);
    print_symbols(l4r.v3, 1, 0);
    print_symbols(l4r.v2, 2, 0);
    print_symbols(l4r.v1, 3, 0);

    print_symbols(rounds_per_session, 0, 5);
    print_symbols(avg_v_last_n, 1, 5);
    print_symbols(avg_v_last_session, 2, 5);
    print_symbols(w_code, 3, 5);

    print_symbols("R", 0, 4);
    print_symbols("N", 1, 4);
    print_symbols("A", 2, 4);
    print_symbols("S", 3, 4);

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

// получить скорость выстрела(м/c)
double zamer_v(int time_s, int time_e){
    return DISTANCE / (time_e - time_s);
}

void print_data_field(char* code_e){
    print_symbols(code_e, 3, 4);
}

// получить энергию в джоулях
double zamer_e(double v, double mass){
    double e = mass/1000 * v * v / 2;
    if (e > MAX_E){ 
        print_data_field("WDE");
    }
    return e;
}

// получить среднюю скорость за последние SHOT_N выстрелов
double get_avg_v_last_n(queue_n *quen){
    double all_t = 0;
    noda_n* temp = quen->head;
    int k = 0;
    int shots = SHOT_N;
    while (k != SHOT_N || temp->value != 0){
        all_t += temp->value;
        temp = temp->next;
        k++;
    }
    free(temp);
    if (k != SHOT_N) shots = k;
    return shots * DISTANCE / all_t;
}

// получить средня скорость выстра за всю сессию
double get_avg_v_last_session(double* last_sessions_t, int rounds_per_session){
    double all_t = 0;
    for (int i = 0 ; i < rounds_per_session; i++){
        all_t += last_sessions_t[i];
    }
    return rounds_per_session * DISTANCE / all_t;
}

// получить валидированный выстрел
double get_validate_shot_v(double shot_v){
    double shot_vt;
    if (shot_v < 0){
        print_data_field("WM0");
        return EXPECTED_V;
    }
    if (shot_vt < MIN_V){
        print_data_field("WMm");
        return EXPECTED_V;
    }
    if (shot_vt > MAX_V){
        print_data_field("WMn");
        return EXPECTED_V;
    }
    print_data_field("NS");
    return shot_v;
}

// получить новую скорость
double get_new_v(){
    //zamer_v()
    return 0;
}



// старт программы
void setup() {
    lcd.init();
    lcd.backlight();
    register_custom_symbols();
    
    for (int i = 0 ; i < 3; i++){
        print_big_symbol_letter('z', Z_IN);
        print_big_num_velocity(150);
        delay(TIME_LED);
        lcd.clear();
        delay(TIME_LED);
    }
    print_settings();
    delay(TIME_LED*15);
    lcd.clear();
    new_queue_n(&quen, SHOT_N);
    default_print();
}

// обновление стандартного экрана
void update(){
    new_v = get_new_v();
    new_v = get_validate_shot_v(new_v);
    rounds_per_session++;
    qn_add(&quen, new_v);
    avg_v_last_n = get_avg_v_last_n(&quen);
    update_l4r(l4r, new_v);
    default_print();
}

// главный цикл
void loop() {
    
}