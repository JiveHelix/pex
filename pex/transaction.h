/**
  * @file transaction.h
  * 
  * @brief A class that allows modifying a model value with out publishing, and
  * reverting to the original value on destruction if Commit is not called.
  *
  * Allows your controller to delay the notification of any values until all
  * have been set.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once


#include "pex/detail/not_null.h"


namespace pex
{


/**
 ** While Transaction exists, the model's value has been changed, but not
 ** published.
 **
 ** When you are ready to publish, call Commit.
 **
 ** If the Transaction goes out of scope without a call to Commit, the model
 ** value is reverted, and nothing is published.
 **
 **/
template<typename Model>
class Transaction
{
public:
    using Type = typename Model::Type;

    Transaction(Model &model)
        :
        model_(&model),
        oldValue_(model.Get())
    {

    }

    Transaction(Model &model, typename detail::Argument<Type>::Type value)
        :
        model_(&model),
        oldValue_(model.Get())
    {
        this->model_->SetWithoutNotify_(value);
    }

    Transaction(const Transaction &) = delete;
    Transaction & operator=(const Transaction &) = delete;

    Type & operator * ()
    {
        static_assert(
            std::is_same_v<void, typename Model::Filter>,
            "Direct access to underlying value is incompatible with filters.");

        NOT_NULL(this->model_);
        return this->model_->value_;
    }

    Type Get() const
    {
        NOT_NULL(this->model_);
        return this->model_->Get();
    }

    void Set(typename detail::Argument<Type>::Type value)
    {
        NOT_NULL(this->model_);
        this->model_->SetWithoutNotify_(value);
    }

    void Commit()
    {
        if (nullptr != this->model_)
        {
            this->model_->Notify_(this->model_->value_);
            this->model_ = nullptr;
        }
    }

    ~Transaction()
    {
        // Revert on destruction
        if (nullptr != this->model_)
        {
            this->model_->SetWithoutNotify_(this->oldValue_);
        }
    }

    Model *model_;
    Type oldValue_;
};


} // namespace pex
