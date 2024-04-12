/*
 * Copyright (c) 2018 Jan Pham.  All rights reserved.
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_DPCP
#define _RTEMS_SCORE_DPCP

#include <rtems/score/cpuopts.h>

#if defined(RTEMS_SMP)

#include <rtems/score/threadq.h>
#include <rtems/score/percpu.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief DPCP control block.
 */
typedef struct {
  /**
   * @brief The thread queue to manage ownership and waiting threads.
   */
  Thread_queue_Control Wait_queue;

  /**
  * @brief User-defined sychronization cpu, where the thread migrates to
  */
  Per_CPU_Control *cpu;


  /**
   * @brief The ceiling priority used by the owner thread.
   */
  Priority_Node Ceiling_priority;


} DPCP_Control;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_DPCP */
