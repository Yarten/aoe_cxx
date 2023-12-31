/**
 * \note This document is generated by the program. Please do not modify it.
 */

#pragma once

#include "./count.h"

/**
 * \brief Concatenating multiple names into a new name.
 */
#define AOE_CONCAT(...) AOE_DETAILS_CONCAT_CALLER(AOE_DETAILS_CONCAT_, AOE_COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)


// A helper function ensures that the x- and y-parameters are fully expanded before the splice is performed.
#define AOE_DETAILS_CONCAT(x, y)        x##y
#define AOE_DETAILS_CONCAT_CALLER(x, y) AOE_DETAILS_CONCAT(x, y)

// Macro implementations with different number of parameters
#define AOE_DETAILS_CONCAT_2(x, y)    AOE_DETAILS_CONCAT_CALLER(x, y)
#define AOE_DETAILS_CONCAT_3(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_2(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_4(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_3(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_5(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_4(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_6(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_5(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_7(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_6(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_8(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_7(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_9(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_8(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_10(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_9(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_11(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_10(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_12(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_11(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_13(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_12(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_14(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_13(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_15(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_14(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_16(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_15(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_17(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_16(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_18(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_17(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_19(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_18(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_20(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_19(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_21(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_20(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_22(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_21(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_23(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_22(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_24(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_23(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_25(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_24(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_26(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_25(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_27(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_26(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_28(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_27(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_29(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_28(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_30(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_29(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_31(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_30(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_32(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_31(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_33(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_32(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_34(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_33(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_35(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_34(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_36(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_35(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_37(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_36(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_38(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_37(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_39(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_38(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_40(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_39(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_41(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_40(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_42(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_41(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_43(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_42(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_44(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_43(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_45(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_44(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_46(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_45(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_47(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_46(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_48(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_47(__VA_ARGS__))
#define AOE_DETAILS_CONCAT_49(x, ...) AOE_DETAILS_CONCAT_CALLER(x, AOE_DETAILS_CONCAT_48(__VA_ARGS__))

