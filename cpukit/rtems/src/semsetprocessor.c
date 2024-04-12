/**
 * @file
 *
 * @brief RTEMS Semaphore Release
 * @ingroup ClassicSem Semaphores
 *
 * This file contains the implementation of the Classic API directive
 * rtems_semaphore_release().
 */

/*
 *  COPYRIGHT (c) 1989-2014.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/rtems/semimpl.h>
#include <rtems/rtems/statusimpl.h>

rtems_status_code rtems_semaphore_set_processor(
  rtems_id id,
  int      cpu
)
{
  Semaphore_Control   *the_semaphore;
  Thread_queue_Context queue_context;
  ISR_lock_Context     lock_context;
  Status_Control       status;

  the_semaphore = _Semaphore_Get( id, &queue_context );

  if ( the_semaphore == NULL ) {
#if defined(RTEMS_MULTIPROCESSING)
    return _Semaphore_MP_Release( id );
#else
    return RTEMS_INVALID_ID;
#endif
  }

  _Thread_queue_Context_set_MP_callout(
    &queue_context,
    _Semaphore_Core_mutex_mp_support
  );
  switch ( the_semaphore->variant ) {
    case SEMAPHORE_VARIANT_MUTEX_INHERIT_PRIORITY:
      status =  RTEMS_NOT_DEFINED;
      break;
    case SEMAPHORE_VARIANT_MUTEX_PRIORITY_CEILING:
      status = RTEMS_NOT_DEFINED;
      break;
    case SEMAPHORE_VARIANT_MUTEX_NO_PROTOCOL:
      status = RTEMS_NOT_DEFINED;
      break;
    case SEMAPHORE_VARIANT_SIMPLE_BINARY:
      status = RTEMS_NOT_DEFINED;
      break;
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_MRSP:
      status = RTEMS_NOT_DEFINED;
      break;
    case SEMAPHORE_VARIANT_DPCP:
     _DPCP_Set_CPU(
       &the_semaphore->Core_control.DPCP,
       _Per_CPU_Get_by_index(cpu),
       &queue_context
     );
     status = STATUS_SUCCESSFUL;
     break;
#endif
    default:
      _Assert( the_semaphore->variant == SEMAPHORE_VARIANT_COUNTING );
      status = RTEMS_NOT_DEFINED;
      break;
  }

  return _Status_Get( status );
}
