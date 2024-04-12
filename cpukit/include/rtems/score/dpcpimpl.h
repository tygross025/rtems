#ifndef _RTEMS_SCORE_DPCPIMPL_H
#define _RTEMS_SCORE_DPCPIMPL_H

#include <rtems/score/dpcp.h>

#if defined(RTEMS_SMP)

#include <rtems/score/assert.h>
#include <rtems/score/status.h>
#include <rtems/score/threadqimpl.h>
#include <rtems/score/watchdogimpl.h>
#include <rtems/score/wkspace.h>
#include <rtems/score/scheduler.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup ScoreDPCP
 *
 * @{
 */
#define DPCP_TQ_OPERATIONS &_Thread_queue_Operations_priority

/**
 * @brief Migrates Thread to an synchronization processor.
 *
 * @param executing The executing Thread.
 * @param dpcp The semaphore control block.
 */
RTEMS_INLINE_ROUTINE void _DPCP_Migrate(
  Thread_Control *executing,
  DPCP_Control   *dpcp
)
{
  _Scheduler_Migrate_To( executing, dpcp->cpu, &( dpcp->Ceiling_priority ) );
}

/**
 * @brief Migrates the task back to the application processor.
 *
 * @param executing The executing Thread.
 * @param dpcp The semaphore control block.
 */
RTEMS_INLINE_ROUTINE void _DPCP_Migrate_Back(
  Thread_Control *executing,
  DPCP_Control   *dpcp
)
{
  _Scheduler_Migrate_Back( executing,dpcp->cpu );
}

/**
 * @brief Acquires critical according to DPCP.
 *
 * @param dpcp The DPCP control for the operation.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _DPCP_Acquire_critical(
  DPCP_Control         *dpcp,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Acquire_critical( &dpcp->Wait_queue, queue_context );
}

/**
 * @brief Releases according to DPCP.
 *
 * @param dpcp The DPCP control for the operation.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _DPCP_Release(
  DPCP_Control         *dpcp,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Release( &dpcp->Wait_queue, queue_context );
}

/**
 * @brief Gets owner of the DPCP control.
 *
 * @param dpcp The DPCP control to get the owner from.
 *
 * @return The owner of the dpcp control.
 */
RTEMS_INLINE_ROUTINE Thread_Control *_DPCP_Get_owner(
  const DPCP_Control *dpcp
)
{
  return dpcp->Wait_queue.Queue.owner;
}

/**
 * @brief Sets owner of the DPCP control.
 *
 * @param[out] dpcp The DPCP control to set the owner of.
 * @param owner The desired new owner for @a dpcp.
 */
RTEMS_INLINE_ROUTINE void _DPCP_Set_owner(
  DPCP_Control   *dpcp,
  Thread_Control *owner
)
{
  dpcp->Wait_queue.Queue.owner = owner;
}

/**
 * @brief Gets ceiling priority of the DPCP control.
 *
 * @param dpcp The dpcp to get the priority from.
 *
 * @return The priority of the DPCP control.
 */
RTEMS_INLINE_ROUTINE Priority_Control _DPCP_Get_priority(
  const DPCP_Control *dpcp
)
{
  return dpcp->Ceiling_priority.priority;
}


/**
 * @brief Sets the ceiling priority of the DPCP control
 *
 * @param[out] dpcp The DPCP control to set the priority of.
 * @param priority_ceiling The new priority for the DPCP Control
 * @param queue_context The Thread queue context
 */
RTEMS_INLINE_ROUTINE void _DPCP_Set_priority(
  DPCP_Control         *dpcp,
  Priority_Control      priority_ceiling,
  Thread_queue_Context *queue_context
)
{
  Thread_Control *owner;
  owner = _DPCP_Get_owner( dpcp );
  if ( owner != NULL ) {
   //Do nothing, thread executing right now
  } else {
    dpcp->Ceiling_priority.priority = priority_ceiling;
  }
}

/**
 * @brief Gets the synchronization CPU of the DPCP Control, where the task migrates to.
 *
 * @retval The Per_CPU_Control control block
 */
RTEMS_INLINE_ROUTINE Per_CPU_Control *_DPCP_Get_CPU(
  DPCP_Control *dpcp
)
{
  return dpcp->cpu;
}

/**
 * @brief Sets the synchronization CPU of the DPCP Control.
 *
 * @param dpcp The semaphore control block
 * @param cpu The synchronization processor it changes to.
 * @param queue_context struct to secure sempahore access
 */
RTEMS_INLINE_ROUTINE void _DPCP_Set_CPU(
  DPCP_Control         *dpcp,
  Per_CPU_Control      *cpu,
  Thread_queue_Context *queue_context
)
{
  _DPCP_Acquire_critical( dpcp, queue_context );
  dpcp->cpu = cpu;
  _DPCP_Release( dpcp, queue_context );
}

/**
 * @brief Claims ownership of the DPCP control.
 *
 * @param dpcp The DPCP control to claim the ownership of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the executing
 *      thread exceeds the ceiling priority.
 */
RTEMS_INLINE_ROUTINE Status_Control _DPCP_Claim_ownership(
  DPCP_Control         *dpcp,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  ISR_lock_Context  lock_context;
  Scheduler_Node   *scheduler_node;
  Per_CPU_Control  *cpu_self;

  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  scheduler_node = _Thread_Scheduler_get_home_node( executing );

  if ( _Priority_Get_priority( &scheduler_node->Wait.Priority ) <
      dpcp->Ceiling_priority.priority ) {
      _Thread_Wait_release_default_critical( executing, &lock_context );
      _DPCP_Release( dpcp, queue_context );
      return STATUS_MUTEX_CEILING_VIOLATED;
  }

  _DPCP_Set_owner( dpcp, executing );
  cpu_self = _Thread_queue_Dispatch_disable( queue_context );
  _DPCP_Release( dpcp, queue_context );
  _DPCP_Migrate( executing, dpcp );
  _Thread_Wait_release_default_critical( executing, &lock_context );
  _Thread_Dispatch_enable( cpu_self );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Initializes a DPCP control.
 *
 * @param[out] dpcp The DPCP control that is initialized.
 * @param scheduler The scheduler for the operation.
 * @param ceiling_priority
 * @param executing The currently executing thread.  Ignored in this method.
 * @param initially_locked Indicates whether the DPCP control shall be initally
 *      locked. If it is initially locked, this method returns STATUS_INVALID_NUMBER.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_INVALID_NUMBER The DPCP control is initially locked.
 */
RTEMS_INLINE_ROUTINE Status_Control _DPCP_Initialize(
  DPCP_Control            *dpcp,
  const Scheduler_Control *scheduler,
  Priority_Control         ceiling_priority,
  Thread_Control          *executing,
  bool                     initially_locked
)
{
  if ( initially_locked ) {
    return STATUS_INVALID_NUMBER;
  }

  dpcp->cpu = _Per_CPU_Get_by_index( 1 );
  _Priority_Node_initialize( &dpcp->Ceiling_priority, ceiling_priority );
  _Thread_queue_Object_initialize( &dpcp->Wait_queue );
  return STATUS_SUCCESSFUL;
}
/**
 * @brief Waits for the ownership of the DPCP control.
 *
 * @param dpcp The DPCP control to get the ownership of.
 * @param executing The currently executing thread.
 * @param queue_context the thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_DEADLOCK A deadlock occured.
 * @retval STATUS_TIMEOUT A timeout occured.
 */
RTEMS_INLINE_ROUTINE Status_Control _DPCP_Wait_for_ownership(
  DPCP_Control         *dpcp,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Context_set_thread_state(
    queue_context,
    STATES_WAITING_FOR_MUTEX
  );
  _Thread_queue_Context_set_deadlock_callout(
    queue_context,
    _Thread_queue_Deadlock_status
  );
  _Thread_queue_Enqueue(
    &dpcp->Wait_queue.Queue,
    DPCP_TQ_OPERATIONS,
    executing,
    queue_context
  );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Seizes the DPCP control.
 *
 * @param[in, out] dpcp The DPCP control to seize the control of.
 * @param[in, out] executing The currently executing thread.
 * @param wait Indicates whether the calling thread is willing to wait.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the executing
 *      thread exceeds the ceiling priority.
 * @retval STATUS_UNAVAILABLE The executing thread is already the owner of
 *      the DPCP control.  Seizing it is not possible.
 */
RTEMS_INLINE_ROUTINE Status_Control _DPCP_Seize(
  DPCP_Control         *dpcp,
  Thread_Control       *executing,
  bool                  wait,
  Thread_queue_Context *queue_context
)
{
  Status_Control  status;
  Thread_Control *owner;

  _DPCP_Acquire_critical( dpcp, queue_context );

  owner = _DPCP_Get_owner( dpcp );

  if ( owner == NULL ) {
    status = _DPCP_Claim_ownership( dpcp, executing, queue_context );
  } else if ( owner == executing ) {
    _DPCP_Release( dpcp, queue_context );
    status = STATUS_UNAVAILABLE;
  } else if ( wait ) {
    status = _DPCP_Wait_for_ownership( dpcp, executing, queue_context );
  } else {
    _DPCP_Release( dpcp, queue_context );
    status = STATUS_UNAVAILABLE;
  }

  return status;
}

/**
 * @brief Surrenders the DPCP control.
 *
 * @param[in, out] dpcp The DPCP control to surrender the control of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_NOT_OWNER The executing thread does not own the DPCP control.
 */
RTEMS_INLINE_ROUTINE Status_Control _DPCP_Surrender(
  DPCP_Control         *dpcp,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  ISR_lock_Context  lock_context;
  Per_CPU_Control  *cpu_self;
  Thread_Control   *new_owner;

  _DPCP_Acquire_critical( dpcp, queue_context );
  cpu_self = _Thread_Dispatch_disable_critical( &queue_context->Lock_context.Lock_context );
  if (_DPCP_Get_owner( dpcp ) != executing) {
    _DPCP_Release( dpcp , queue_context );
    _Thread_Dispatch_enable( cpu_self );
    return STATUS_NOT_OWNER;
  }

  _Thread_queue_Context_clear_priority_updates( queue_context );
  new_owner = _Thread_queue_First_locked(
                &dpcp->Wait_queue,
	        DPCP_TQ_OPERATIONS
	      );
  _DPCP_Set_owner( dpcp, new_owner );

  if ( new_owner != NULL ) {
  #if defined(RTEMS_MULTIPROCESSING)
    if ( _Objects_Is_local_id( new_owner->Object.id ) )
  #endif
    {
    }
    _Thread_queue_Extract_critical(
      &dpcp->Wait_queue.Queue,
      CORE_MUTEX_TQ_OPERATIONS,
      new_owner,
      queue_context
    );
    _Thread_Wait_acquire_default_critical( new_owner, &lock_context );
    _DPCP_Migrate(new_owner, dpcp);
    _Thread_Wait_release_default_critical( new_owner, &lock_context );
  } else {
    _DPCP_Release( dpcp, queue_context );
  }

  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  _DPCP_Migrate_Back( executing, dpcp );
  _Thread_Wait_release_default_critical( executing, &lock_context );
  _Thread_Dispatch_enable( cpu_self );

  return STATUS_SUCCESSFUL;
}

/**
 * @brief Checks if the DPCP control can be destroyed.
 *
 * @param dpcp The DPCP control for the operation.
 *
 * @retval STATUS_SUCCESSFUL The DPCP is currently not used
 *      and can be destroyed.
 * @retval STATUS_RESOURCE_IN_USE The DPCP control is in use,
 *      it cannot be destroyed.
 */
RTEMS_INLINE_ROUTINE Status_Control _DPCP_Can_destroy(
  DPCP_Control *dpcp
)
{
  if ( _DPCP_Get_owner( dpcp ) != NULL ) {
    return STATUS_RESOURCE_IN_USE;
  }
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Destroys the DPCP control
 *
 * @param[in, out] The dpcp that is about to be destroyed.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _DPCP_Destroy(
  DPCP_Control         *dpcp,
  Thread_queue_Context *queue_context
)
{
  _DPCP_Release( dpcp, queue_context );
  _Thread_queue_Destroy( &dpcp->Wait_queue );
}
/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_DPCPIMPL_H */