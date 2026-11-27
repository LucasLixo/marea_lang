// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathLibrary.h"

#include "marea/base/MAREACTypes.h"
#include "marea/base/MAREAHostFunction.h"
#include "marea/base/MAREAMacros.h"
#include "marea/base/MAREAMathVector2.h"
#include "marea/base/MAREAMathVector3.h"
#include "marea/base/MAREAMathVector4.h"
#include "marea/base/MAREATypeRegistry.h"

#include <string.h>

MAREA_HOST_FUNCTION(MAREA_MathAddVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathSubtractVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathDivideVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathScaleVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector2Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathLengthVector2Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathDistanceVector2Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathNormalizeVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathMixVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathMinimumVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathMaximumVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathClampVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathAbsoluteVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathSineVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathCosineVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathTangentVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcSineVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcCosineVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcTangentVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathRadiansVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathDegreesVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathExponentialVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathLogarithmVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathSquareRootVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathInverseSquareRootVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathPowerVector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathAddVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathSubtractVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDivideVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_CONTEXT, MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathScaleVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector2Sinteger32, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMinimumVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMaximumVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathClampVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathAbsoluteVector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathAddVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathSubtractVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathDivideVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathScaleVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector3Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathLengthVector3Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathDistanceVector3Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathNormalizeVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathMixVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathMinimumVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathMaximumVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathClampVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathAbsoluteVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathSineVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathCosineVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathTangentVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcSineVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcCosineVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcTangentVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathRadiansVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathDegreesVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathExponentialVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathLogarithmVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathSquareRootVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathInverseSquareRootVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathPowerVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathCrossVector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathAddVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathSubtractVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDivideVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_CONTEXT, MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathScaleVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector3Sinteger32, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMinimumVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMaximumVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathClampVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathAbsoluteVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathCrossVector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathAddVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathSubtractVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathDivideVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathScaleVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector4Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathLengthVector4Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathDistanceVector4Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathNormalizeVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMixVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathMinimumVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMaximumVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathClampVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathAbsoluteVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathSineVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathCosineVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathTangentVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcSineVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcCosineVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathArcTangentVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathRadiansVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathDegreesVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathExponentialVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathLogarithmVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathSquareRootVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathInverseSquareRootVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathPowerVector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathAddVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathSubtractVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDivideVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_CONTEXT, MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathScaleVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector4Sinteger32, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMinimumVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMaximumVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathClampVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathAbsoluteVector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));

