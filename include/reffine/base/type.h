#ifndef INCLUDE_REFFINE_BASE_TYPE_H_
#define INCLUDE_REFFINE_BASE_TYPE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "reffine/base/log.h"

using namespace std;

namespace reffine {

enum class BaseType {
    UNKNOWN,  // never use this type
    BOOL,
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT32,
    FLOAT64,
    STRUCT,
    PTR,

    // Reffine IR types
    IDX,
    VECTOR,
};

struct DataType {
    const BaseType btype;
    const vector<DataType> dtypes;
    const size_t dim;

    explicit DataType(BaseType btype, vector<DataType> dtypes = {},
                      size_t dim = 0)
        : btype(btype), dtypes(dtypes), dim(dim)
    {
        switch (btype) {
            case BaseType::STRUCT:
                ASSERT(dim == 0);
                ASSERT(dtypes.size() > 0);
                break;
            case BaseType::PTR:
                ASSERT(dim == 0);
                ASSERT(dtypes.size() == 1);
                break;
            case BaseType::VECTOR:
                ASSERT(dim > 0);
                ASSERT(dtypes.size() >= dim);
                break;
            default:
                ASSERT(dtypes.size() == 0);
                ASSERT(dim == 0);
                break;
        }
    }

    bool operator==(const DataType& o) const
    {
        return (this->btype == o.btype) && (this->dtypes == o.dtypes) &&
               (this->dim == o.dim);
    }

    bool is_struct() const { return btype == BaseType::STRUCT; }
    bool is_ptr() const { return btype == BaseType::PTR; }
    bool is_idx() const { return btype == BaseType::IDX; }
    bool is_vector() const { return btype == BaseType::VECTOR; }

    bool is_val() const { return !(this->is_vector()); }

    bool is_float() const
    {
        return (this->btype == BaseType::FLOAT32) ||
               (this->btype == BaseType::FLOAT64);
    }

    bool is_primitive() const
    {
        return (this->is_int() || this->is_float() || this->is_idx() ||
                this->btype == BaseType::BOOL);
    }

    bool is_int() const
    {
        return (this->btype == BaseType::INT8) ||
               (this->btype == BaseType::INT16) ||
               (this->btype == BaseType::INT32) ||
               (this->btype == BaseType::INT64) ||
               (this->btype == BaseType::UINT8) ||
               (this->btype == BaseType::UINT16) ||
               (this->btype == BaseType::UINT32) ||
               (this->btype == BaseType::UINT64);
    }

    bool is_signed() const
    {
        return (this->btype == BaseType::INT8) ||
               (this->btype == BaseType::INT16) ||
               (this->btype == BaseType::INT32) ||
               (this->btype == BaseType::INT64) ||
               (this->btype == BaseType::FLOAT32) ||
               (this->btype == BaseType::FLOAT64);
    }

    DataType ptr() const { return DataType(BaseType::PTR, {*this}); }

    DataType deref() const
    {
        ASSERT(this->is_ptr());
        return this->dtypes[0];
    }

    DataType valty() const
    {
        ASSERT(this->is_vector());
        return DataType(BaseType::STRUCT,
                        std::vector<DataType>(this->dtypes.begin() + this->dim,
                                              this->dtypes.end()));
    }

    string str() const
    {
        switch (btype) {
            case BaseType::BOOL:
                return "b";
            case BaseType::INT8:
                return "i8";
            case BaseType::UINT8:
                return "u8";
            case BaseType::INT16:
                return "i16";
            case BaseType::UINT16:
                return "u16";
            case BaseType::INT32:
                return "i32";
            case BaseType::UINT32:
                return "u32";
            case BaseType::INT64:
                return "i64";
            case BaseType::UINT64:
                return "u64";
            case BaseType::FLOAT32:
                return "f32";
            case BaseType::FLOAT64:
                return "f64";
            case BaseType::PTR:
                return "*" + dtypes[0].str();
            case BaseType::STRUCT: {
                string res = "";
                for (const auto& dtype : dtypes) { res += dtype.str() + ", "; }
                res.resize(res.size() - 2);
                return "{" + res + "}";
            }
            case BaseType::IDX:
                return "x";
            case BaseType::VECTOR:
                return "~{" + dtypes[0].str() + "}";
            default:
                throw std::runtime_error("Invalid type");
        }
    }
};

enum class MathOp {
    ADD,
    SUB,
    MUL,
    DIV,
    MAX,
    MIN,
    MOD,
    SQRT,
    POW,
    ABS,
    NEG,
    CEIL,
    FLOOR,
    LT,
    LTE,
    GT,
    GTE,
    EQ,
    NOT,
    AND,
    OR,
    IMPLIES,
    FORALL,
    EXISTS,
};

}  // namespace reffine

