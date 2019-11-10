#ifndef FIXPOINT_HPP
#define FIXPOINT_HPP
#include <array>
#include <limits>
#include <type_traits>

#include <stdint.h>


class Fixpoint final
{
public:
    enum RepresentationTag { REPRESENTATION };

    // Getting attributes
    static constexpr uint32_t GetPrecision() noexcept { return 4u; }
    static constexpr  int64_t GetFactor() noexcept { return 10000; }

    static constexpr Fixpoint Min() noexcept { return Fixpoint(std::numeric_limits<int64_t>::min(), REPRESENTATION); }
    static constexpr Fixpoint Max() noexcept { return Fixpoint(std::numeric_limits<int64_t>::max(), REPRESENTATION); }

    // Accept default copy/assign/move
    Fixpoint(Fixpoint const &) noexcept = default;
    Fixpoint(Fixpoint &&) noexcept = default;
    Fixpoint & operator =(Fixpoint const &) noexcept = default;
    Fixpoint & operator =(Fixpoint &&) noexcept = default;

    // Constructors
    constexpr Fixpoint() noexcept : mValue(0) { }
    constexpr Fixpoint(std::int64_t value, RepresentationTag) noexcept : mValue(value) { }

    template <typename T>
    constexpr explicit Fixpoint(T value) noexcept : mValue(Convert(value)) { }

    template <typename T>
    constexpr explicit operator T() const noexcept
    {
        return Cast<T>();
    }

    // Access to representation
    constexpr int64_t GetRepresentation() const noexcept { return mValue; }

    // Multiply/divide primitives
    constexpr Fixpoint Multiply(Fixpoint num) const noexcept
    {
        return Fixpoint(MultiplyImpl(CalcAbs(mValue), CalcAbs(num.mValue)) ^ GetSign(mValue, num.mValue), REPRESENTATION);
    }
    constexpr Fixpoint Divide(Fixpoint num) const noexcept
    {
        return Fixpoint(DivideImpl(CalcAbs(mValue), CalcAbs(num.mValue)) ^ GetSign(mValue, num.mValue), REPRESENTATION);
    }

    // Unary sign operators
    constexpr Fixpoint operator +() const noexcept { return *this; }
    constexpr Fixpoint operator -() const noexcept { return Fixpoint(-mValue, REPRESENTATION); }

    // Relational operators for same type with trivial implementation
    constexpr bool operator ==(Fixpoint num) const noexcept { return mValue == num.mValue; }
    constexpr bool operator <(Fixpoint num) const noexcept { return mValue < num.mValue; }

    // Relational operators for other numeric types
    template <typename T> 
    constexpr typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator ==(T num) const noexcept
    {
        return *this == Fixpoint(num);
    }
    template <typename T>
    constexpr typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator <(T num) const noexcept
    {
        return *this < Fixpoint(num);
    }

    // Relational operators implemented in terms of other operators
    template <typename T> constexpr bool operator !=(T num) const noexcept { return not(*this == num); }
    template <typename T> constexpr bool operator > (T num) const noexcept { return num < *this; }
    template <typename T> constexpr bool operator <=(T num) const noexcept { return not(num < *this); }
    template <typename T> constexpr bool operator >=(T num) const noexcept { return not(*this < num); }


    constexpr Fixpoint operator +(Fixpoint num) const noexcept { return Fixpoint(mValue + num.mValue, REPRESENTATION); }
    Fixpoint & operator +=(Fixpoint num) noexcept { return *this = *this + num; }

    template <typename T>
    constexpr typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type operator +(T num) const noexcept
    {
        return *this + Fixpoint(num);
    }
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type & operator +=(T num) noexcept
    {
        return *this += Fixpoint(num);
    }


    constexpr Fixpoint operator -(Fixpoint num) const noexcept { return Fixpoint(mValue - num.mValue, REPRESENTATION);}
    Fixpoint & operator -=(Fixpoint num) noexcept { return *this = *this - num; }

    template <typename T>
    constexpr typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type operator -(T num) const noexcept
    {
        return *this - Fixpoint(num);
    }
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type & operator -=(T num) noexcept
    {
        return *this -= Fixpoint(num);
    }


