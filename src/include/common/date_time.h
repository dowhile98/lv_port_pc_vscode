#ifndef DATE_TIME_H
#define DATE_TIME_H


#include <stdint.h>

/* C/C++ portable static assertion: _Static_assert is C11, static_assert is C++ */
#if defined(__cplusplus) && !defined(_Static_assert)
#define _Static_assert static_assert
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Fecha y hora en formato UTC.
 * @note PACKED para garantizar layout determinista en EEPROM.
 */
typedef struct __attribute__((packed))
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekDay;
} DateTime_t;

/* Compile-time size validation */
//_Static_assert(sizeof(DateTime_t) == 8, "DateTime_t must be exactly 8 bytes");

/**
 * @brief Añade segundos a una fecha/hora (ACTION-007).
 * @param dt Puntero a DateTime_t (modificado in-place).
 * @param seconds Segundos a añadir (puede ser negativo).
 * @note Maneja overflow/underflow de horas, días, meses.
 */
void DateTime_AddSeconds(DateTime_t *dt, int32_t seconds);

/**
 * @brief Convierte DateTime_t a Unix timestamp (segundos desde 1970-01-01 00:00:00 UTC).
 * @param dt Puntero a DateTime_t (debe estar en UTC).
 * @return uint32_t Unix timestamp (0 si dt es NULL o inválido).
 * @note Válido para fechas entre 1970-2106 (límite uint32_t).
 */
uint32_t DateTime_ToUnix(const DateTime_t *dt);

/**
 * @brief Convierte Unix timestamp a DateTime_t.
 * @param timestamp Unix timestamp (segundos desde 1970-01-01 00:00:00 UTC).
 * @param out_dt Puntero a DateTime_t donde se almacenará el resultado.
 * @note Válido para timestamps hasta 2106 (límite uint32_t).
 */
void DateTime_FromUnix(uint32_t timestamp, DateTime_t *out_dt);

#ifdef __cplusplus
}
#endif
#endif /* DATE_TIME_H */
