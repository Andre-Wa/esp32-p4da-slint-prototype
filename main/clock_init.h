#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ajuste manual de data/hora (sem NTP ainda — a data zera a cada boot
 * sem energia contínua, ver README seção RTC/bateria). Continua útil
 * mesmo depois do NTP funcionar, pra corrigir fuso/hora manualmente.
 */
void board_clock_set(int year, int month, int day, int hour, int minute, int second);

/** Formata a hora atual em buf, como "DD/MM/AAAA HH:MM:SS". */
void board_clock_get_string(char *buf, size_t len);

#ifdef __cplusplus
}
#endif