// INFO: registers one function unless an earlier one failed, so the first error is the one that is returned.
#define MAREA_G_REGISTER(registry, except, name, function)                                                                                                   \
    if ((except) == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {                                                                                                    \
        (except) = MAREA_REGISTER_FUNCTION((registry), MAREA_LIBRARY_FLAG_MAREA_MATH, "marea.math", MAREA_INTEGER_SIZE(10), (name), strlen(name), function); \
    }                                                                                                                                                        \
    MAREA_IGNORE(except)

MAREA_TypeRegistryExcept MAREA_CALL MAREA_MathLibraryRegister(MAREA_TypeRegistry *registry)
{
    if (registry == MAREA_NULLPTR) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_TypeRegistryExcept except = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;

    MAREA_G_REGISTER(registry, except, "add_vector2_float32", MAREA_MathAddVector2Float32);

    MAREA_G_REGISTER(registry, except, "subtract_vector2_float32", MAREA_MathSubtractVector2Float32);

    MAREA_G_REGISTER(registry, except, "multiply_vector2_float32", MAREA_MathMultiplyVector2Float32);

    MAREA_G_REGISTER(registry, except, "divide_vector2_float32", MAREA_MathDivideVector2Float32);

    MAREA_G_REGISTER(registry, except, "scale_vector2_float32", MAREA_MathScaleVector2Float32);

    MAREA_G_REGISTER(registry, except, "dot_vector2_float32", MAREA_MathDotVector2Float32);

    MAREA_G_REGISTER(registry, except, "length_vector2_float32", MAREA_MathLengthVector2Float32);

    MAREA_G_REGISTER(registry, except, "distance_vector2_float32", MAREA_MathDistanceVector2Float32);

    MAREA_G_REGISTER(registry, except, "normalize_vector2_float32", MAREA_MathNormalizeVector2Float32);

    MAREA_G_REGISTER(registry, except, "mix_vector2_float32", MAREA_MathMixVector2Float32);

    MAREA_G_REGISTER(registry, except, "minimum_vector2_float32", MAREA_MathMinimumVector2Float32);

    MAREA_G_REGISTER(registry, except, "maximum_vector2_float32", MAREA_MathMaximumVector2Float32);

    MAREA_G_REGISTER(registry, except, "clamp_vector2_float32", MAREA_MathClampVector2Float32);

    MAREA_G_REGISTER(registry, except, "absolute_vector2_float32", MAREA_MathAbsoluteVector2Float32);

    MAREA_G_REGISTER(registry, except, "sine_vector2_float32", MAREA_MathSineVector2Float32);

    MAREA_G_REGISTER(registry, except, "cosine_vector2_float32", MAREA_MathCosineVector2Float32);

    MAREA_G_REGISTER(registry, except, "tangent_vector2_float32", MAREA_MathTangentVector2Float32);

    MAREA_G_REGISTER(registry, except, "arc_sine_vector2_float32", MAREA_MathArcSineVector2Float32);

    MAREA_G_REGISTER(registry, except, "arc_cosine_vector2_float32", MAREA_MathArcCosineVector2Float32);

    MAREA_G_REGISTER(registry, except, "arc_tangent_vector2_float32", MAREA_MathArcTangentVector2Float32);

    MAREA_G_REGISTER(registry, except, "radians_vector2_float32", MAREA_MathRadiansVector2Float32);

    MAREA_G_REGISTER(registry, except, "degrees_vector2_float32", MAREA_MathDegreesVector2Float32);

    MAREA_G_REGISTER(registry, except, "exponential_vector2_float32", MAREA_MathExponentialVector2Float32);

    MAREA_G_REGISTER(registry, except, "logarithm_vector2_float32", MAREA_MathLogarithmVector2Float32);

    MAREA_G_REGISTER(registry, except, "square_root_vector2_float32", MAREA_MathSquareRootVector2Float32);

    MAREA_G_REGISTER(registry, except, "inverse_square_root_vector2_float32", MAREA_MathInverseSquareRootVector2Float32);

    MAREA_G_REGISTER(registry, except, "power_vector2_float32", MAREA_MathPowerVector2Float32);

    MAREA_G_REGISTER(registry, except, "add_vector2_sinteger32", MAREA_MathAddVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "subtract_vector2_sinteger32", MAREA_MathSubtractVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_vector2_sinteger32", MAREA_MathMultiplyVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "divide_vector2_sinteger32", MAREA_MathDivideVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "scale_vector2_sinteger32", MAREA_MathScaleVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "dot_vector2_sinteger32", MAREA_MathDotVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "minimum_vector2_sinteger32", MAREA_MathMinimumVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "maximum_vector2_sinteger32", MAREA_MathMaximumVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "clamp_vector2_sinteger32", MAREA_MathClampVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "absolute_vector2_sinteger32", MAREA_MathAbsoluteVector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "add_vector3_float32", MAREA_MathAddVector3Float32);

    MAREA_G_REGISTER(registry, except, "subtract_vector3_float32", MAREA_MathSubtractVector3Float32);

    MAREA_G_REGISTER(registry, except, "multiply_vector3_float32", MAREA_MathMultiplyVector3Float32);

    MAREA_G_REGISTER(registry, except, "divide_vector3_float32", MAREA_MathDivideVector3Float32);

    MAREA_G_REGISTER(registry, except, "scale_vector3_float32", MAREA_MathScaleVector3Float32);

    MAREA_G_REGISTER(registry, except, "dot_vector3_float32", MAREA_MathDotVector3Float32);

    MAREA_G_REGISTER(registry, except, "length_vector3_float32", MAREA_MathLengthVector3Float32);

    MAREA_G_REGISTER(registry, except, "distance_vector3_float32", MAREA_MathDistanceVector3Float32);

    MAREA_G_REGISTER(registry, except, "normalize_vector3_float32", MAREA_MathNormalizeVector3Float32);

    MAREA_G_REGISTER(registry, except, "mix_vector3_float32", MAREA_MathMixVector3Float32);

    MAREA_G_REGISTER(registry, except, "minimum_vector3_float32", MAREA_MathMinimumVector3Float32);

    MAREA_G_REGISTER(registry, except, "maximum_vector3_float32", MAREA_MathMaximumVector3Float32);

    MAREA_G_REGISTER(registry, except, "clamp_vector3_float32", MAREA_MathClampVector3Float32);

    MAREA_G_REGISTER(registry, except, "absolute_vector3_float32", MAREA_MathAbsoluteVector3Float32);

    MAREA_G_REGISTER(registry, except, "sine_vector3_float32", MAREA_MathSineVector3Float32);

    MAREA_G_REGISTER(registry, except, "cosine_vector3_float32", MAREA_MathCosineVector3Float32);

    MAREA_G_REGISTER(registry, except, "tangent_vector3_float32", MAREA_MathTangentVector3Float32);

    MAREA_G_REGISTER(registry, except, "arc_sine_vector3_float32", MAREA_MathArcSineVector3Float32);

    MAREA_G_REGISTER(registry, except, "arc_cosine_vector3_float32", MAREA_MathArcCosineVector3Float32);

    MAREA_G_REGISTER(registry, except, "arc_tangent_vector3_float32", MAREA_MathArcTangentVector3Float32);

    MAREA_G_REGISTER(registry, except, "radians_vector3_float32", MAREA_MathRadiansVector3Float32);

    MAREA_G_REGISTER(registry, except, "degrees_vector3_float32", MAREA_MathDegreesVector3Float32);

    MAREA_G_REGISTER(registry, except, "exponential_vector3_float32", MAREA_MathExponentialVector3Float32);

    MAREA_G_REGISTER(registry, except, "logarithm_vector3_float32", MAREA_MathLogarithmVector3Float32);

    MAREA_G_REGISTER(registry, except, "square_root_vector3_float32", MAREA_MathSquareRootVector3Float32);

    MAREA_G_REGISTER(registry, except, "inverse_square_root_vector3_float32", MAREA_MathInverseSquareRootVector3Float32);

    MAREA_G_REGISTER(registry, except, "power_vector3_float32", MAREA_MathPowerVector3Float32);

    MAREA_G_REGISTER(registry, except, "cross_vector3_float32", MAREA_MathCrossVector3Float32);

    MAREA_G_REGISTER(registry, except, "add_vector3_sinteger32", MAREA_MathAddVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "subtract_vector3_sinteger32", MAREA_MathSubtractVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_vector3_sinteger32", MAREA_MathMultiplyVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "divide_vector3_sinteger32", MAREA_MathDivideVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "scale_vector3_sinteger32", MAREA_MathScaleVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "dot_vector3_sinteger32", MAREA_MathDotVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "minimum_vector3_sinteger32", MAREA_MathMinimumVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "maximum_vector3_sinteger32", MAREA_MathMaximumVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "clamp_vector3_sinteger32", MAREA_MathClampVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "absolute_vector3_sinteger32", MAREA_MathAbsoluteVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "cross_vector3_sinteger32", MAREA_MathCrossVector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "add_vector4_float32", MAREA_MathAddVector4Float32);

    MAREA_G_REGISTER(registry, except, "subtract_vector4_float32", MAREA_MathSubtractVector4Float32);

    MAREA_G_REGISTER(registry, except, "multiply_vector4_float32", MAREA_MathMultiplyVector4Float32);

    MAREA_G_REGISTER(registry, except, "divide_vector4_float32", MAREA_MathDivideVector4Float32);

    MAREA_G_REGISTER(registry, except, "scale_vector4_float32", MAREA_MathScaleVector4Float32);

    MAREA_G_REGISTER(registry, except, "dot_vector4_float32", MAREA_MathDotVector4Float32);

    MAREA_G_REGISTER(registry, except, "length_vector4_float32", MAREA_MathLengthVector4Float32);

    MAREA_G_REGISTER(registry, except, "distance_vector4_float32", MAREA_MathDistanceVector4Float32);

    MAREA_G_REGISTER(registry, except, "normalize_vector4_float32", MAREA_MathNormalizeVector4Float32);

    MAREA_G_REGISTER(registry, except, "mix_vector4_float32", MAREA_MathMixVector4Float32);

    MAREA_G_REGISTER(registry, except, "minimum_vector4_float32", MAREA_MathMinimumVector4Float32);

    MAREA_G_REGISTER(registry, except, "maximum_vector4_float32", MAREA_MathMaximumVector4Float32);

    MAREA_G_REGISTER(registry, except, "clamp_vector4_float32", MAREA_MathClampVector4Float32);

    MAREA_G_REGISTER(registry, except, "absolute_vector4_float32", MAREA_MathAbsoluteVector4Float32);

    MAREA_G_REGISTER(registry, except, "sine_vector4_float32", MAREA_MathSineVector4Float32);

    MAREA_G_REGISTER(registry, except, "cosine_vector4_float32", MAREA_MathCosineVector4Float32);

    MAREA_G_REGISTER(registry, except, "tangent_vector4_float32", MAREA_MathTangentVector4Float32);

    MAREA_G_REGISTER(registry, except, "arc_sine_vector4_float32", MAREA_MathArcSineVector4Float32);

    MAREA_G_REGISTER(registry, except, "arc_cosine_vector4_float32", MAREA_MathArcCosineVector4Float32);

    MAREA_G_REGISTER(registry, except, "arc_tangent_vector4_float32", MAREA_MathArcTangentVector4Float32);

    MAREA_G_REGISTER(registry, except, "radians_vector4_float32", MAREA_MathRadiansVector4Float32);

    MAREA_G_REGISTER(registry, except, "degrees_vector4_float32", MAREA_MathDegreesVector4Float32);

    MAREA_G_REGISTER(registry, except, "exponential_vector4_float32", MAREA_MathExponentialVector4Float32);

    MAREA_G_REGISTER(registry, except, "logarithm_vector4_float32", MAREA_MathLogarithmVector4Float32);

    MAREA_G_REGISTER(registry, except, "square_root_vector4_float32", MAREA_MathSquareRootVector4Float32);

    MAREA_G_REGISTER(registry, except, "inverse_square_root_vector4_float32", MAREA_MathInverseSquareRootVector4Float32);

    MAREA_G_REGISTER(registry, except, "power_vector4_float32", MAREA_MathPowerVector4Float32);

    MAREA_G_REGISTER(registry, except, "add_vector4_sinteger32", MAREA_MathAddVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "subtract_vector4_sinteger32", MAREA_MathSubtractVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_vector4_sinteger32", MAREA_MathMultiplyVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "divide_vector4_sinteger32", MAREA_MathDivideVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "scale_vector4_sinteger32", MAREA_MathScaleVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "dot_vector4_sinteger32", MAREA_MathDotVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "minimum_vector4_sinteger32", MAREA_MathMinimumVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "maximum_vector4_sinteger32", MAREA_MathMaximumVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "clamp_vector4_sinteger32", MAREA_MathClampVector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "absolute_vector4_sinteger32", MAREA_MathAbsoluteVector4Sinteger32);

    return except;
}
