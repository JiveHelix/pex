/**
  * @file reference.h
  *
  * @brief Provides access to Model/Interface values by reference, delaying the
  * notification (if any) until editing is complete.
  *
  * Does not work with any values that have filters applied.
  *
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 14 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/
#pragma once


namespace pex
{


/**
 ** While the Reference exists, the model's value has been changed, but not
 ** published.
 **
 ** The new value will be published in the destructor.
 **/
template<typename Pex>
class Reference
{
    static_assert(
        std::is_same_v<void, typename Pex::Filter>,
        "Direct access to underlying value is incompatible with filters.");

public:
    using Type = typename Pex::Type;

    Reference(Pex &pex)
        :
        pex_(pex)
    {

    }

    Reference(const Reference &) = delete;
    Reference & operator=(const Reference &) = delete;

    Type & Get()
    {
        return this->pex_.value_;
    }

    Type & operator * ()
    {
        return this->pex_.value_;
    }

    ~Reference()
    {
        // Notify on destruction
        this->pex_.Notify_(this->pex_.value_);
    }

private:
    Pex &pex_;
};


/**
 ** Allows direct access to the model value as a const reference, so there is
 ** no need to publish any new values.
 **/
template<typename Pex>
class ConstReference
{
    static_assert(
        std::is_same_v<void, typename Pex::Filter>,
        "Direct access to underlying value is incompatible with filters.");

public:
    using Type = typename Pex::Type;

    ConstReference(const Pex &pex)
        :
        pex_(pex)
    {

    }

    ConstReference(const ConstReference &) = delete;
    ConstReference & operator=(const ConstReference &) = delete;

    const Type & Get() const
    {
        return this->pex_.value_;
    }

    const Type & operator * () const
    {
        return this->pex_.value_;
    }

private:
    const Pex &pex_;
};


} // namespace pex
