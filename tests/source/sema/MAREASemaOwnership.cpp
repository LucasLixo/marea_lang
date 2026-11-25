// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREALibraryRegistryHelpers.hpp"
#include "tests/MAREASemaHelpers.hpp"

// INFO: internal header on purpose: the default passing mode is read through MAREA_SemaGetParameterSemantics, which no public header exposes.
#include "marea/internal/sema/MAREASemaInternal.h"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 NEEDS_WORD{ MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE };
    constexpr MAREA_Uinteger32 ESCAPES{ MAREA_SEMA_DIAGNOSTIC_CODE_REFERENCE_ESCAPES_SCOPE };
    constexpr MAREA_Uinteger32 INVALID_BORROW{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BORROW };
    constexpr MAREA_Uinteger32 INVALID_JUMP{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP };
    constexpr MAREA_Uinteger32 EXCLUSIVE{ MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT };
    constexpr MAREA_Uinteger32 CONSTANT_AS_MUTABLE{ MAREA_SEMA_DIAGNOSTIC_CODE_CANNOT_BORROW_CONSTANT_AS_MUTABLE };
    constexpr MAREA_Uinteger32 INVALID_SPAWN{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_SPAWN_ARGUMENT };
    constexpr MAREA_Uinteger32 USE_MOVED{ MAREA_SEMA_DIAGNOSTIC_CODE_USE_OF_MOVED_VALUE };
    constexpr MAREA_Uinteger32 MOVED_AGAIN{ MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_MOVED_AGAIN };
    constexpr MAREA_Uinteger32 PARTLY_MOVED{ MAREA_SEMA_DIAGNOSTIC_CODE_USE_OF_PARTIALLY_MOVED_VALUE };

    const std::string SHAPE{ "structure Shape x: float32; end structure Outer inner: Shape; end function make(): Shape return Shape{ x = 1.0 }; end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, MAREA_buildLibraryRegistry(fixture.parser.lexer.arena)));

        if (fixture.MAREA_sema("namespace main; require c.array; require c.slice; require c.task; require c.pointer; require c.vector; require c.table; require c.owned; require vulkan; require demo.owned; require demo.array; require demo.vector; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }

    const MAREA_AstNode *MAREA_findParameter(const MAREA_AstNode *node)
    {
        if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_PARAMETER) {
            return node;
        }

        const MAREA_IntegerSize COUNT{ MAREA_AstNodeGetChildCount(node) };

        for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
            const MAREA_AstNode *child{ nullptr };

            if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
                continue;
            }

            const MAREA_AstNode *found{ MAREA_findParameter(child) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }

    // INFO: the passing mode of the first parameter of the first function that has one, or the value 99 when the source does not check or has none.
    MAREA_Uinteger32 MAREA_firstParameterSemantics(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, MAREA_buildLibraryRegistry(fixture.parser.lexer.arena)));

        if (fixture.MAREA_sema("namespace main; require c.array; require c.slice; require c.task; require c.pointer; require c.vector; require c.table; require c.owned; require vulkan; require demo.owned; require demo.array; require demo.vector; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return 99;
        }

        const MAREA_AstNode *parameter{ MAREA_findParameter(fixture.parser.root) };

        if (parameter == nullptr) {
            return 99;
        }

        return static_cast<MAREA_Uinteger32>(MAREA_SemaGetParameterSemantics(fixture.context, parameter));
    }

    constexpr MAREA_Uinteger32 VALUE_READ_ONLY{ MAREA_SEMA_PARAMETER_SEMANTICS_VALUE_READ_ONLY };
    constexpr MAREA_Uinteger32 REFERENCE_READ_ONLY{ MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_READ_ONLY };
    constexpr MAREA_Uinteger32 REFERENCE_MUTABLE{ MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE };
} // namespace

