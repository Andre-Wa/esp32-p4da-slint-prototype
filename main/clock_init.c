/**
 * clock_init.c
 *
 * Ajuste manual de hora via settimeofday() e formatação pra exibir na
 * UI. Sem NTP (bloqueado até o firmware do C6 ser atualizado) e sem RTC
 * de hardware confirmado nessa placa, a hora do sistema só existe
 * enquanto a placa está ligada continuamente — some no próximo boot sem
 * energia. Esse módulo é o ajuste manual, útil enquanto isso (e depois
 * também, pra corrigir fuso/hora na mão).
 *
 * NOTA: sem seleção de fuso horário ainda — fica implícito UTC. Se você
 * digitar o horário local daqui, o relógio vai mostrar certo, mas
 * qualquer coisa que compare com hora "real" (ex: um NTP futuro) vai
 * assumir UTC até isso ser resolvido.
 */

#include "clock_init.h"

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

void board_clock_set(int year, int month, int day, int hour, int minute, int second)
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;

    time_t epoch = mktime(&t);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
}

void board_clock_get_string(char *buf, size_t len)
{
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buf, len, "%d/%m/%Y %H:%M:%S", &timeinfo);
}
