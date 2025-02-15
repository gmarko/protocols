from collections import namedtuple

FloatEncoding = namedtuple("FloatEncoding", "numBitsExponent numBitsMantissa exponentBase")
Float32Encoding = FloatEncoding(7, 25, 10)
Float31Encoding = FloatEncoding(7, 24, 10)
Float30Encoding = FloatEncoding(5, 25, 10)
Float29Encoding = FloatEncoding(5, 24, 10)
Float28Encoding = FloatEncoding(5, 23, 10)
Float24Encoding = FloatEncoding(5, 19, 10)
Float23Encoding = FloatEncoding(5, 18, 10)
Float16Encoding = FloatEncoding(5, 11, 10)
Float12Encoding = FloatEncoding(5,  7, 10)
Float8Encoding = FloatEncoding(5,  3, 10)

def toFloat(value, floatEncoding):
    maxPower = (1 << floatEncoding.numBitsExponent) - 1
    maxMantissa = (1 << floatEncoding.numBitsMantissa) - 1
    maxExponent = floatEncoding.exponentBase ** maxPower
    maxValue = maxMantissa * maxExponent
    assert value <= maxValue, "value too large: "

    exponent = 0
    r = value // maxMantissa
    d = 1
    while r >= floatEncoding.exponentBase or d * maxMantissa < value:
        r = r // floatEncoding.exponentBase
        exponent += 1
        d *= floatEncoding.exponentBase
    mantissa = value // d

    assert exponent <= maxExponent, "Exponent too large"
    assert mantissa <= maxMantissa, "Mantissa too large"
    f = (exponent << floatEncoding.numBitsMantissa) + mantissa
    return f

def fromFloat(f, floatEncoding):
    exponent = f >> floatEncoding.numBitsMantissa
    mantissa = f & ((1 << floatEncoding.numBitsMantissa) - 1)
    value = mantissa * (floatEncoding.exponentBase ** exponent)
    return value

def roundToFloatValue(value, encoding):
  f = toFloat(int(value), encoding)
  floatValue = fromFloat(f, encoding)
  return floatValue

def roundToFloatValueWithNegative(value, encoding):
    floatValue = 0
    if value > 0:
        f = toFloat(int(value), encoding)
        floatValue = fromFloat(f, encoding)
    else:
        f = toFloat(int(-value), encoding)
        floatValue = fromFloat(f, encoding)
        floatValue = -floatValue
    return floatValue