TEST(MAREASemaOwnership, ParameterWithNoAttributeDefaultsByTypeClass)
{
    GTEST_COUT << "Validating a parameter with no semantics attribute is by value for a primitive or an enum, a read-only borrow for any other plain type, and a write borrow only for a move-in type." << std::endl;

    EXPECT_EQ(MAREA_firstParameterSemantics("function f(n: sinteger32) end"), VALUE_READ_ONLY) << "A primitive must default to VALUE_READ_ONLY.";

    EXPECT_EQ(MAREA_firstParameterSemantics("enum Color red; end function f(c: Color) end"), VALUE_READ_ONLY) << "An enum must default to VALUE_READ_ONLY.";

    EXPECT_EQ(MAREA_firstParameterSemantics(SHAPE + "function f(s: Shape) end"), REFERENCE_READ_ONLY) << "A user structure must default to REFERENCE_READ_ONLY.";

    EXPECT_EQ(MAREA_firstParameterSemantics("function f<T>(t: T) end"), REFERENCE_READ_ONLY) << "A generic parameter must default to REFERENCE_READ_ONLY.";

    EXPECT_EQ(MAREA_firstParameterSemantics("function f(a: c.array<float32>) end"), REFERENCE_READ_ONLY) << "A qualified library type must default to REFERENCE_READ_ONLY.";

    EXPECT_EQ(MAREA_firstParameterSemantics(SHAPE + "function f(o: demo.owned<Shape>) end"), REFERENCE_MUTABLE) << "A move-in owned must keep REFERENCE_MUTABLE.";

    EXPECT_EQ(MAREA_firstParameterSemantics(SHAPE + "function f(v: demo.vector<Shape>) end"), REFERENCE_MUTABLE) << "A move-in vector must keep REFERENCE_MUTABLE.";

    EXPECT_EQ(MAREA_firstParameterSemantics(SHAPE + "function f(#[semantics = reference_mutable] s: Shape) end"), REFERENCE_MUTABLE) << "An explicit attribute must win over the default.";
}

TEST(MAREASemaOwnership, ParameterModesAreAccepted)
{
    GTEST_COUT << "Validating a read-only borrow (the non-primitive default), a write borrow, a forced borrow and a move-in raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_read_only] s: Shape) end").empty()) << "A read-only borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape) end").empty()) << "A read-only borrow (a non-primitive defaults to reference_read_only) must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] s: Shape) end").empty()) << "A write borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; end function f(#[semantics = reference_read_only] c: Color) end").empty()) << "A forced borrow of an enum, the by-value exception, must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(o: owned<Shape>) end").empty()) << "A move-in must raise nothing.";
}

TEST(MAREASemaOwnership, ValueReadOnlyOnANonPrimitiveIsRejected)
{
    GTEST_COUT << "Validating value_read_only on a non-primitive is rejected; on a primitive it raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = value_read_only] s: Shape) end"), Codes{ INVALID_BORROW }) << "value_read_only on a plain structure must raise INVALID_BORROW.";

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = value_read_only] n: sinteger32) end").empty()) << "value_read_only on a primitive must raise nothing.";
}

TEST(MAREASemaOwnership, AssignmentOfAnExistingValueNeedsAWord)
{
    GTEST_COUT << "Validating assigning a name, a field or an element of a plain type needs copy or move." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape, b: Shape) a = b; end"), Codes{ NEEDS_WORD }) << "A name must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape, o: Outer) a = o.inner; end"), Codes{ NEEDS_WORD }) << "A field must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape, items: vector<Shape>) a = items[0]; end"), Codes{ NEEDS_WORD }) << "An element must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(b: Shape) local a: Shape = b; end"), Codes{ NEEDS_WORD }) << "A local initializer must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";
}

TEST(MAREASemaOwnership, ArrayCopiesByValueAndVectorNeedsAWord)
{
    GTEST_COUT << "Validating an array is assigned as a value and a vector needs copy or move." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: demo.array<float32, 4>, b: demo.array<float32, 4>) a = b; end").empty()) << "An array must be assigned without a word.";

    EXPECT_EQ(MAREA_codesOf("function f(a: demo.vector<float32>, b: demo.vector<float32>) a = b; end"), Codes{ NEEDS_WORD }) << "A vector must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: demo.vector<float32>, b: demo.vector<float32>) a = move b; end").empty()) << "A move of a vector must raise nothing.";
}

TEST(MAREASemaOwnership, CopyAndMoveSatisfyTheRule)
{
    GTEST_COUT << "Validating copy and move satisfy the assignment rule." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape, b: Shape) a = copy b; end").empty()) << "copy must satisfy the rule.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape, b: owned<Shape>) a = move b; end").empty()) << "move must satisfy the rule.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(b: Shape) local a: Shape = copy b; end").empty()) << "copy in a local initializer must satisfy the rule.";
}

TEST(MAREASemaOwnership, PrimitiveAndEnumAssignmentNeedNoWord)
{
    GTEST_COUT << "Validating a primitive or an enum is assigned with a plain equal sign." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: sinteger32, b: sinteger32) a = b; end").empty()) << "A primitive must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; end function f(a: Color, b: Color) a = b; end").empty()) << "An enum, the by-value exception, must raise nothing.";
}