    constexpr Fixpoint operator *(Fixpoint num) const noexcept { return Multiply(num); }
    Fixpoint & operator *=(Fixpoint num) noexcept { return *this = Multiply(num); }

    template <typename T>
    constexpr typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type operator *(T num) const noexcept
    {
        return *this * Fixpoint(num);
    }

    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type & operator *=(T num) noexcept
    {
        return *this *= Fixpoint(num);
    }


    constexpr Fixpoint operator /(Fixpoint num) const noexcept { return Divide(num); }
    Fixpoint & operator /=(Fixpoint num) noexcept { return *this = Divide(num); }

    template <typename T>
    constexpr typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type operator /(T num) const noexcept
    {
        return *this / Fixpoint(num);
    }

    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, Fixpoint>::type & operator /=(T num) noexcept
    {
        return *this /= Fixpoint(num);
    }


    constexpr Fixpoint Abs() const noexcept { return Fixpoint(CalcAbs(mValue), REPRESENTATION); }

    std::array<char, 24> ToString() const noexcept;

private:
    static constexpr int64_t CalcAbs(int64_t value) noexcept { return (value & std::numeric_limits<int64_t>::min()) ^ value; }
    static constexpr int64_t GetSign(int64_t v1, int64_t v2) noexcept { return (v1 ^ v2) & std::numeric_limits<int64_t>::min(); }

    template <typename T>
    static constexpr typename std::enable_if<std::is_unsigned<T>::value, int64_t>::type Convert(T value) noexcept
    {
        return int64_t(uint64_t(value)) * GetFactor();
    }

    template <typename T>
    static constexpr typename std::enable_if<std::is_signed<T>::value and std::is_integral<T>::value, int64_t>::type Convert(T value) noexcept
    {
        return int64_t(value) * GetFactor();
    }

    template <typename T>
    static constexpr typename std::enable_if<std::is_floating_point<T>::value, int64_t>::type Convert(T value) noexcept
    {
        return int64_t(static_cast<long double>(value) * GetFactor() + 0.5 - int(value < 0));
    }

    static constexpr int64_t MultiplyImpl(int64_t v1, int64_t v2) noexcept
    {
        return v1 / GetFactor() * v2 + v2 / GetFactor() * (v1 % GetFactor()) + (v1 % GetFactor()) * (v2 % GetFactor()) / GetFactor();
    }

    static constexpr int64_t DivideImpl(int64_t v1, int64_t v2) noexcept
    {
        return v1 / v2 * GetFactor() + v1 % v2 * GetFactor() / v2;
    }

    template <typename T>
    constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type Cast() const noexcept
    {
        return T(uint64_t(mValue / GetFactor()));
    }

    template <typename T>
    constexpr typename std::enable_if<std::is_signed<T>::value and std::is_integral<T>::value, T>::type Cast() const noexcept
    {
        return T(mValue / GetFactor());
    }

    template <typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type Cast() const noexcept
    {
        return T(mValue / GetFactor()) + T(mValue % GetFactor()) / GetFactor();
    }

    std::int64_t mValue;

    template <typename Stream>
    friend Stream & operator <<(Stream &stream, Fixpoint f) noexcept
    {
        stream << f.ToString().data();
        return stream;
    }
};

template <typename T>
constexpr inline typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator ==(T num, Fixpoint f) noexcept
{
    return f == num;
}

template <typename T>
constexpr inline typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator !=(T num, Fixpoint f) noexcept
{
    return f != num;
}

template <typename T>
constexpr inline typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator <(T num, Fixpoint f) noexcept
{
    return f > num;
}

template <typename T>
constexpr inline typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator <=(T num, Fixpoint f) noexcept
{
    return f >= num;
}

template <typename T>
constexpr inline typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator >(T num, Fixpoint f) noexcept
{
    return f < num;
}

template <typename T>
constexpr inline typename std::enable_if<std::is_arithmetic<T>::value, bool>::type operator >=(T num, Fixpoint f) noexcept
{
    return f <= num;
}

#endif // FIXPOINT_HPP

