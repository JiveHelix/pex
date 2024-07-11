#pragma once


#include <memory>
#include "pex/error.h"


namespace pex
{


namespace poly
{


CREATE_EXCEPTION(PolyError, PexError);


template<typename ValueBase_>
class Value
{
public:
    using ValueBase = ValueBase_;
    static constexpr auto fieldsTypeName = ValueBase::polyTypeName;

    Value()
        :
        value_{}
    {

    }

    Value(const std::shared_ptr<ValueBase> &value)
        :
        value_(value)
    {

    }

    Value(const ValueBase &value)
        :
        value_(value.Copy())
    {

    }

    std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const
    {
        if (!this->value_)
        {
            throw std::logic_error("Unitialized member");
        }

        return this->value_->Describe(outputStream, style, indent);
    }

    template<typename Json>
    Json Unstructure() const
    {
        static_assert(
            std::is_same_v<Json, typename ValueBase::Json>,
            "Must match ValueBase Json type");

        if (!this->value_)
        {
            throw std::logic_error("Unitialized member");
        }

        return this->value_->Unstructure();
    }

    template<typename Json>
    static Value Structure(const Json &jsonValues)
    {
        return {ValueBase::Structure(jsonValues)};
    }

    template<typename Derived, typename ...Args>
    void Initialize(Args &&...args)
    {
        this->value_ = std::make_shared<Derived>(std::forward<Args>(args)...);
    }

    Value & operator=(std::shared_ptr<ValueBase> value)
    {
        this->value_ = value;
        return *this;
    }

    operator bool () const
    {
        return !!this->value_;
    }

    std::shared_ptr<const ValueBase> GetValueBase() const
    {
        return this->value_;
    }

    std::shared_ptr<ValueBase> GetValueBase()
    {
        return this->value_;
    }

    bool operator==(const Value &other) const
    {
        if (!(this->value_ && other.value_))
        {
            return false;
        }

        return this->value_->operator==(*other.value_);
    }

    template<typename Derived>
    const Derived * GetDerived() const
    {
        const auto base = this->value_.get();
        return dynamic_cast<const Derived *>(base);
    }

    template<typename Derived>
    const Derived & RequireDerived() const
    {
        auto derived = this->template GetDerived<Derived>();

        if (!derived)
        {
            throw PolyError("Mismatched polymorphic value");
        }

        return *derived;
    }

private:
    std::shared_ptr<ValueBase> value_;
};


} // end namespace poly


} // end namespace pex