TEST(MAREASemaOwnership, StructureIsNeverCopiedByValue)
{
    GTEST_COUT << "Validating a structure, even one marked move_only or an ordinary one, is never assigned by plain equal sign; only a primitive, a copyable array and an enum are." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape, b: Shape) a = b; end"), Codes{ NEEDS_WORD }) << "An ordinary structure must still raise ASSIGNMENT_NEEDS_COPY_OR_MOVE; #[value] no longer exempts it.";

    EXPECT_EQ(MAREA_codesOf("#[move_only] structure M x: float32; end function f(#[semantics = reference_mutable] a: M, b: M) a = b; end"), Codes{ NEEDS_WORD }) << "A move-only structure must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE on the bare equal sign.";
}

TEST(MAREASemaOwnership, NewValuesNeedNoWord)
{
    GTEST_COUT << "Validating a call or a literal on the right side needs no word." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape) a = make(); end").empty()) << "A call must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape) a = Shape{ x = 2.0 }; end").empty()) << "A literal must raise nothing.";
}

TEST(MAREASemaOwnership, CompoundAssignmentIsNotChecked)
{
    GTEST_COUT << "Validating a compound assignment raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: sinteger32, b: sinteger32) a += b; end").empty()) << "A compound assignment must raise nothing.";
}

TEST(MAREASemaOwnership, UnknownTypesAreNotGuessed)
{
    GTEST_COUT << "Validating an assignment whose type is a really unknown external type, a library view or an address raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] a: vulkan.Handle, b: vulkan.Handle) a = b; end").empty()) << "An unknown external type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] a: c.slice<sinteger32>, b: c.slice<sinteger32>) a = b; end").empty()) << "A library slice, a view, must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] a: c.pointer<sinteger32>, b: c.pointer<sinteger32>) a = b; end").empty()) << "A library pointer, an address, must raise nothing.";
}

TEST(MAREASemaOwnership, LibraryStorageTypeNeedsAWord)
{
    GTEST_COUT << "Validating a c.array, c.vector, c.table, c.owned or c.task assigned from an existing value needs copy or move." << std::endl;

    const char *const TYPES[] = { "c.array<sinteger32>", "c.vector<sinteger32>", "c.table<sinteger32, sinteger32>", "c.owned<sinteger32>", "c.task<sinteger32>" };

    for (const char *type : TYPES) {
        const std::string T{ type };

        EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] a: " + T + ", b: " + T + ") a = b; end"), Codes{ NEEDS_WORD }) << T << " assignment must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

        EXPECT_EQ(MAREA_codesOf("function f(b: " + T + ") local a: " + T + " = b; end"), Codes{ NEEDS_WORD }) << T << " local initializer must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

        EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] a: " + T + ", b: " + T + ") a = copy b; end").empty()) << T << " with copy must raise nothing.";

        EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] a: " + T + ", b: " + T + ") a = move b; end").empty()) << T << " with move must raise nothing.";
    }
}

TEST(MAREASemaOwnership, LibraryStorageTypeFromAFreshValueNeedsNoWord)
{
    GTEST_COUT << "Validating a c.array assigned from a call result raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function make(): c.array<sinteger32> return make(); end function f(#[semantics = reference_mutable] a: c.array<sinteger32>) a = make(); end").empty()) << "A call result must raise nothing.";
}

TEST(MAREASemaOwnership, ReturningABorrowIsRejected)
{
    GTEST_COUT << "Validating a borrow parameter cannot be returned directly." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_read_only] s: Shape): Shape return s; end"), Codes{ ESCAPES }) << "A read-only borrow must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: Shape): Shape return s; end"), Codes{ ESCAPES }) << "A read-only borrow (the non-primitive default) must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] s: Shape): Shape return s; end"), Codes{ ESCAPES }) << "A write borrow must raise REFERENCE_ESCAPES_SCOPE.";
}

TEST(MAREASemaOwnership, ReturningOwnedValuesIsAccepted)
{
    GTEST_COUT << "Validating a move-in parameter, a value, a copy and a local can be returned." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(o: demo.owned<Shape>): demo.owned<Shape> return o; end").empty()) << "A move-in must be returnable.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: sinteger32): sinteger32 return n; end").empty()) << "A by-value parameter must be returnable.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape): Shape return copy s; end").empty()) << "A copy must be returnable.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(): Shape local s: Shape = make(); return s; end").empty()) << "A local must be returnable.";
}

TEST(MAREASemaOwnership, LocalShadowingABorrowIsReturnable)
{
    GTEST_COUT << "Validating a local that shadows a borrow parameter is not confined." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape): Shape do local s: Shape = make(); return s; end end").empty()) << "A shadowing local must be returnable.";
}

TEST(MAREASemaOwnership, ForEachVariableCannotEscape)
{
    GTEST_COUT << "Validating a for each variable cannot be returned and can be used inside its loop." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: c.array<Shape>): Shape for each item in items do return item; end return make(); end"), Codes{ ESCAPES }) << "A for each variable must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(items: c.array<Shape>): Shape for each item in items do local y: float32 = item.x; end return make(); end").empty()) << "Use inside the loop must raise nothing.";
}

