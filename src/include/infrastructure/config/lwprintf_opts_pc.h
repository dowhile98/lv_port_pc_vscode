/**
 * @file lwprintf_opts_pc.h
 * @brief LwPRINTF configuration for PC simulator (no RTOS mutex)
 */
#ifndef LWPRINTF_OPTS_HDR_H
#define LWPRINTF_OPTS_HDR_H

/* Disable RTOS mutex (not needed for single-threaded PC printf) */
#define LWPRINTF_CFG_OS 0

/* Feature Support */
#define LWPRINTF_CFG_SUPPORT_LONG_LONG  1
#define LWPRINTF_CFG_SUPPORT_TYPE_FLOAT 1

#endif /* LWPRINTF_OPTS_HDR_H */
