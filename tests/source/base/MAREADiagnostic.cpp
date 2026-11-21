#include "tests/dependencies.hpp"

#include "marea/base/MAREAArena.h"
#include "marea/base/MAREADiagnostic.h"

#include <cstring>
#include <type_traits>

namespace
{

    template <typename T, typename = void>
    struct MareaIsComplete : std::false_type
    {
    };

    template <typename T>
    struct MareaIsComplete<T, std::void_t<decltype(sizeof(T))>> : std::true_type
    {
    };

    MAREA_SourcePosition MakePosition(MAREA_Uinteger32 offset)
    {
        return MAREA_SourcePosition{ "test.marea", offset, MAREA_UINTEGER_32(1), MAREA_UINTEGER_32(1), MAREA_UINTEGER_32(1) };
    }

} // namespace

TEST(MAREADiagnostic, SinkCreateIsEmpty)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkCreate starts with an empty sink." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetCount(sink), MAREA_INTEGER_SIZE(0)) << "A fresh sink must have no diagnostics.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetErrorCount(sink), MAREA_INTEGER_SIZE(0)) << "A fresh sink must have no errors.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, SinkCreateRejectsNull)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkCreate rejects null arguments." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    EXPECT_EQ(MAREA_DiagnosticSinkCreate(nullptr, &sink), MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER) << "MAREA_DiagnosticSinkCreate must reject a null arena.";

    EXPECT_EQ(MAREA_DiagnosticSinkCreate(arena, nullptr), MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER) << "MAREA_DiagnosticSinkCreate must reject a null outSink.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, SinkAddStoresFields)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkAdd stores every field, read back with the getters." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    const MAREA_SourcePosition position{ MakePosition(MAREA_UINTEGER_32(42)) };

    ASSERT_EQ(MAREA_DiagnosticSinkAdd(sink, MAREA_UINTEGER_32(1001), MAREA_DIAGNOSTIC_SEVERITY_WARNING, position, "a message"), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkAdd must succeed.";

    ASSERT_EQ(MAREA_DiagnosticSinkGetCount(sink), MAREA_INTEGER_SIZE(1)) << "The sink must hold exactly one diagnostic.";

    const MAREA_Diagnostic *diagnostic{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkGetAt(sink, MAREA_INTEGER_SIZE(0), &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkGetAt must succeed for index 0.";

    EXPECT_EQ(MAREA_DiagnosticGetCode(diagnostic), MAREA_UINTEGER_32(1001)) << "The stored code must round-trip.";

    EXPECT_EQ(MAREA_DiagnosticGetSeverity(diagnostic), MAREA_DIAGNOSTIC_SEVERITY_WARNING) << "The stored severity must round-trip.";

    EXPECT_EQ(MAREA_DiagnosticGetPosition(diagnostic).offset, MAREA_UINTEGER_32(42)) << "The stored position must round-trip.";

    EXPECT_STREQ(MAREA_DiagnosticGetMessage(diagnostic), "a message") << "The stored message must round-trip.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, SinkAddCopiesMessage)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkAdd copies the message instead of keeping the caller's pointer." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    char buffer[]{ "mutable message" };

    ASSERT_EQ(MAREA_DiagnosticSinkAdd(sink, MAREA_UINTEGER_32(1), MAREA_DIAGNOSTIC_SEVERITY_ERROR, MakePosition(MAREA_UINTEGER_32(0)), buffer), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkAdd must succeed.";

    std::memset(buffer, 'x', sizeof(buffer) - 1);

    const MAREA_Diagnostic *diagnostic{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkGetAt(sink, MAREA_INTEGER_SIZE(0), &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkGetAt must succeed.";

    EXPECT_STREQ(MAREA_DiagnosticGetMessage(diagnostic), "mutable message") << "The stored message must stay intact after the caller's buffer changes.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, SinkAddGrowsPastInitialCapacity)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkAdd grows past its initial capacity, keeping order." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    constexpr int ADD_COUNT{ 300 };

    for (int index{ 0 }; index < ADD_COUNT; ++index) {
        ASSERT_EQ(MAREA_DiagnosticSinkAdd(sink, static_cast<MAREA_Uinteger32>(index), MAREA_DIAGNOSTIC_SEVERITY_ERROR, MakePosition(static_cast<MAREA_Uinteger32>(index)), "growing"), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkAdd must succeed for add " << index << ".";
    }

    ASSERT_EQ(MAREA_DiagnosticSinkGetCount(sink), static_cast<MAREA_IntegerSize>(ADD_COUNT)) << "The sink must hold every added diagnostic.";

    for (int index{ 0 }; index < ADD_COUNT; ++index) {
        const MAREA_Diagnostic *diagnostic{ nullptr };

        ASSERT_EQ(MAREA_DiagnosticSinkGetAt(sink, static_cast<MAREA_IntegerSize>(index), &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkGetAt must succeed for index " << index << ".";

        EXPECT_EQ(MAREA_DiagnosticGetCode(diagnostic), static_cast<MAREA_Uinteger32>(index)) << "Diagnostic order must be kept at index " << index << ".";
    }

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, SinkAddRejectsNullSink)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkAdd rejects a null sink." << std::endl;

    EXPECT_EQ(MAREA_DiagnosticSinkAdd(nullptr, MAREA_UINTEGER_32(1), MAREA_DIAGNOSTIC_SEVERITY_ERROR, MakePosition(MAREA_UINTEGER_32(0)), "message"), MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER) << "MAREA_DiagnosticSinkAdd must reject a null sink.";
}

TEST(MAREADiagnostic, SinkAddRejectsNullMessage)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkAdd rejects a null message." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    EXPECT_EQ(MAREA_DiagnosticSinkAdd(sink, MAREA_UINTEGER_32(1), MAREA_DIAGNOSTIC_SEVERITY_ERROR, MakePosition(MAREA_UINTEGER_32(0)), nullptr), MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER) << "MAREA_DiagnosticSinkAdd must reject a null message.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, SinkErrorCountIgnoresWarnings)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkGetErrorCount counts only MAREA_DIAGNOSTIC_SEVERITY_ERROR entries." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    ASSERT_EQ(MAREA_DiagnosticSinkAdd(sink, MAREA_UINTEGER_32(1), MAREA_DIAGNOSTIC_SEVERITY_ERROR, MakePosition(MAREA_UINTEGER_32(0)), "error"), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkAdd must succeed.";

    ASSERT_EQ(MAREA_DiagnosticSinkAdd(sink, MAREA_UINTEGER_32(2), MAREA_DIAGNOSTIC_SEVERITY_WARNING, MakePosition(MAREA_UINTEGER_32(0)), "warning"), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkAdd must succeed.";

    ASSERT_EQ(MAREA_DiagnosticSinkAdd(sink, MAREA_UINTEGER_32(3), MAREA_DIAGNOSTIC_SEVERITY_WARNING, MakePosition(MAREA_UINTEGER_32(0)), "warning"), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkAdd must succeed.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetCount(sink), MAREA_INTEGER_SIZE(3)) << "The sink must hold every added diagnostic.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetErrorCount(sink), MAREA_INTEGER_SIZE(1)) << "Only the error diagnostic must be counted.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, SinkGetAtRejectsOutOfRange)
{
    GTEST_COUT << "Validating MAREA_DiagnosticSinkGetAt rejects an out of range index." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    const MAREA_Diagnostic *diagnostic{ nullptr };

    EXPECT_EQ(MAREA_DiagnosticSinkGetAt(sink, MAREA_INTEGER_SIZE(0), &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_RANGE) << "MAREA_DiagnosticSinkGetAt must reject index 0 on an empty sink.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREADiagnostic, AccessorsOnNullReturnDefaults)
{
    GTEST_COUT << "Validating every diagnostic accessor returns a safe default for a null diagnostic." << std::endl;

    EXPECT_EQ(MAREA_DiagnosticGetCode(nullptr), MAREA_UINTEGER_32(0)) << "MAREA_DiagnosticGetCode must return 0 for null.";

    EXPECT_EQ(MAREA_DiagnosticGetSeverity(nullptr), MAREA_DIAGNOSTIC_SEVERITY_ERROR) << "MAREA_DiagnosticGetSeverity must return ERROR for null.";

    EXPECT_EQ(MAREA_DiagnosticGetPosition(nullptr).offset, MAREA_UINTEGER_32(0)) << "MAREA_DiagnosticGetPosition must return a zeroed position for null.";

    EXPECT_EQ(MAREA_DiagnosticGetMessage(nullptr), nullptr) << "MAREA_DiagnosticGetMessage must return null for null.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetCount(nullptr), MAREA_INTEGER_SIZE(0)) << "MAREA_DiagnosticSinkGetCount must return 0 for null.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetErrorCount(nullptr), MAREA_INTEGER_SIZE(0)) << "MAREA_DiagnosticSinkGetErrorCount must return 0 for null.";
}

TEST(MAREADiagnostic, TypesAreOpaque)
{
    GTEST_COUT << "Validating MAREA_Arena, MAREA_Diagnostic and MAREA_DiagnosticSink stay opaque to callers." << std::endl;

    EXPECT_FALSE(MareaIsComplete<MAREA_Arena>::value) << "MAREA_Arena must not be a complete type outside its implementation.";

    EXPECT_FALSE(MareaIsComplete<MAREA_Diagnostic>::value) << "MAREA_Diagnostic must not be a complete type outside its implementation.";

    EXPECT_FALSE(MareaIsComplete<MAREA_DiagnosticSink>::value) << "MAREA_DiagnosticSink must not be a complete type outside its implementation.";
}