TEST(MAREASemaOwnership, PartitionAndWithNamesCannotEscape)
{
    GTEST_COUT << "Validating a partition part and a with name cannot be returned." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: c.array<Shape>): Shape fork_join partition items into 4 as part do return part; end end return make(); end"), (Codes{ ESCAPES, INVALID_JUMP })) << "A partition part returned from its body must raise REFERENCE_ESCAPES_SCOPE and INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: c.array<Shape>) fork_join partition items into 4 as part do local o: Outer = Outer{ inner = part }; end end end"), Codes{ ESCAPES }) << "A partition part stored in a field must raise REFERENCE_ESCAPES_SCOPE only.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: c.array<Shape>): Shape with items as view do return view; end return make(); end"), Codes{ ESCAPES }) << "A with name must raise REFERENCE_ESCAPES_SCOPE.";
}

TEST(MAREASemaOwnership, ReferenceCannotBeStoredInAField)
{
    GTEST_COUT << "Validating a reference cannot be stored in a literal field or a field assignment." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: Shape): Outer return Outer{ inner = s }; end"), Codes{ ESCAPES }) << "A literal field must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: Shape, #[semantics = reference_mutable] o: Outer) o.inner = s; end"), (Codes{ ESCAPES, NEEDS_WORD })) << "A field assignment must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape): Outer return Outer{ inner = copy s }; end").empty()) << "A copy in a field must raise nothing.";
}

TEST(MAREASemaOwnership, SameNameInTwoMutableArgumentsConflicts)
{
    GTEST_COUT << "Validating one name passed twice with a write borrow among them conflicts." << std::endl;

    const std::string TAKE{ SHAPE + "function take(#[semantics = reference_mutable] a: Shape, #[semantics = reference_mutable] b: Shape) end function other(#[semantics = reference_read_only] a: Shape, #[semantics = reference_read_only] b: Shape) end function plain(a: Shape, b: Shape) end " };

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(#[semantics = reference_mutable] s: Shape) take(s, s); end"), Codes{ EXCLUSIVE }) << "A repeated name with an explicit write borrow must raise EXCLUSIVE_BORROW_CONFLICT.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(s: Shape) plain(s, s); end").empty()) << "A repeated name with default parameters (read-only borrows) must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(s: Shape) other(s, s); end").empty()) << "Two read-only borrows must raise nothing.";
}

TEST(MAREASemaOwnership, ConstantBoundToAWriteBorrowParameterIsRejected)
{
    GTEST_COUT << "Validating a file constant passed to a write-borrow parameter raises CANNOT_BORROW_CONSTANT_AS_MUTABLE; a variable in the same position still passes." << std::endl;

    const std::string TAKE{ SHAPE + "constant ORIGIN: Shape = make(); function take(#[semantics = reference_mutable] a: Shape) end function other(#[semantics = reference_read_only] a: Shape) end function plain(a: Shape) end " };

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f() take(ORIGIN); end"), Codes{ CONSTANT_AS_MUTABLE }) << "A constant bound to an explicit write borrow must raise CANNOT_BORROW_CONSTANT_AS_MUTABLE.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f() plain(ORIGIN); end").empty()) << "A constant bound to a default parameter (a read-only borrow) must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(#[semantics = reference_mutable] s: Shape) take(s); end").empty()) << "A variable bound to the same write-borrow parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f() other(ORIGIN); end").empty()) << "A constant bound to a read-only borrow must raise nothing.";
}

TEST(MAREASemaOwnership, FieldOrElementOfAConstantBoundToAWriteBorrowParameterIsRejected)
{
    GTEST_COUT << "Validating a field or element chain rooted in a file constant passed to a write-borrow parameter raises CANNOT_BORROW_CONSTANT_AS_MUTABLE once." << std::endl;

    const std::string TAKE{ SHAPE + "constant ORIGIN: Outer = Outer{ inner = make() }; constant ORIGINS: c.array<Shape> = {}; function take(#[semantics = reference_mutable] a: Shape) end function take_number(#[semantics = reference_mutable] a: float32) end function other(#[semantics = reference_read_only] a: Shape) end function plain(a: Shape) end " };

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f() take(ORIGIN.inner); end"), Codes{ CONSTANT_AS_MUTABLE }) << "A field of a constant bound to a write borrow must raise CANNOT_BORROW_CONSTANT_AS_MUTABLE.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f() take_number(ORIGIN.inner.x); end"), Codes{ CONSTANT_AS_MUTABLE }) << "A nested field of a constant bound to a write borrow must raise CANNOT_BORROW_CONSTANT_AS_MUTABLE.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f() take(ORIGINS[0]); end"), Codes{ CONSTANT_AS_MUTABLE }) << "An element of a constant array bound to a write borrow must raise CANNOT_BORROW_CONSTANT_AS_MUTABLE.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(#[semantics = reference_mutable] o: Outer) take(o.inner); end").empty()) << "A field of a variable bound to a write borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f() other(ORIGIN.inner); end").empty()) << "A field of a constant bound to a read-only borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f() plain(ORIGIN.inner); end").empty()) << "A field of a constant bound to a default parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(#[semantics = reference_mutable] ORIGIN: Outer) take(ORIGIN.inner); end").empty()) << "A local that hides the constant name must raise nothing.";
}

TEST(MAREASemaOwnership, ErrorNodeInPlaceOfAssignmentOrReturnIsSkipped)
{
    GTEST_COUT << "Validating a broken assignment or return raises no sema diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] a: Shape) a = ; end").empty()) << "A broken assignment must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(a: Shape): Shape return + ; end").empty()) << "A broken return must raise no sema diagnostic.";
}

