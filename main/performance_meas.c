/*
 * Codigo Tomado y modeficado de: https://esp32.com/viewtopic.php?p=82090#p82090
 *
 *
 */
#include <stdio.h>
#include <esp_attr.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include "esp_system.h"    // Incluir la cabecera del sistema para las funciones de frecuencia de la CPU

//static double tv[8]; // Declaración de un arreglo estático de doubles
const int N = 2097152; // 3200000; // Constante que define el número de operaciones a realizar, divisible en 16

//DRAM_ATTR static double tv_dram[8];


// Definición de una macro para la prueba de , IRAM_ATTR memoria de acceso rapido, RTC_SLOW_ATTR memoria de acceso lento 
//IRAM: La IRAM es SRAM que está optimizada para la ejecución rápida de instrucciones. El acceso a la IRAM es mucho más rápido que el acceso a la memoria flash externa.
//RTC_SLOW_ATTR: En el ESP32, RTC_SLOW_ATTR es un atributo que se usa para colocar variables en la memoria lenta del RTC (Real-Time Clock). La memoria RTC tiene la particularidad de ser accesible incluso cuando el resto del chip está en modo de bajo consumo, lo que la hace ideal para almacenar datos que deben persistir a través de ciclos de sueño profundo.
//DRAM_ATTR static double tv[8];
IRAM_ATTR static double tv[8];
//RTC_SLOW_ATTR static double tv[8];

#define TEST(type,name,ops) void name (void) {\
    type f0 = tv[0],f1 = tv[1],f2 = tv[2],f3 = tv[3];\
    type f4 = tv[4],f5 = tv[5],f6 = tv[6],f7 = tv[7];\
    for (int j = 0; j < N/128; j++) {\
        ops \
    }\
    tv[0] = f0;tv[1] = f1;tv[2] = f2;tv[3] = f3;\
    tv[4] = f4;tv[5] = f5;tv[6] = f6;tv[7] = f7;\
    }

// Definición de una macro para operaciones flotantes
#define fops(op1,op2) f0 op1##=f1 op2 f2;f1 op1##=f2 op2 f3;\
    f2 op1##=f3 op2 f4;f3 op1##=f4 op2 f5;\
    f4 op1##=f5 op2 f6;f5 op1##=f6 op2 f7;\
    f6 op1##=f7 op2 f0;f7 op1##=f0 op2 f1;

// Definición de macros para operaciones específicas
#define addops fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) fops(,+) // hace dos veces el llamado, 16 veces para que sea cosnistente con MACs
#define divops fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/) fops(,/)
#define mulops fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*) fops(,*)
#define muladdops fops(+,*) fops(+,*) fops(+,*) fops(+,*) fops(+,*) fops(+,*) fops(+,*) fops(+,*)

// Implementación de funciones de prueba para diferentes tipos de datos y operaciones
TEST(int,mulint,mulops)
TEST(float,mulfloat,mulops)
TEST(double,muldouble,mulops)
TEST(int,addint,addops)
TEST(float,addfloat,addops)
TEST(double,adddouble,addops)
TEST(int,divint,divops)
TEST(float,divfloat,divops)
TEST(double,divdouble,divops)
TEST(int,muladdint,muladdops)
TEST(float,muladdfloat,muladdops)
TEST(double,muladddouble,muladdops)

// Función para medir el tiempo de ejecución de una función de prueba
void timeit(char *name, void fn(void)) {
    vTaskDelay(1); // Espera un tiempo antes de iniciar la medición
    tv[0]=tv[1]=tv[2]=tv[3]=tv[4]=tv[5]=tv[6]=tv[7]=1; // Inicializa el arreglo tv
    // Obtener el tiempo desde el arranque en microsegundos
    float time = esp_timer_get_time();
    unsigned ccount, icount, ccount_new;
    RSR(CCOUNT, ccount); // Lee el contador de ciclos (RSR: Read special register)
    WSR(ICOUNT, 0); // Reinicia el contador de instrucciones (WSR: write special register)
    WSR(ICOUNTLEVEL, 2); // Establece el nivel de interrupción
    fn(); // Ejecuta la función de prueba
    RSR(CCOUNT, ccount_new); // Lee el nuevo valor del contador de ciclos
    RSR(ICOUNT, icount); // Lee el contador de instrucciones
    time = esp_timer_get_time() - time; // Calcula el tiempo transcurrido

     unsigned cpu_freq_mhz = 160; // Frecuencia de la CPU en MHz conocida de sdkconfig
     unsigned cycles = ccount_new - ccount;
     float cpi = (float)(cycles)/(float)(icount); // Calcula los ciclos por instrucción
     float time_cy = (float)(cycles) / (float)(cpu_freq_mhz); // Tiempo calculado del numero de ciclos

     // Imprime los resultados
     printf("%s \t Time_meas=%f ms \t%f MOP/S  \tCPI=%f  \tFreq=%u MHz \tCycles=%u \ticount=%u \tTime_cy=%f ms\n", name, time / 1000, (float)N/time, cpi, cpu_freq_mhz, cycles, icount, time_cy/1000);
}

// Función principal de la aplicación
void app_main() {
    // Asegurar que el watchdog está activo
    esp_task_wdt_add(NULL); // Registrar la tarea actual con el watchdog

    // Mide el tiempo de ejecución de varias operaciones aritméticas para diferentes tipos de datos
    timeit("Integer Addition", addint);
    timeit("Integer Multiply", mulint);
    timeit("Integer Division", divint);
    timeit("Integer Multiply-Add", muladdint);
    timeit("Float Addition ", addfloat);
    timeit("Float Multiply ", mulfloat);
    timeit("Float Division ", divfloat);
    timeit("Float Multiply-Add", muladdfloat);
    timeit("Double Addition", adddouble);
    timeit("Double Multiply", muldouble);
    timeit("Double Multiply-Add", muladddouble);

    esp_task_wdt_delete(NULL); // Eliminar la tarea actual del watchdog
}