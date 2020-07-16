#pragma once
#include <vector>
#include <set>
#include <type_traits>

#include "jive/compare.h"

namespace pex
{


template<typename Callable_>
class Callback_: jive::Compare<Callback_<Callable_>>
{
public:
    using Callable = Callable_;

    Callback_(void * const observer, Callable callable)
        :
        observer_(observer),
        callable_(callable)
    {

    }

    /** Conversion from observer pointer for comparisons. **/
    explicit Callback_(void * const observer)
        :
        observer_(observer),
        callable_{}
    {

    }
    
    Callback_(const Callback_ &other)
        :
        observer_(other.observer_),
        callable_(other.callable_)
    {

    }

    Callback_ & operator=(const Callback_ &other)
    {
        this->observer_ = other.observer_;
        this->callable_ = other.callable_;

        return *this;
    }

    /** Compare using the memory address of observer_ **/
    template<typename Operator>
    bool Compare(const Callback_<Callable> &other) const
    {
        return Operator::Call(this->observer_, other.observer_);
    }

protected:
    void * observer_;
    Callable callable_;
};


template<typename T>
using ValueCallable = void (*)(void * const observer, T value);

using SignalCallable = void (*)(void * const observer);


template<typename T>
class ValueCallback: public Callback_<ValueCallable<T>>
{
public:
    using Base = Callback_<ValueCallable<T>>;
    using Base::Base;

    void operator()(T value)
    {
        this->callable_(this->observer_, value); 
    }
};


class SignalCallback: public Callback_<SignalCallable>
{
public:
    using Base = Callback_<SignalCallable>;
    using Base::Base;

    void operator()()
    {
        this->callable_(this->observer_); 
    }
};


template<typename Callback>
class Tube
{
public:
    void Connect(void * const observer, typename Callback::Callable callable)
    {
        auto callback = Callback(observer, callable);
        
        // sorted insert
        this->callbacks_.insert(
            std::upper_bound(
                this->callbacks_.begin(),
                this->callbacks_.end(),
                callback),
            callback);
    }

    /** Remove all registered callbacks for the observer. **/
    void Disconnect(void * const observer)
    {
        auto [first, last] = std::equal_range(
            this->callbacks_.begin(),
            this->callbacks_.end(),
            Callback(observer));

        this->callbacks_.erase(first, last);
    }

protected:
    std::vector<Callback> callbacks_;
    std::set<Tube *> observed_;
};


template<typename T, typename Filter = void>
class ModelValue: public Tube<ValueCallback<T>>
{
public:
    using Type = T;
    
    void Set(T value)
    {
        if constexpr (std::is_void_v<Filter>)
        {
            this->value_ = value;
        }
        else
        {
            this->value_ = Filter::Call(value);
        }

        this->Notify_(this->value_);
    }

    T Get() const
    {
        return this->value_;
    }

    ModelValue(): value_{}
    {
        
    }

    ModelValue(T value): value_(value)
    {

    }

    ModelValue(const ModelValue<T, Filter> &) = delete;
    ModelValue(ModelValue<T, Filter> &&) = delete;

private:
    void Notify_(T value)
    {
        for (auto &callback: this->callbacks_)
        {
            callback(value);
        }
    }

    T value_;
};


template<typename ModelValue, typename InterfaceFilter = void>
class InterfaceValue: public Tube<ValueCallback<typename ModelValue::Type>>
{
public:
    using T = typename ModelValue::Type;

    InterfaceValue(ModelValue * const model)
        :
        model_(model)
    {
        this->model_->Connect(this, &InterfaceValue::OnModelChanged_);
    }

    ~InterfaceValue()
    {
        this->model_->Disconnect(this);
    }

    InterfaceValue(const InterfaceValue &other)
        :
        model_(other.model_)
    {
        this->model_->Connect(this, &InterfaceValue::OnModelChanged_);
    }

    /** 
     ** Assignment could cause an InterfaceValue to track a different model
     ** value.
     **/
    InterfaceValue & operator=(const InterfaceValue &) = delete;
    InterfaceValue & operator=(InterfaceValue &&) = delete;
    
    T Get() const
    {
        if constexpr (std::is_void_v<InterfaceFilter>)
        {
            return this->model_->Get();
        }
        else
        {
            return InterfaceFilter::Get(this->model_->Get());
        }
    }

    void Set(T value)
    {
        if constexpr (std::is_void_v<InterfaceFilter>)
        {
            this->model_->Set(value);
        }
        else
        {
            this->model_->Set(InterfaceFilter::Set(value));
        }
    }

private:
    static void OnModelChanged_(void * observer, T value)
    {
        // The model value has changed.
        // Update our observers.
        if constexpr (!std::is_void_v<InterfaceFilter>)
        {
            value = InterfaceFilter::Get(value);
        }
        
        auto self = static_cast<InterfaceValue *>(observer);
        for (auto &callback: self->callbacks_)
        {
            callback(value);
        }
    }

    ModelValue *model_;
};

} // namespace pex
