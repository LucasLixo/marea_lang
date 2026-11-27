// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREATasksRuntime.h"

// INFO: the opaque struct of MAREA_TasksTask. Arena allocated, never freed on its own: the arena that made it
// owns it, the same lifetime rule as every other arena-backed value of this language.
struct MAREA_TasksTask
{
    MAREA_TasksTaskState state;
    MAREA_Null result;
};

MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskCreate(MAREA_Arena *arena, MAREA_TasksTask **outTask)
{
    if ((arena == MAREA_NULLPTR) || (outTask == MAREA_NULLPTR)) {
        return MAREA_TASKS_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, MAREA_INTEGER_SIZE(sizeof(MAREA_TasksTask)), MAREA_INTEGER_SIZE(8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_TASKS_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_TasksTask *const TASK = (MAREA_TasksTask *)memory;

    // INFO: the parentheses around the state name are kept (not clang-format noise to remove): the name
    // ends in "PENDING", which matches MacroBlockEnd's ".*END.*" in .clang-format (there for
    // MAREA_BEGIN_EXTERN_C/MAREA_END_EXTERN_C), so a bare name right before ';' is misread as a closing
    // macro block.
    TASK->state = (MAREA_TASKS_TASK_STATE_PENDING);
    TASK->result = MAREA_NULLPTR;

    *outTask = TASK;

    return MAREA_TASKS_EXCEPT_SUCCESS;
}

MAREA_TasksTaskState MAREA_CALL MAREA_TasksTaskGetState(const MAREA_TasksTask *task)
{
    return (task != MAREA_NULLPTR) ? task->state : (MAREA_TASKS_TASK_STATE_PENDING);
}

MAREA_Boolean MAREA_CALL MAREA_TasksTaskIsCompleted(const MAREA_TasksTask *task)
{
    return ((task != MAREA_NULLPTR) && (task->state == MAREA_TASKS_TASK_STATE_COMPLETED)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskStart(MAREA_TasksTask *task)
{
    if (task == MAREA_NULLPTR) {
        return MAREA_TASKS_EXCEPT_ERROR_NULL_POINTER;
    }

    if (task->state != MAREA_TASKS_TASK_STATE_PENDING) {
        return MAREA_TASKS_EXCEPT_ERROR_INVALID_STATE;
    }

    task->state = MAREA_TASKS_TASK_STATE_RUNNING;

    return MAREA_TASKS_EXCEPT_SUCCESS;
}

MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskComplete(MAREA_TasksTask *task, MAREA_Null result)
{
    if (task == MAREA_NULLPTR) {
        return MAREA_TASKS_EXCEPT_ERROR_NULL_POINTER;
    }

    if (task->state != MAREA_TASKS_TASK_STATE_RUNNING) {
        return MAREA_TASKS_EXCEPT_ERROR_INVALID_STATE;
    }

    task->state = MAREA_TASKS_TASK_STATE_COMPLETED;
    task->result = result;

    return MAREA_TASKS_EXCEPT_SUCCESS;
}

MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskCancel(MAREA_TasksTask *task)
{
    if (task == MAREA_NULLPTR) {
        return MAREA_TASKS_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((task->state != MAREA_TASKS_TASK_STATE_PENDING) && (task->state != MAREA_TASKS_TASK_STATE_RUNNING)) {
        return MAREA_TASKS_EXCEPT_ERROR_INVALID_STATE;
    }

    task->state = MAREA_TASKS_TASK_STATE_CANCELLED;

    return MAREA_TASKS_EXCEPT_SUCCESS;
}

MAREA_TasksExcept MAREA_CALL MAREA_TasksTaskGetResult(MAREA_HostContext *context, const MAREA_TasksTask *task, MAREA_Null *outResult)
{
    if ((context == MAREA_NULLPTR) || (task == MAREA_NULLPTR) || (outResult == MAREA_NULLPTR)) {
        return MAREA_TASKS_EXCEPT_ERROR_NULL_POINTER;
    }

    if (task->state != MAREA_TASKS_TASK_STATE_COMPLETED) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_TASK_NOT_COMPLETED);

        return MAREA_TASKS_EXCEPT_ERROR_TASK_NOT_COMPLETED;
    }

    *outResult = task->result;

    return MAREA_TASKS_EXCEPT_SUCCESS;
}