TEST(MAREASemaOwnership, EnumsAreByValue)
{
    GTEST_COUT << "Validating an enum is copied by a plain equal sign and can be returned from a parameter." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("enum Color red; green; end function f(a: Color, b: Color) a = b; end").empty()) << "An enum assignment must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; green; end function f(b: Color): Color return b; end").empty()) << "An enum parameter must be returnable.";
}

TEST(MAREASemaOwnership, ExistingValueGivenToOwnedParameterNeedsCopyOrMove)
{
    GTEST_COUT << "Validating a name, field or element given to an owned parameter raises ASSIGNMENT_NEEDS_COPY_OR_MOVE once; copy, move, a call result and a literal raise nothing." << std::endl;

    const std::string TAKE{ SHAPE + "function take(a: demo.owned<Shape>) end " };

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(s: Shape) take(s); end"), Codes{ NEEDS_WORD }) << "A name given to an owned parameter must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(o: Outer) take(o.inner); end"), Codes{ NEEDS_WORD }) << "A field given to an owned parameter must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(v: demo.vector<Shape>) take(v[0]); end"), Codes{ NEEDS_WORD }) << "An element given to an owned parameter must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(s: Shape) take(move s); end").empty()) << "A move argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(s: Shape) take(copy s); end").empty()) << "A copy argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f() take(make()); end").empty()) << "A call result must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f() take(Shape{ x = 1.0 }); end").empty()) << "A literal must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function take(a: demo.vector<Shape>) end function f(v: demo.vector<Shape>) take(v); end").empty()) << "A vector parameter is not checked by this rule.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function take<Item>(a: demo.owned<Item>) end function f(s: Shape) take(s); end"), Codes{ NEEDS_WORD }) << "A generic owned parameter given a known structure must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";
}

TEST(MAREASemaOwnership, UseAfterMoveStraightLine)
{
    GTEST_COUT << "Validating a read of a local after move raises USE_OF_MOVED_VALUE once, a second move raises VALUE_MOVED_AGAIN, and an assignment makes the local live again." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f() local a: Shape = make(); local b: Shape = move a; local c: float32 = a.x; end"), Codes{ USE_MOVED }) << "A field read after a move must raise USE_OF_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f() local a: Shape = make(); local b: Shape = move a; local c: Shape = move a; end"), Codes{ MOVED_AGAIN }) << "A second move must raise VALUE_MOVED_AGAIN.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f() local a: Shape = make(); local b: Shape = move a; a = make(); local c: float32 = a.x; end").empty()) << "An assignment must make a moved local live again.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f() local a: Shape = make(); local b: Shape = copy a; local c: float32 = a.x; end").empty()) << "A copy must not move.";

    EXPECT_TRUE(MAREA_codesOf("function f() local a: sinteger32 = 1; local b: sinteger32 = move a; local c: sinteger32 = a; end").empty()) << "A primitive is by value, so a move of it must not mark it moved.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f() local a: Shape = make(); local b: Shape = move a; do local a: Shape = make(); local c: float32 = a.x; end end").empty()) << "A new local of the same name must be live.";
}

