/**
  * @file reference.h
  *
  * @brief Provides access to Model/Control values by reference, delaying the
  * notification (if any) until editing is complete.
  *
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 14 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/
#pragma once


#include "pex/model_value.h"


namespace pex
{


/**
 ** While the Reference exists, the model's value has been changed, but not
 ** published.
 **
 ** The underlying value can be accessed directly using the dereference
 ** operator, but only for Values that do not use filters.
 **
 ** The new value will be published in the destructor.
 **/
template<typename Pex>
class Reference
{
public:
    using Type = typename Pex::Type;

    Reference(Pex &model)
        :
        pex_(model)
    {

    }

    Reference(const Reference &) = delete;
    Reference & operator=(const Reference &) = delete;

    Type & operator * ()
    {
        static_assert(
            std::is_same_v<NoFilter, typename Pex::Filter>,
            "Direct access to underlying value is incompatible with filters.");

        return this->pex_.value_;
    }

    Type Get() const
    {
        return this->pex_.Get();
    }

    void Set(ArgumentT<Type> value)
    {
        this->pex_.Set(value);
    }

protected:
    void SetWithoutNotify_(ArgumentT<Type> value)
    {
        this->pex_.SetWithoutNotify_(value);
    }

    void DoNotify_()
    {
        this->pex_.DoNotify_();
    }

private:
    Pex &pex_;
};


/**
 ** While the Defer exists, the model's value has been changed, but not
 ** published.
 **
 ** The new value will be published in the destructor.
 **/
template<typename Pex>
class Defer: public Reference<Pex>
{
public:
    using Base = Reference<Pex>;
    using Type = typename Base::Type;

    using Base::Base;

    void Set(ArgumentT<Type> value)
    {
        this->SetWithoutNotify_(value);
    }

    ~Defer()
    {
        // Notify on destruction
        this->DoNotify_();
    }
};


/**
 ** Allows direct access to the model value as a const reference, so there is
 ** no need to publish any new values.
 **
 ** It is not possible to create a ConstReference to a Value with a filter.
 **/
template<typename Model>
class ConstReference
{
    static_assert(
        IsModel<Model>,
        "Access to the value by reference is only possible for model values.");

    static_assert(
        std::is_same_v<NoFilter, typename Model::Filter>,
        "Direct access to underlying value is incompatible with filters.");

public:
    using Type = typename Model::Type;

    ConstReference(const Model &model)
        :
        model_(model)
    {

    }

    ConstReference(const ConstReference &) = delete;
    ConstReference & operator=(const ConstReference &) = delete;

    const Type & Get() const
    {
        return this->model_.value_;
    }

    const Type & operator * () const
    {
        return this->model_.value_;
    }

private:
    const Model &model_;
};


} // namespace pex
