/**
 * @file pc_event_log_storage.h
 * @brief In-memory IEventLogStorage for the PC simulator.
 *
 * Pre-populated with sample events so the Historical Events screen shows
 * meaningful data on first launch.  Supports Append, ReadByIndex, GetCount,
 * GetMetadata and Clear.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef PC_EVENT_LOG_STORAGE_H
#define PC_EVENT_LOG_STORAGE_H

#include "interfaces/i_event_log_storage.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Returns the singleton IEventLogStorage interface.
     *
     * Pre-populated with sample events on first call.
     *
     * @return Pointer to IEventLogStorage (never NULL).
     */
    IEventLogStorage *PCEventLogStorage_GetInstance(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_EVENT_LOG_STORAGE_H */
