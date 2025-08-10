
#pragma once

#include <memory>
#include <string_view>
#include "pex/poly_value.h"
#include "pex/detail/poly_detail.h"


namespace pex
{


namespace poly
{


/**
 ** ControlSuper declares virtual methods that allow its derived classes to be
 ** in a pex::List. (These are mostly used internally by pex.)
 ** A user can add their own virtual interface with ControlUserBase.
 **/
template<typename ValueBase_, typename ControlUserBase>
class ControlSuper: public ControlUserBase
{
public:
    using ValueBase = ValueBase_;
    using Value = ::pex::poly::Value<ValueBase>;

    virtual ~ControlSuper() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;

    using Callable = std::function<void(void *, const Value &)>;
    virtual void Connect(void *observer, Callable callable) = 0;
    virtual void Disconnect(void *observer) = 0;

    virtual void SetValueWithoutNotify(const Value &) = 0;
    virtual void DoValueNotify() = 0;

    virtual std::unique_ptr<ControlSuper> Copy() const = 0;
};


/**
 ** ModelSuper declares virtual methods that allow its derived classes to be
 ** in a pex::List. (These are mostly used internally by pex.)
 ** A user can add their own virtual interface with ModelUserBase.
 **/
template<typename ValueBase_, typename ModelUserBase, typename ControlBase>
class ModelSuper: public ModelUserBase
{
public:
    using ValueBase = ValueBase_;
    using Value = ::pex::poly::Value<ValueBase>;
    using ControlPtr = std::unique_ptr<ControlBase>;

    virtual ~ModelSuper() {}
    virtual Value GetValue() const = 0;
    virtual void SetValue(const Value &) = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual ControlPtr CreateControl() = 0;
    virtual void SetValueWithoutNotify(const Value &) = 0;
    virtual void DoValueNotify() = 0;
};


} // end namespace poly


} // end namespace pex
