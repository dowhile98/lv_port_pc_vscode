/*
 * system_init.h
 *
 *  Created on: Jan 30, 2026
 *      Author: tecna-smart-lab
 */

#ifndef INFRASTRUCTURE_STARTUP_SYSTEM_INIT_H_
#define INFRASTRUCTURE_STARTUP_SYSTEM_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "hal_types.h"


/**
 * @brief Initialize all system components with dependency injection.
 * @note Called from main() before vTaskStartScheduler().
 */
Result_t System_Init(void);


#ifdef __cplusplus
}
#endif

#endif /* INFRASTRUCTURE_STARTUP_SYSTEM_INIT_H_ */
