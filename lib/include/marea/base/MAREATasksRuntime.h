// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREATASKSRUNTIME_H)
    #define MAREA_INCLUDE_BASE_MAREATASKSRUNTIME_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREAHostContext.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The runtime of marea.tasks: the state machine of a spawned call's handle. The pool and the
     * scheduling are a sequential stand-in, matching fork_join's own sequential lowering in the vm - nothing
     * here ever runs in parallel; a task is driven by hand through start, complete and cancel below.
     */

    /**
     * @brief The state of a task: Pending (never started), Running (started, not finished), Completed (ended
     * with a result) or Cancelled (ended with no result, the only non-Completed terminal state - there is no
     * Failed state).
     */
    typedef enum MAREA_TasksTaskState
    {
        MAREA_TASKS_TASK_STATE_PENDING = MAREA_UINTEGER_8(0),
        MAREA_TASKS_TASK_STATE_RUNNING = MAREA_UINTEGER_8(1),
        MAREA_TASKS_TASK_STATE_COMPLETED = MAREA_UINTEGER_8(2),
        MAREA_TASKS_TASK_STATE_CANCELLED = MAREA_UINTEGER_8(3),
    } MAREA_TasksTaskState;

    /**
     * @brief Result of a marea.tasks operation.
     */
    typedef enum MAREA_TasksExcept
    {
        MAREA_TASKS_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_TASKS_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_TASKS_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_TASKS_EXCEPT_ERROR_TASK_NOT_COMPLETED = MAREA_UINTEGER_8(3),
        MAREA_TASKS_EXCEPT_ERROR_INVALID_STATE = MAREA_UINTEGER_8(4),
    } MAREA_TasksExcept;

    /**
     * @brief Opaque handle of marea.tasks.Task<T>: the state of one spawned call and its result once
     * Completed. The caller owns the arena it was created from; the task itself is never freed on its own.
     */
    typedef struct MAREA_TasksTask MAREA_TasksTask;

    /**
     * @brief Creates a new task in the Pending state, with no result.
     * @param arena Arena to allocate the task from. Must not be null.
     * @param outTask Receives the new task. Must not be null.
     * @return MAREA_TASKS_EXCEPT_SUCCESS, ERROR_NULL_POINTER when arena or outTask is null, or
     * ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskCreate(MAREA_Arena * arena, MAREA_TasksTask * *outTask);

    /**
     * @brief Reads the state of a task.
     * @param task Task to query. Null reads as Pending.
     * @return The state.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TasksTaskState MAREA_CALL MAREA_TasksTaskGetState(const MAREA_TasksTask *task);

    /**
     * @brief The completion query: whether a task is in the Completed state.
     * @param task Task to query. Null reads as not completed.
     * @return MAREA_BOOLEAN_TRUE only in the Completed state.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_TasksTaskIsCompleted(const MAREA_TasksTask *task);

    /**
     * @brief Moves a Pending task to Running.
     * @param task Task to start. Must not be null.
     * @return MAREA_TASKS_EXCEPT_SUCCESS, ERROR_NULL_POINTER when task is null, or ERROR_INVALID_STATE when
     * task is not Pending.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskStart(MAREA_TasksTask * task);

    /**
     * @brief Moves a Running task to Completed and gives it its result.
     * @param task Task to complete. Must not be null.
     * @param result Result the task's call gave, kept by reference; the caller keeps it valid for as long as
     * the task might be read.
     * @return MAREA_TASKS_EXCEPT_SUCCESS, ERROR_NULL_POINTER when task is null, or ERROR_INVALID_STATE when
     * task is not Running.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskComplete(MAREA_TasksTask * task, MAREA_Null result);

    /**
     * @brief Moves a Pending or Running task to Cancelled, the terminal state of a task that never gives a
     * result - there is no Failed state.
     * @param task Task to cancel. Must not be null.
     * @return MAREA_TASKS_EXCEPT_SUCCESS, ERROR_NULL_POINTER when task is null, or ERROR_INVALID_STATE when
     * task is already Completed or Cancelled.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskCancel(MAREA_TasksTask * task);

    /**
     * @brief The result of a task: the defined trap, never an undefined read, when it is asked before the
     * task is Completed.
     * @param context Context of the call, which carries the trap. Must not be null.
     * @param task Task to read. Must not be null.
     * @param outResult Receives the result MAREA_TasksTaskComplete was given. Must not be null; read only on
     * success.
     * @return MAREA_TASKS_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context, task or outResult is null, or
     * ERROR_TASK_NOT_COMPLETED when task is not yet Completed, which also raises MAREA_HOST_TRAP_TASK_NOT_COMPLETED
     * on context.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskGetResult(MAREA_HostContext * context, const MAREA_TasksTask *task, MAREA_Null *outResult);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREATASKSRUNTIME_H */
