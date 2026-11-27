// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathLibrary.h"

#include "marea/base/MAREACTypes.h"
#include "marea/base/MAREAHostFunction.h"
#include "marea/base/MAREAMacros.h"
#include "marea/base/MAREAMathMatrix2x2.h"
#include "marea/base/MAREAMathMatrix3x3.h"
#include "marea/base/MAREAMathMatrix4x4.h"
#include "marea/base/MAREAMathQuaternion.h"
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
MAREA_HOST_FUNCTION(MAREA_MathIdentityMatrix2x2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix2x2Float32));
MAREA_HOST_FUNCTION(MAREA_MathTransposeMatrix2x2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix2x2Float32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix2x2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix2x2Float32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Float32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix2x2Vector2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathIdentityMatrix2x2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix2x2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathTransposeMatrix2x2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix2x2Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix2x2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix2x2Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix2x2Vector2Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector2Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDeterminantMatrix2x2Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Float32));
MAREA_HOST_FUNCTION(MAREA_MathInverseMatrix2x2Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix2x2Float32), MAREA_HOST_BORROW(MAREA_MathMatrix2x2Float32));
MAREA_HOST_FUNCTION(MAREA_MathIdentityMatrix3x3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix3x3Float32));
MAREA_HOST_FUNCTION(MAREA_MathTransposeMatrix3x3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix3x3Float32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix3x3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix3x3Float32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Float32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix3x3Vector3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathIdentityMatrix3x3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix3x3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathTransposeMatrix3x3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix3x3Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix3x3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix3x3Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix3x3Vector3Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector3Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDeterminantMatrix3x3Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Float32));
MAREA_HOST_FUNCTION(MAREA_MathInverseMatrix3x3Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix3x3Float32), MAREA_HOST_BORROW(MAREA_MathMatrix3x3Float32));
MAREA_HOST_FUNCTION(MAREA_MathIdentityMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32));
MAREA_HOST_FUNCTION(MAREA_MathTransposeMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix4x4Vector4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathIdentityMatrix4x4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathTransposeMatrix4x4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix4x4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix4x4Vector4Sinteger32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Sinteger32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Sinteger32), MAREA_HOST_BORROW(MAREA_MathVector4Sinteger32));
MAREA_HOST_FUNCTION(MAREA_MathDeterminantMatrix4x4Float32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32));
MAREA_HOST_FUNCTION(MAREA_MathInverseMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32));
MAREA_HOST_FUNCTION(MAREA_MathTranslateMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathScaleMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathRotateMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32), MAREA_HOST_VALUE(float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathLookAtMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathPerspectiveMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathOrthographicMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathFrustumMatrix4x4Float32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathIdentityQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32));
MAREA_HOST_FUNCTION(MAREA_MathFromAxisAngleQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathFromEulerQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathToMatrix4x4QuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));
MAREA_HOST_FUNCTION(MAREA_MathConjugateQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));
MAREA_HOST_FUNCTION(MAREA_MathNormalizeQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));
MAREA_HOST_FUNCTION(MAREA_MathDotQuaternionFloat32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));
MAREA_HOST_FUNCTION(MAREA_MathLengthQuaternionFloat32, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));
MAREA_HOST_FUNCTION(MAREA_MathMixQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathSphericalMixQuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_MathRotateVector3QuaternionFloat32, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));

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

    MAREA_G_REGISTER(registry, except, "identity_matrix2x2_float32", MAREA_MathIdentityMatrix2x2Float32);

    MAREA_G_REGISTER(registry, except, "transpose_matrix2x2_float32", MAREA_MathTransposeMatrix2x2Float32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix2x2_float32", MAREA_MathMultiplyMatrix2x2Float32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix2x2_vector2_float32", MAREA_MathMultiplyMatrix2x2Vector2Float32);

    MAREA_G_REGISTER(registry, except, "identity_matrix2x2_sinteger32", MAREA_MathIdentityMatrix2x2Sinteger32);

    MAREA_G_REGISTER(registry, except, "transpose_matrix2x2_sinteger32", MAREA_MathTransposeMatrix2x2Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix2x2_sinteger32", MAREA_MathMultiplyMatrix2x2Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix2x2_vector2_sinteger32", MAREA_MathMultiplyMatrix2x2Vector2Sinteger32);

    MAREA_G_REGISTER(registry, except, "determinant_matrix2x2_float32", MAREA_MathDeterminantMatrix2x2Float32);

    MAREA_G_REGISTER(registry, except, "inverse_matrix2x2_float32", MAREA_MathInverseMatrix2x2Float32);

    MAREA_G_REGISTER(registry, except, "identity_matrix3x3_float32", MAREA_MathIdentityMatrix3x3Float32);

    MAREA_G_REGISTER(registry, except, "transpose_matrix3x3_float32", MAREA_MathTransposeMatrix3x3Float32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix3x3_float32", MAREA_MathMultiplyMatrix3x3Float32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix3x3_vector3_float32", MAREA_MathMultiplyMatrix3x3Vector3Float32);

    MAREA_G_REGISTER(registry, except, "identity_matrix3x3_sinteger32", MAREA_MathIdentityMatrix3x3Sinteger32);

    MAREA_G_REGISTER(registry, except, "transpose_matrix3x3_sinteger32", MAREA_MathTransposeMatrix3x3Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix3x3_sinteger32", MAREA_MathMultiplyMatrix3x3Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix3x3_vector3_sinteger32", MAREA_MathMultiplyMatrix3x3Vector3Sinteger32);

    MAREA_G_REGISTER(registry, except, "determinant_matrix3x3_float32", MAREA_MathDeterminantMatrix3x3Float32);

    MAREA_G_REGISTER(registry, except, "inverse_matrix3x3_float32", MAREA_MathInverseMatrix3x3Float32);

    MAREA_G_REGISTER(registry, except, "identity_matrix4x4_float32", MAREA_MathIdentityMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "transpose_matrix4x4_float32", MAREA_MathTransposeMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix4x4_float32", MAREA_MathMultiplyMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix4x4_vector4_float32", MAREA_MathMultiplyMatrix4x4Vector4Float32);

    MAREA_G_REGISTER(registry, except, "identity_matrix4x4_sinteger32", MAREA_MathIdentityMatrix4x4Sinteger32);

    MAREA_G_REGISTER(registry, except, "transpose_matrix4x4_sinteger32", MAREA_MathTransposeMatrix4x4Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix4x4_sinteger32", MAREA_MathMultiplyMatrix4x4Sinteger32);

    MAREA_G_REGISTER(registry, except, "multiply_matrix4x4_vector4_sinteger32", MAREA_MathMultiplyMatrix4x4Vector4Sinteger32);

    MAREA_G_REGISTER(registry, except, "determinant_matrix4x4_float32", MAREA_MathDeterminantMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "inverse_matrix4x4_float32", MAREA_MathInverseMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "translate_matrix4x4_float32", MAREA_MathTranslateMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "scale_matrix4x4_float32", MAREA_MathScaleMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "rotate_matrix4x4_float32", MAREA_MathRotateMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "look_at_matrix4x4_float32", MAREA_MathLookAtMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "perspective_matrix4x4_float32", MAREA_MathPerspectiveMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "orthographic_matrix4x4_float32", MAREA_MathOrthographicMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "frustum_matrix4x4_float32", MAREA_MathFrustumMatrix4x4Float32);

    MAREA_G_REGISTER(registry, except, "identity_quaternion_float32", MAREA_MathIdentityQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "from_axis_angle_quaternion_float32", MAREA_MathFromAxisAngleQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "from_euler_quaternion_float32", MAREA_MathFromEulerQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "to_matrix4x4_quaternion_float32", MAREA_MathToMatrix4x4QuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "multiply_quaternion_float32", MAREA_MathMultiplyQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "conjugate_quaternion_float32", MAREA_MathConjugateQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "normalize_quaternion_float32", MAREA_MathNormalizeQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "dot_quaternion_float32", MAREA_MathDotQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "length_quaternion_float32", MAREA_MathLengthQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "mix_quaternion_float32", MAREA_MathMixQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "spherical_mix_quaternion_float32", MAREA_MathSphericalMixQuaternionFloat32);

    MAREA_G_REGISTER(registry, except, "rotate_vector3_quaternion_float32", MAREA_MathRotateVector3QuaternionFloat32);

    return except;
}
