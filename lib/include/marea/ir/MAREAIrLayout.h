// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_IR_MAREAIRLAYOUT_H)
    #define MAREA_INCLUDE_IR_MAREAIRLAYOUT_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/ir/MAREAIrNode.h"
    #include "marea/sema/MAREASemaLayout.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of a layout table operation.
     */
    typedef enum MAREA_IrLayoutExcept
    {
        MAREA_IR_LAYOUT_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(2),
        MAREA_IR_LAYOUT_EXCEPT_ERROR_UNDECIDED = MAREA_UINTEGER_8(3),
        MAREA_IR_LAYOUT_EXCEPT_ERROR_NOT_FOUND = MAREA_UINTEGER_8(4),
    } MAREA_IrLayoutExcept;

    /**
     * @brief What a layout entry describes.
     */
    typedef enum MAREA_IrLayoutKind
    {
        MAREA_IR_LAYOUT_KIND_STRUCTURE = MAREA_UINTEGER_8(0),
        MAREA_IR_LAYOUT_KIND_ENUM = MAREA_UINTEGER_8(1),
        MAREA_IR_LAYOUT_KIND_COUNT = MAREA_UINTEGER_8(2),
    } MAREA_IrLayoutKind;

    /**
     * @brief Opaque entry of the layout table of a module: the layout of one structure or one enum that the module
     * declares. The entry copies the values that sema recorded and never computes one: the verdict and the reason are
     * the ones of sema, and every number counts in bytes from the start of the value. An opaque structure has an
     * entry that is undecided with the reason MAREA_SEMA_MEASURE_REASON_OPAQUE and no member, because its layout
     * belongs to C. An undecided entry has no size, no alignment and no offset: a later change may measure it
     * without a change of this format. The entry is valid for the arena lifetime.
     */
    typedef struct MAREA_IrLayout MAREA_IrLayout;

    /**
     * @brief Returns how many entries the layout table of a module has: one for each STRUCTURE_DECLARATION and
     * ENUM_DECLARATION child of the module, in the same order. A declaration that lowering leaves out has no entry.
     * @param module Module to query. Null, or a node that is not a MODULE, returns 0.
     * @return The entry count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_IrModuleGetLayoutCount(const MAREA_IrNode *module);

    /**
     * @brief Retrieves one entry of the layout table of a module.
     * @param module Module to query. Must not be null.
     * @param index Zero-based entry index, in the declaration order.
     * @param out Receives the entry, valid for the arena lifetime. Must not be null.
     * @return MAREA_IR_LAYOUT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when module or out is null, or ERROR_OUT_OF_RANGE
     * when index is not less than the entry count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutExcept MAREA_CALL MAREA_IrModuleGetLayoutAt(const MAREA_IrNode *module, MAREA_IntegerSize index, const MAREA_IrLayout **out);

    /**
     * @brief Finds the entry of a structure or an enum by its name.
     * @param module Module to query. Must not be null.
     * @param name Name text, not NUL terminated. Must not be null.
     * @param nameLength Byte length of name.
     * @param out Receives the entry, valid for the arena lifetime. Must not be null.
     * @return MAREA_IR_LAYOUT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when module, name or out is null, or ERROR_NOT_FOUND
     * when no entry has that name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutExcept MAREA_CALL MAREA_IrModuleFindLayout(const MAREA_IrNode *module, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_IrLayout **out);

    /**
     * @brief Tells whether an entry describes a structure or an enum.
     * @param layout Entry to query. Null returns MAREA_IR_LAYOUT_KIND_COUNT.
     * @return The kind.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutKind MAREA_CALL MAREA_IrLayoutGetKind(const MAREA_IrLayout *layout);

    /**
     * @brief Returns the name of the structure or the enum, a pointer into the source, not NUL terminated.
     * @param layout Entry to query. Null returns null.
     * @return The name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_IrLayoutGetName(const MAREA_IrLayout *layout);

    /**
     * @brief Returns the byte length of the name of the entry.
     * @param layout Entry to query. Null returns 0.
     * @return The length in bytes.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_IrLayoutGetNameLength(const MAREA_IrLayout *layout);

    /**
     * @brief Tells whether sema measured the layout.
     * @param layout Entry to query. Null returns MAREA_SEMA_MEASURE_VERDICT_UNDECIDED.
     * @return The verdict of sema.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaMeasureVerdict MAREA_CALL MAREA_IrLayoutGetVerdict(const MAREA_IrLayout *layout);

    /**
     * @brief Tells why the layout is undecided.
     * @param layout Entry to query. Null returns MAREA_SEMA_MEASURE_REASON_UNRESOLVED.
     * @return MAREA_SEMA_MEASURE_REASON_NONE for a measured entry, else the reason of sema.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaMeasureReason MAREA_CALL MAREA_IrLayoutGetReason(const MAREA_IrLayout *layout);

    /**
     * @brief Returns the size of the type in bytes, with its tail padding.
     * @param layout Entry to query. Must not be null.
     * @param out Receives the size. Must not be null.
     * @return MAREA_IR_LAYOUT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, or ERROR_UNDECIDED when
     * the entry is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetSize(const MAREA_IrLayout *layout, MAREA_Uinteger64 *out);

    /**
     * @brief Returns the alignment of the type in bytes.
     * @param layout Entry to query. Must not be null.
     * @param out Receives the alignment. Must not be null.
     * @return MAREA_IR_LAYOUT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, or ERROR_UNDECIDED when
     * the entry is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetAlignment(const MAREA_IrLayout *layout, MAREA_Uinteger64 *out);

    /**
     * @brief Returns the size of the tag of an enum in bytes. The tag sits at offset 0.
     * @param layout Entry to query. Must not be null.
     * @param out Receives the tag size, 0 for a structure. Must not be null.
     * @return MAREA_IR_LAYOUT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, or ERROR_UNDECIDED when
     * the entry is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetTagSize(const MAREA_IrLayout *layout, MAREA_Uinteger64 *out);

    /**
     * @brief Returns how many members the type declares: the fields of a structure or the variants of an enum. The
     * count is known even when the entry is undecided.
     * @param layout Entry to query. Null returns 0.
     * @return The member count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_IrLayoutGetMemberCount(const MAREA_IrLayout *layout);

    /**
     * @brief Returns the offset of one member: the offset of a field of a structure, or the payload offset of a
     * variant of an enum (the same for every variant).
     * @param layout Entry to query. Must not be null.
     * @param index Zero-based member index, in the order written.
     * @param out Receives the offset. Must not be null.
     * @return MAREA_IR_LAYOUT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, ERROR_UNDECIDED when
     * the entry is undecided, or ERROR_OUT_OF_RANGE when index is not less than the member count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetMemberOffset(const MAREA_IrLayout *layout, MAREA_IntegerSize index, MAREA_Uinteger64 *out);

    /**
     * @brief Returns how many fields one variant of an enum has.
     * @param layout Entry to query. Null returns 0.
     * @param variantIndex Zero-based variant index.
     * @return The field count, 0 for a structure and for an index that is out of range.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_IrLayoutGetVariantFieldCount(const MAREA_IrLayout *layout, MAREA_IntegerSize variantIndex);

    /**
     * @brief Returns the offset of one field of one variant of an enum, from the start of the enum value.
     * @param layout Entry to query. Must not be null.
     * @param variantIndex Zero-based variant index.
     * @param fieldIndex Zero-based field index inside the variant.
     * @param out Receives the offset. Must not be null.
     * @return MAREA_IR_LAYOUT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, ERROR_UNDECIDED when
     * the entry is undecided, or ERROR_OUT_OF_RANGE when an index is out of range, also for any index on a structure.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetVariantFieldOffset(const MAREA_IrLayout *layout, MAREA_IntegerSize variantIndex, MAREA_IntegerSize fieldIndex, MAREA_Uinteger64 *out);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_IR_MAREAIRLAYOUT_H */
