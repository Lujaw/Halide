#include "FastIntegerDivide.h"
#include "IntegerDivisionTable.h"

namespace Halide {

using namespace Halide::Internal::IntegerDivision;

namespace IntegerDivideTable {
Image<uint8_t> integer_divide_table_u8() {
    static Image<uint8_t> im(256, 2);
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        for (size_t i = 0; i < 256; i++) {
            im(i, 0) = table_runtime_u8[i][2];
            im(i, 1) = table_runtime_u8[i][3];
        }
    }
    return im;
}

Image<uint8_t> integer_divide_table_s8() {
    static Image<uint8_t> im(256, 2);
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        for (size_t i = 0; i < 256; i++) {
            im(i, 0) = table_runtime_s8[i][2];
            im(i, 1) = table_runtime_s8[i][3];
        }
    }
    return im;
}

Image<uint16_t> integer_divide_table_u16() {
    static Image<uint16_t> im(256, 2);
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        for (size_t i = 0; i < 256; i++) {
            im(i, 0) = table_runtime_u16[i][2];
            im(i, 1) = table_runtime_u16[i][3];
        }
    }
    return im;
}

Image<uint16_t> integer_divide_table_s16() {
    static Image<uint16_t> im(256, 2);
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        for (size_t i = 0; i < 256; i++) {
            im(i, 0) = table_runtime_s16[i][2];
            im(i, 1) = table_runtime_s16[i][3];
        }
    }
    return im;
}

Image<uint32_t> integer_divide_table_u32() {
    static Image<uint32_t> im(256, 2);
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        for (size_t i = 0; i < 256; i++) {
            im(i, 0) = table_runtime_u32[i][2];
            im(i, 1) = table_runtime_u32[i][3];
        }
    }
    return im;
}

Image<uint32_t> integer_divide_table_s32() {
    static Image<uint32_t> im(256, 2);
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        for (size_t i = 0; i < 256; i++) {
            im(i, 0) = table_runtime_s32[i][2];
            im(i, 1) = table_runtime_s32[i][3];
        }
    }
    return im;
}
}

Expr fast_integer_divide(Expr numerator, Expr denominator) {
    if (is_const(denominator)) {
        // There's code elsewhere for this case.
        return numerator / denominator;
    }
    assert(denominator.type() == UInt(8));
    Type t = numerator.type();
    assert((t.is_uint() || t.is_int()) && "Argument to fast_integer divide must be an integer");
    assert((t.bits == 8 || t.bits == 16 || t.bits == 32) && "Argument to integer divide must be 8, 16, or 32-bit");

    Type wide = t;
    wide.bits *= 2;

    Expr result;
    if (t.is_uint()) {
        Expr mul, shift;
        switch(t.bits) {
        case 8:
        {
            Image<uint8_t> table = IntegerDivideTable::integer_divide_table_u8();
            mul = table(denominator, 0);
            shift = table(denominator, 1);
            break;
        }
        case 16:
        {
            Image<uint16_t> table = IntegerDivideTable::integer_divide_table_u16();
            mul = table(denominator, 0);
            shift = table(denominator, 1);
            break;
        }
        default: // 32
        {
            Image<uint32_t> table = IntegerDivideTable::integer_divide_table_u32();
            mul = table(denominator, 0);
            shift = table(denominator, 1);
            break;
        }
        }

        // Multiply-keep-high-half
        result = (cast(wide, mul) * numerator);

        if (t.bits < 32) result = result / (1 << t.bits);
        else result = result >> t.bits;

        result = cast(t, result);

        // Add half the difference between input and output so far
        result = result + (numerator - result)/2;

        // Do a final shift
        result = result >> shift;


    } else {

        Expr mul, shift;
        switch(t.bits) {
        case 8:
        {
            Image<uint8_t> table = IntegerDivideTable::integer_divide_table_s8();
            mul = table(denominator, 0);
            shift = table(denominator, 1);
            break;
        }
        case 16:
        {
            Image<uint16_t> table = IntegerDivideTable::integer_divide_table_s16();
            mul = table(denominator, 0);
            shift = table(denominator, 1);
            break;
        }
        default: // 32
        {
            Image<uint32_t> table = IntegerDivideTable::integer_divide_table_s32();
            mul = table(denominator, 0);
            shift = table(denominator, 1);
            break;
        }
        }

        // Extract sign bit
        //Expr xsign = (t.bits < 32) ? (numerator / (1 << (t.bits-1))) : (numerator >> (t.bits-1));
        Expr xsign = select(numerator > 0, cast(t, 0), cast(t, -1));

        // If it's negative, flip the bits of the
        // numerator. Equivalent to:
        // select(numerator < 0, -(numerator+1), numerator);
        numerator = xsign ^ numerator;

        // Multiply-keep-high-half
        result = (cast(wide, mul) * numerator);
        if (t.bits < 32) result = result / (1 << t.bits);
        else result = result >> t.bits;
        result = cast(t, result);

        // Do the final shift
        result = result >> shift;

        // Maybe flip the bits again
        result = xsign ^ result;
    }

    // The tables don't work for denominator == 1
    result = select(denominator == 1, numerator, result);

    return result;

}
}