TEST(MAREASemaOwnership, UseAfterMoveInScopesAndBranches)
{
    GTEST_COUT << "Validating a move in a nested block, an if arm or a switch arm: a move in any arm that reaches the end counts after the branch." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f() local a: Shape = make(); do local b: Shape = move a; end local c: float32 = a.x; end"), Codes{ USE_MOVED }) << "A move in a do block must reach the outer scope.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); if n == 1 then local b: Shape = move a; else local c: Shape = move a; end local d: float32 = a.x; end"), Codes{ USE_MOVED }) << "A move in every arm of an if must reach after it.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); if n == 1 then local b: Shape = move a; end local d: float32 = a.x; end"), Codes{ USE_MOVED }) << "A move in one arm of an if with no else is maybe moved, so a read after it must raise USE_OF_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); if n == 1 then local b: Shape = move a; end local c: Shape = move a; end"), Codes{ MOVED_AGAIN }) << "A move after a branch that maybe moved the local must raise VALUE_MOVED_AGAIN.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); if n == 1 then local b: Shape = move a; a = make(); else a = make(); end local d: float32 = a.x; end").empty()) << "A local that every arm of an if with an else made live again must be live after it.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); local b: Shape = move a; if n == 1 then a = make(); end local d: float32 = a.x; end"), Codes{ USE_MOVED }) << "An if with no else keeps the moved state of the path that skips the arm.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32): float32 local a: Shape = make(); if n == 1 then local b: Shape = move a; return 1.0; else local c: float32 = a.x; end return a.x; end").empty()) << "An arm that moves and returns must stay out of the merge when a sibling arm does not move.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32): float32 local a: Shape = make(); while n < 2 do if n == 0 then local b: Shape = move a; break; end end local c: float32 = a.x; return c; end").empty()) << "An arm that moves and breaks leaves the loop, so it must not count in the merge nor after the loop.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); if n == 1 then local b: Shape = move a; elseif n == 2 then local c: Shape = move a; else local d: Shape = make(); end local e: float32 = a.x; end"), Codes{ USE_MOVED }) << "A move in some arms must make the local maybe moved after the if.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32): float32 local a: Shape = make(); if n == 1 then local b: Shape = move a; return 1.0; end return a.x; end").empty()) << "An arm that returns must not count in the merge.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); if n == 1 then local b: Shape = move a; local c: Shape = move a; end end"), Codes{ MOVED_AGAIN }) << "A double move inside one arm must raise VALUE_MOVED_AGAIN.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); if n == 1 then local b: Shape = move a; else local c: float32 = a.x; end end").empty()) << "A sibling arm must not see the move of another arm.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); switch n case 1 then local b: Shape = move a; default then local c: Shape = move a; end local d: float32 = a.x; end"), Codes{ USE_MOVED }) << "A move in every arm of a switch with a default must reach after it.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); switch n case 1 then local b: Shape = move a; default then end local d: float32 = a.x; end"), Codes{ USE_MOVED }) << "A switch arm that moves must make the local maybe moved after the switch.";
}

TEST(MAREASemaOwnership, UseAfterMoveInLoops)
{
    GTEST_COUT << "Validating a move of an outer local in a loop body raises VALUE_MOVED_AGAIN unless the body leaves the loop, and never reaches after the loop." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f() local a: Shape = make(); for turn in 0 .. 2 do local b: Shape = move a; end end"), Codes{ MOVED_AGAIN }) << "A move of an outer local in a for body must raise VALUE_MOVED_AGAIN.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); while n < 2 do local b: Shape = move a; end end"), Codes{ MOVED_AGAIN }) << "A move of an outer local in a while body must raise VALUE_MOVED_AGAIN.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32) while n < 2 do local a: Shape = make(); local b: Shape = move a; end end").empty()) << "A local made in the body must be live again each turn.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); while n < 2 do local b: Shape = move a; break; end end").empty()) << "A body that leaves the loop after the move must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32): float32 local a: Shape = make(); while n < 2 do local b: Shape = move a; return 1.0; end return 0.0; end").empty()) << "A body that returns after the move must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); while n < 2 do local b: Shape = move a; a = make(); end end").empty()) << "A body that makes the local live again must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(n: sinteger32): float32 local a: Shape = make(); while n < 2 do if n == 0 then local b: Shape = move a; break; end end return 0.0; end").empty()) << "A guarded move that leaves the loop must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(n: sinteger32) local a: Shape = make(); while n < 2 do if n == 0 then local b: Shape = move a; end end end"), Codes{ MOVED_AGAIN }) << "A conditional move of an outer local in a body that does not leave the loop must raise VALUE_MOVED_AGAIN.";
}

