// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

// Adapted from this project's own reference material on a task's state machine (Pending, Running,
// Completed, Cancelled - no Failed state) and the shape a future-like handle gives: a state query,
// a completion query, and a result that is a defined trap before completion rather than a value
// pulled out early.

#include "tests/dependencies.hpp"

#include "marea/base/MAREATasksRuntime.h"
#include "marea/base/MAREAHostContext.h"

TEST(MAREATasksRuntime, ANewTaskStartsPending)
{
    GTEST_COUT << "Validating a freshly created task starts in the Pending state." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS);

    MAREA_TasksTask *task{ nullptr };

    ASSERT_EQ(MAREA_TasksTaskCreate(arena, &task), MAREA_TASKS_EXCEPT_SUCCESS) << "The task must be created.";

    EXPECT_EQ(MAREA_TasksTaskGetState(task), MAREA_TASKS_TASK_STATE_PENDING) << "A task that never ran is Pending.";

    EXPECT_EQ(MAREA_TasksTaskIsCompleted(task), MAREA_BOOLEAN_FALSE) << "Pending is not completed.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREATasksRuntime, StartMovesPendingToRunning)
{
    GTEST_COUT << "Validating starting a pending task moves it to Running." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS);

    MAREA_TasksTask *task{ nullptr };

    ASSERT_EQ(MAREA_TasksTaskCreate(arena, &task), MAREA_TASKS_EXCEPT_SUCCESS);

    ASSERT_EQ(MAREA_TasksTaskStart(task), MAREA_TASKS_EXCEPT_SUCCESS) << "Starting a Pending task must succeed.";

    EXPECT_EQ(MAREA_TasksTaskGetState(task), MAREA_TASKS_TASK_STATE_RUNNING) << "The task is now Running.";

    EXPECT_EQ(MAREA_TasksTaskIsCompleted(task), MAREA_BOOLEAN_FALSE) << "Running is not completed.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREATasksRuntime, CompleteGivesTheResultBack)
{
    GTEST_COUT << "Validating a completed task is_completed, and its result is the value it was given." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS);

    MAREA_TasksTask *task{ nullptr };

    ASSERT_EQ(MAREA_TasksTaskCreate(arena, &task), MAREA_TASKS_EXCEPT_SUCCESS);

    ASSERT_EQ(MAREA_TasksTaskStart(task), MAREA_TASKS_EXCEPT_SUCCESS);

    MAREA_Sinteger32 value{ 7 };

    ASSERT_EQ(MAREA_TasksTaskComplete(task, &value), MAREA_TASKS_EXCEPT_SUCCESS) << "Completing a Running task must succeed.";

    EXPECT_EQ(MAREA_TasksTaskGetState(task), MAREA_TASKS_TASK_STATE_COMPLETED) << "The task is now Completed.";

    EXPECT_EQ(MAREA_TasksTaskIsCompleted(task), MAREA_BOOLEAN_TRUE) << "Completed is completed.";

    MAREA_HostContext context{};

    void *result{ nullptr };

    ASSERT_EQ(MAREA_TasksTaskGetResult(&context, task, &result), MAREA_TASKS_EXCEPT_SUCCESS) << "The result must be readable now.";

    EXPECT_EQ(*static_cast<MAREA_Sinteger32 *>(result), 7) << "The result is the value that was given to complete.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREATasksRuntime, ResultBeforeCompletionTrapsAsTaskNotCompleted)
{
    GTEST_COUT << "Validating result() of a task that is not yet Completed raises the defined trap, never an "
                  "undefined read."
               << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS);

    MAREA_TasksTask *task{ nullptr };

    ASSERT_EQ(MAREA_TasksTaskCreate(arena, &task), MAREA_TASKS_EXCEPT_SUCCESS);

    MAREA_HostContext context{};

    void *result{ nullptr };

    EXPECT_EQ(MAREA_TasksTaskGetResult(&context, task, &result), MAREA_TASKS_EXCEPT_ERROR_TASK_NOT_COMPLETED) << "Pending has no result yet.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_TASK_NOT_COMPLETED) << "The context carries the defined trap.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREATasksRuntime, CancelMovesPendingOrRunningToCancelledAndNeverToFailed)
{
    GTEST_COUT << "Validating cancel moves a task to Cancelled, the only non-Completed terminal state - there is no "
                  "Failed state."
               << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS);

    MAREA_TasksTask *task{ nullptr };

    ASSERT_EQ(MAREA_TasksTaskCreate(arena, &task), MAREA_TASKS_EXCEPT_SUCCESS);

    ASSERT_EQ(MAREA_TasksTaskCancel(task), MAREA_TASKS_EXCEPT_SUCCESS) << "Cancelling a Pending task must succeed.";

    EXPECT_EQ(MAREA_TasksTaskGetState(task), MAREA_TASKS_TASK_STATE_CANCELLED) << "The task is now Cancelled.";

    EXPECT_EQ(MAREA_TasksTaskIsCompleted(task), MAREA_BOOLEAN_FALSE) << "Cancelled is not completed.";

    MAREA_HostContext context{};

    void *result{ nullptr };

    EXPECT_EQ(MAREA_TasksTaskGetResult(&context, task, &result), MAREA_TASKS_EXCEPT_ERROR_TASK_NOT_COMPLETED) << "A cancelled task still traps on result(), it never fabricates one.";

    MAREA_ArenaDestroy(arena);
}