namespace reffine::types {

static const DataType UNKNOWN(BaseType::UNKNOWN);
static const DataType BOOL(BaseType::BOOL);
static const DataType INT8(BaseType::INT8);
static const DataType INT16(BaseType::INT16);
static const DataType INT32(BaseType::INT32);
static const DataType INT64(BaseType::INT64);
static const DataType UINT8(BaseType::UINT8);
static const DataType UINT16(BaseType::UINT16);
static const DataType UINT32(BaseType::UINT32);
static const DataType UINT64(BaseType::UINT64);
static const DataType FLOAT32(BaseType::FLOAT32);
static const DataType FLOAT64(BaseType::FLOAT64);
static const DataType CHAR_PTR(BaseType::PTR, {types::INT8});
static const DataType IDX(BaseType::IDX);

template <typename H>
struct Converter {
    static const BaseType btype = BaseType::UNKNOWN;
};
template <>
struct Converter<bool> {
    static const BaseType btype = BaseType::BOOL;
};
template <>
struct Converter<char> {
    static const BaseType btype = BaseType::INT8;
};
template <>
struct Converter<int8_t> {
    static const BaseType btype = BaseType::INT8;
};
template <>
struct Converter<int16_t> {
    static const BaseType btype = BaseType::INT16;
};
template <>
struct Converter<int32_t> {
    static const BaseType btype = BaseType::INT32;
};
template <>
struct Converter<int64_t> {
    static const BaseType btype = BaseType::INT64;
};
template <>
struct Converter<uint8_t> {
    static const BaseType btype = BaseType::UINT8;
};
template <>
struct Converter<uint16_t> {
    static const BaseType btype = BaseType::UINT16;
};
template <>
struct Converter<uint32_t> {
    static const BaseType btype = BaseType::UINT32;
};
template <>
struct Converter<uint64_t> {
    static const BaseType btype = BaseType::UINT64;
};
template <>
struct Converter<float> {
    static const BaseType btype = BaseType::FLOAT32;
};
template <>
struct Converter<double> {
    static const BaseType btype = BaseType::FLOAT64;
};

template <size_t n>
static void convert(BaseType* btypes)
{
}

template <size_t n, typename H, typename... Ts>
static void convert(BaseType* btypes)
{
    btypes[n - sizeof...(Ts) - 1] = Converter<H>::btype;
    convert<n, Ts...>(btypes);
}

template <typename... Ts>
DataType STRUCT()
{
    vector<BaseType> btypes(sizeof...(Ts));
    convert<sizeof...(Ts), Ts...>(btypes.data());

    vector<DataType> dtypes;
    for (const auto& btype : btypes) { dtypes.push_back(DataType(btype)); }
    return DataType(BaseType::STRUCT, dtypes);
}

template <size_t dim>
DataType VECTOR(vector<DataType> types)
{
    return DataType(BaseType::VECTOR, types, dim);
}

template <size_t dim, typename... Ts>
DataType VEC()
{
    vector<BaseType> btypes(sizeof...(Ts));
    convert<sizeof...(Ts), Ts...>(btypes.data());

    vector<DataType> dtypes;
    for (const auto& btype : btypes) { dtypes.push_back(DataType(btype)); }
    return DataType(BaseType::VECTOR, dtypes, dim);
}

}  // namespace reffine::types

#endif  // INCLUDE_REFFINE_BASE_TYPE_H_