TEST(MAREASemaOwnership, UseAfterMoveOfOwnedParameterAndCallArgument)
{
    GTEST_COUT << "Validating a move into an owned parameter, a move of an owned parameter and a spawn argument after the move." << std::endl;

    const std::string TAKE{ SHAPE + "function take(a: demo.owned<Shape>): float32 return a.x; end function read(#[semantics = reference_read_only] a: Shape): float32 return a.x; end " };

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f() local a: Shape = make(); local r: float32 = take(move a); local s: float32 = read(a); end"), Codes{ USE_MOVED }) << "A read after a move into an owned parameter must raise USE_OF_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(a: demo.owned<Shape>): float32 local r: float32 = take(move a); return a.x; end"), Codes{ USE_MOVED }) << "A read of an owned parameter after it was moved on must raise USE_OF_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(a: demo.owned<Shape>) local r: float32 = take(move a); local s: float32 = take(move a); end"), Codes{ MOVED_AGAIN }) << "A second move of an owned parameter must raise VALUE_MOVED_AGAIN.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(a: demo.owned<Shape>) local r: float32 = take(move a); fork_join spawn read(a); end end"), (Codes{ INVALID_SPAWN, USE_MOVED })) << "A spawn argument after the move must raise USE_OF_MOVED_VALUE besides the spawn rule.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(a: demo.owned<Shape>): float32 local r: float32 = take(move a); return r + a.x; end"), Codes{ USE_MOVED }) << "A read in a later expression must raise USE_OF_MOVED_VALUE.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(a: demo.owned<Shape>): float32 return take(move a); end").empty()) << "A single move must raise nothing.";
}

TEST(MAREASemaOwnership, UseAfterMoveOfOpaqueOwnerAndMethodReceiver)
{
    GTEST_COUT << "Validating an owner of an opaque type, a method receiver and a generic owned parameter after a move." << std::endl;

    EXPECT_EQ(MAREA_codesOf("opaque structure Texture; function show(#[semantics = reference_read_only] t: Texture) end function f(a: demo.owned<Texture>) local b: demo.owned<Texture> = move a; show(a); end"), Codes{ USE_MOVED }) << "An owned opaque value used after a move must raise USE_OF_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "implementation Shape function sum(self: Shape): float32 return self.x; end end function f(): float32 local a: Shape = make(); local b: Shape = move a; return a.sum(); end"), Codes{ USE_MOVED }) << "A method call on a moved value must raise USE_OF_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf("function sink<Item>(value: demo.owned<Item>) where Item: Movable end function twice<Item>(value: demo.owned<Item>) where Item: Movable sink(move value); sink(move value); end"), Codes{ MOVED_AGAIN }) << "A generic owned parameter moved twice must raise VALUE_MOVED_AGAIN.";
}

TEST(MAREASemaOwnership, UseAfterPartMoved)
{
    GTEST_COUT << "Validating a use of an owner after a field or an element of it was moved out raises USE_OF_PARTIALLY_MOVED_VALUE, and a disjoint part, a reassigned part and a by value part raise nothing." << std::endl;

    const std::string OUTER{ SHAPE + "function sum(a: Shape): float32 return a.x; end " };

    EXPECT_EQ(MAREA_codesOf(OUTER + "function f() local o: Outer = Outer{ inner = make() }; local s: Shape = move o.inner; local t: Shape = move o; end"), Codes{ PARTLY_MOVED }) << "A move of the whole owner after a field was moved out must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(OUTER + "function f() local o: Outer = Outer{ inner = make() }; local s: Shape = move o.inner; local x: float32 = o.inner.x; end"), Codes{ PARTLY_MOVED }) << "A read below the moved field must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(OUTER + "function f() local o: Outer = Outer{ inner = make() }; local s: Shape = move o.inner; local r: float32 = sum(o.inner); end"), Codes{ PARTLY_MOVED }) << "A read of the moved field itself must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(OUTER + "function f() local o: Outer = Outer{ inner = make() }; local s: Shape = move o.inner; local t: Shape = move o.inner; end"), Codes{ PARTLY_MOVED }) << "A second move of the same field must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "implementation Outer function total(self: Outer): float32 return 1.0; end end function f(): float32 local o: Outer = Outer{ inner = make() }; local s: Shape = move o.inner; return o.total(); end"), Codes{ PARTLY_MOVED }) << "A method call on the owner after a field was moved out must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_TRUE(MAREA_codesOf("structure Pair a: Shape; b: Shape; end " + SHAPE + "function f() local p: Pair = Pair{ a = make(), b = make() }; local s: Shape = move p.a; local x: float32 = p.b.x; end").empty()) << "A read of a sibling field must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(OUTER + "function f() local o: Outer = Outer{ inner = make() }; local s: Shape = move o.inner; o.inner = make(); local x: float32 = o.inner.x; end").empty()) << "An assignment to the moved field must make it live again.";

    EXPECT_TRUE(MAREA_codesOf(OUTER + "function f() local o: Outer = Outer{ inner = make() }; local s: Shape = move o.inner; o = Outer{ inner = make() }; local x: float32 = o.inner.x; end").empty()) << "An assignment to the whole owner must make it live again.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f() local s: Shape = make(); local x: float32 = move s.x; local y: float32 = s.x; end").empty()) << "A by value field is copied, so a move of it must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(OUTER + "function f(n: sinteger32) local o: Outer = Outer{ inner = make() }; if n == 1 then local s: Shape = move o.inner; end local x: float32 = o.inner.x; end").empty()) << "A part moved in one arm only must round toward valid.";

    EXPECT_EQ(MAREA_codesOf(OUTER + "function f(n: sinteger32) local o: Outer = Outer{ inner = make() }; if n == 1 then local s: Shape = move o.inner; else local t: Shape = move o.inner; end local x: float32 = o.inner.x; end"), Codes{ PARTLY_MOVED }) << "A part moved in every arm must reach after the branch.";
}

