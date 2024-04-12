#ifndef SOURCE_MPCP_H
#define SOURCE_MPCP_H

#if defined(RTEMS_SMP)

#include <rtems/score/threadq.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup ScoreMPCP Multiprocessor Resource Sharing Protocol Handler
 *
 * @ingroup Score
 *
 * @brief Multiprocessor Resource Sharing Protocol (MPCP).
 *
 *
 * @{
 */

/**
 * @brief MPCP control block.
 */
typedef struct {
  /**
   * @brief The thread queue to manage ownership and waiting threads.
   */
  Thread_queue_Control Wait_queue;

  /**
   * @brief The ceiling priority used by the owner thread.
   */
  Priority_Node Ceiling_priority;

  /**
   * @brief One ceiling priority per scheduler instance.
   */
  Priority_Control *ceiling_priorities;
} MPCP_Control;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif //SOURCE_MPCP_H