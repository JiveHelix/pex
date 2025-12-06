
#pragma once

#include <memory>
#include <string_view>
#include "pex/value_wrapper.h"
#include "pex/detail/poly_detail.h"


namespace pex
{


namespace poly
{


/**
 ** SuperControlStencil declares virtual methods that allow its derived classes to be
 ** in a pex::List. (These are mostly used internally by pex.)
 ** A user can add their own virtual interface with ControlUserBase.
 **/
template<typename ValueBase_, typename ControlUserBase>
class SuperControlStencil: public ControlUserBase
{
public:
    using ValueBase = ValueBase_;
    using ValueWrapper = ::pex::poly::ValueWrapperTemplate<ValueBase>;

    virtual ~SuperControlStencil() {}
    virtual ValueWrapper GetValue() const = 0;
    virtual void SetValue(const ValueWrapper &) = 0;
    virtual std::string_view GetTypeName() const = 0;

    using Callable = std::function<void(void *, const ValueWrapper &)>;
    virtual void Connect(void *observer, Callable callable) = 0;
    virtual void Disconnect(void *observer) = 0;

    virtual void SetValueWithoutNotify(const ValueWrapper &) = 0;
    virtual void DoValueNotify() = 0;

    virtual std::unique_ptr<SuperControlStencil> Copy() const = 0;
};


/**
 ** SuperModelStencil declares virtual methods that allow its derived classes
 ** to be in a pex::List. (These are mostly used internally by pex.) A user can
 ** add their own virtual interface with ModelUserBase.
 **/
template<typename ValueBase_, typename ModelUserBase, typename ControlBase>
class SuperModelStencil: public ModelUserBase
{
public:
    using ValueBase = ValueBase_;
    using ValueWrapper = ::pex::poly::ValueWrapperTemplate<ValueBase>;
    using ControlPtr = std::unique_ptr<ControlBase>;

    virtual ~SuperModelStencil() {}
    virtual ValueWrapper GetValue() const = 0;
    virtual void SetValue(const ValueWrapper &) = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual ControlPtr CreateControl() = 0;
    virtual void SetValueWithoutNotify(const ValueWrapper &) = 0;
    virtual void DoValueNotify() = 0;
};


} // end namespace poly


} // end namespace pex