TEST(MAREASemaOwnership, UseAfterElementMoved)
{
    GTEST_COUT << "Validating a use of a collection after an element was moved out, and a move of a for each element, raise USE_OF_PARTIALLY_MOVED_VALUE." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(v: demo.owned<demo.vector<Shape>>): float32 local s: Shape = move v[0]; return v[0].x; end"), Codes{ PARTLY_MOVED }) << "A read of the moved element must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function take(v: demo.owned<demo.vector<Shape>>) end function f(v: demo.owned<demo.vector<Shape>>) local s: Shape = move v[0]; take(move v); end"), Codes{ PARTLY_MOVED }) << "A move of the whole collection after an element was moved out must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(v: demo.owned<demo.vector<Shape>>): float32 local s: Shape = move v[0]; return v[1].x; end").empty()) << "A read of another literal index must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(v: demo.owned<demo.vector<Shape>>, i: sinteger32, j: sinteger32): float32 local s: Shape = move v[i]; return v[j].x; end").empty()) << "A computed index is never compared, so it must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(v: demo.vector<Shape>) for each item in v do local s: Shape = move item; end end"), Codes{ PARTLY_MOVED }) << "A move of a for each element must raise USE_OF_PARTIALLY_MOVED_VALUE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(v: demo.owned<demo.vector<Shape>>, w: demo.owned<demo.vector<Shape>>) for each item in v do local o: demo.vector<Shape> = move w; break; end end").empty()) << "A move of the collection itself is not a move of a loop element, so it must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(v: demo.vector<sinteger32>) for each item in v do local s: sinteger32 = move item; end end").empty()) << "A by value for each element is copied, so a move of it must raise nothing.";
}

TEST(MAREASemaOwnership, FieldOfABorrowCannotBeReturned)
{
    GTEST_COUT << "Validating a field of a borrow cannot be returned, also from an arm of switch, and a copy or a primitive field can." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(o: Outer): Shape return o.inner; end"), Codes{ ESCAPES }) << "A structure field of a borrow must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(o: Outer, n: sinteger32): Shape switch n case 1 then return o.inner; default then return copy o.inner; end end"), Codes{ ESCAPES }) << "A field of a borrow returned from one arm of switch must raise REFERENCE_ESCAPES_SCOPE once.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(o: Outer): Shape return copy o.inner; end").empty()) << "A copy of a field must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape): float32 return s.x; end").empty()) << "A primitive field is a value and must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(): Shape local o: Outer = Outer{ inner = make() }; return o.inner; end").empty()) << "A field of a local owner must raise nothing.";
}

TEST(MAREASemaOwnership, PartitionPartCannotBeStoredInAnOuterLocal)
{
    GTEST_COUT << "Validating a partition part assigned to a local declared outside the body raises REFERENCE_ESCAPES_SCOPE." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: c.array<float32>) local kept: c.slice<float32> = c.slice<float32>{}; fork_join partition items into 2 as part do kept = part; end end end"), (Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_BODY_WRITES_OUTER_NAME, ESCAPES })) << "A part stored in an outer local must raise PARTITION_BODY_WRITES_OUTER_NAME and REFERENCE_ESCAPES_SCOPE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(items: c.array<float32>) fork_join partition items into 2 as part do local mine: c.slice<float32> = c.slice<float32>{}; mine = part; end end end").empty()) << "A part stored in a local of the body must raise nothing.";
}

TEST(MAREASemaOwnership, BorrowCannotBeAppendedToAVector)
{
    GTEST_COUT << "Validating a borrow passed to append raises REFERENCE_ESCAPES_SCOPE and a copy does not." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] shapes: vector<Shape>, s: Shape) shapes.append(s); end"), Codes{ ESCAPES }) << "A borrow appended to a vector must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(#[semantics = reference_mutable] shapes: vector<Shape>, s: Shape) shapes.append(copy s); end").empty()) << "A copy appended to a vector must raise nothing.";
}
