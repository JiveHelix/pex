#pragma once


class AnyNotification
{
public:
    virtual ~AnyNotification() {}
    virtual void Notify() = 0;
};


template<typename Value>
class Notification: public AnyNotification
{
    Notification(Value &value): value_(value)
    {

    }

    void Notify() override
    {
        this->value_.DoNotify();
    }

private:
    Value &value_;
};


class NotificationQueue
{
public:
    NotificationQueue()
        :
        queue_()
    {

    }

    void Notify()
    {
        if (!this->queue_.empty())
        {
            for (auto &notification: this->queue_)
            {
                notification.Notify();
            }

            this->queue_.empty();
        }
    }

    template<typename Value>
    void Enqueue(Value &value)
    {
        this->queue_.push_back(Notification<Value>(value));
    }

private:
    std::vector<AnyNotification> queue_;
};
