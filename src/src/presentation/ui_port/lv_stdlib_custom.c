/*
 * lv_stdlib_custom.c
 *
 *  Created on: 24 feb 2026
 *      Author: tecna-smart-lab
 */


/*
 * lv_stdlib_custom.c
 *
 *  Created on: Aug 18, 2025
 *      Author: tecna-smart-lab
 */

#include "src/stdlib/lv_mem.h"
#include "osal/osal.h"
#include "src/stdlib/lv_sprintf.h"
#include "lwprintf.h"


#if (LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM)
void lv_mem_init(void)
{

}

void * lv_malloc_core(size_t size)
{
	void *ptr = NULL;
	Result_t res = os_alloc(size, &ptr);

	if(res != ERR_OK)
	{
		return NULL;
	}

	return ptr;
}

void * lv_realloc_core(void * p, size_t new_size)
{
	return os_realloc(p, new_size);
}

void lv_free_core(void * p)
{
	(void)os_free(p);
}

#endif


#if (LV_USE_STDLIB_SPRINTF == LV_STDLIB_CUSTOM)
int lv_snprintf(char * buffer, size_t count, const char * format, ...)
{
    va_list va;
    va_start(va, format);
    const int ret = lwprintf_vsnprintf(buffer, count, format, va);
    va_end(va);
    return ret;
}

int lv_vsnprintf(char * buffer, size_t count, const char * format, va_list va)
{
    return lwprintf_vsnprintf( buffer, count, format, va);
}
#endif
