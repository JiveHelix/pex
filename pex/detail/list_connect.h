#pragma once

#include "pex/list.h"
#include "pex/selectors.h"
#include "pex/make_control.h"
#include "pex/detail/forward.h"


namespace pex
{


template<typename T>
concept HasControlType = requires
{
    typename T::ControlType;
};


template<typename T, typename Enable = void>
struct GetControlType_
{
    using Type = pex::control::Value<T>;
};


template<typename T>
struct GetControlType_<T, std::enable_if_t<HasControlType<T>>>
{
    using Type = typename T::ControlType;
};


template<typename T>
using GetControlType = typename GetControlType_<T>::Type;


// Specializations of MakeControl for List.
template<typename P>
struct MakeControl<P, std::enable_if_t<IsListModel<P>>>
{
    using Control = typename P::ControlType;
    using Upstream = P;
};


template<typename P>
struct MakeControl<P, std::enable_if_t<IsListControl<P>>>
{
    using Control = P;
    using Upstream = typename P::Upstream;
};


namespace detail
{

template<typename T, typename Enable = void>
struct ConnectableSelector_
{
    using Type = typename MakeControl<T>::Control;
};

template<typename T>
struct ConnectableSelector_<T, std::enable_if_t<IsGroupModel<T>>>
{
    using Type = GroupConnect<void, typename T::ControlType>;
};

template<typename T>
struct ConnectableSelector_<T, std::enable_if_t<IsGroupControl<T>>>
{
    using Type = GroupConnect<void, T>;
};

template<typename T>
struct ConnectableSelector_<T, std::enable_if_t<IsListControl<T>>>
{
    using Type = ListConnect<void, T>;
};


template<typename T>
using ConnectableSelector = typename ConnectableSelector_<T>::Type;


template<typename Observer, typename Upstream_>
class ListConnect
{
public:
    static_assert(::pex::IsListNode<Upstream_>);

    static constexpr auto observerName = "pex::ListConnect";
    static constexpr bool isListConnect = true;

    using ListControl = typename MakeControl<Upstream_>::Control;

    using Connectable =
        ConnectableSelector<typename ListControl::ListItem>;

    using Connectables = std::map<size_t, Connectable>;

    using UpstreamControl = ListControl;
    using Upstream = typename MakeControl<Upstream_>::Upstream;

    using ListType = typename ListControl::Type;
    using Plain = ListType;
    using Type = Plain;

    using Item = typename ListControl::Item;

    using MemberWillRemoveTerminus =
        Terminus<ListConnect, typename ListControl::MemberWillRemove>;

    using MemberAddedTerminus =
        ::pex::Terminus<ListConnect, typename ListControl::MemberAdded>;

    using ListFlagTerminus =
        ::pex::Terminus<ListConnect, typename ListControl::ListFlag>;

    using ValueConnection_ = ValueConnection<Observer, ListType>;
    using ValueCallable = typename ValueConnection_::Callable;
    using Callable = ValueCallable;

    using SignalConnection_ = SignalConnection<Observer>;
    using SignalCallable = typename SignalConnection_::Callable;

    static_assert(std::is_same_v<Item, typename Connectable::Plain>);

    ListConnect()
        :
        muteTerminus_(),
        muteState_(),
        listControl_(),
        connectables_(),
        observer_(nullptr),
        valueConnection_(),
        signalConnection_(),
        memberWillRemoveTerminus_(),
        memberAddedTerminus_(),
        isNotifying_(false),
        isNotifyingTerminus_(),
        cached_()
    {

    }

    ListConnect(
        Observer *observer,
        const ListControl &listControl)
        :
        muteTerminus_(
            this,
            listControl.CloneMuteControl(),
            &ListConnect::OnMute_),
        muteState_(listControl.CloneMuteControl().Get()),
        listControl_(listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(),
        signalConnection_(),

        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_(this->listControl_.Get())
    {
        REGISTER_PEX_NAME(this, "ListConnect");
    }

    ListConnect(
        Observer *observer,
        const ListControl &listControl,
        ValueCallable callable)
        :
        muteTerminus_(
            this,
            listControl.CloneMuteControl(),
            &ListConnect::OnMute_),
        muteState_(listControl.CloneMuteControl().Get()),
        listControl_(listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(std::in_place_t{}, observer, callable),
        signalConnection_(),

        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_(this->listControl_.Get())
    {
        REGISTER_PEX_NAME(this, "ListConnect");
        this->MakeListConnections_();
    }

    ListConnect(
        Observer *observer,
        const ListControl &listControl,
        SignalCallable callable)
        :
        muteTerminus_(
            this,
            listControl.CloneMuteControl(),
            &ListConnect::OnMute_),
        muteState_(listControl.CloneMuteControl().Get()),
        listControl_(listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(),
        signalConnection_(std::in_place_t{}, observer, callable),

        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_(this->listControl_.Get())
    {
        REGISTER_PEX_NAME(this, "ListConnect");
        this->MakeListConnections_();
    }

    ListConnect(
        Observer *observer,
        Upstream &upstream)
        :
        ListConnect(observer, ListControl(upstream))
    {

    }

    ListConnect(Upstream &upstream)
        :
        ListConnect(ListControl(upstream))
    {

    }

    ListConnect(const ListControl &listControl)
        :
        ListConnect(nullptr, listControl)
    {

    }

    ListConnect(
        Observer *observer,
        Upstream &upstream,
        ValueCallable callable)
        :
        ListConnect(observer, ListControl(upstream), callable)
    {

    }

    ListConnect(
        Observer *observer,
        Upstream &upstream,
        SignalCallable callable)
        :
        ListConnect(observer, ListControl(upstream), callable)
    {

    }

    ListConnect(Observer *observer, const ListConnect &other)
        :
        muteTerminus_(
            this,
            other.listControl_.CloneMuteControl(),
            &ListConnect::OnMute_),
        muteState_(other.muteState_),
        listControl_(other.listControl_),
        connectables_(),
        observer_(observer),
        valueConnection_(),
        signalConnection_(),
        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_(this->listControl_.Get())
    {
        REGISTER_PEX_NAME(this, "ListConnect");

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());
        }

        if (other.signalConnection_)
        {
            this->signalConnection_.emplace(
                observer,
                other.signalConnection_->GetCallable());
        }

        if (other.valueConnection_ || other.signalConnection_)
        {
            this->MakeListConnections_();
        }
    }

    ListConnect & operator=(const ListConnect &other)
    {
        return this->Assign(other.observer_, other);
    }

    ListConnect(const ListConnect &other)
        :
        ListConnect(other.observer_, other)
    {
        REGISTER_PEX_NAME(this, "ListConnect");
    }

    ListConnect & Assign(Observer *observer, const ListConnect &other)
    {
        this->muteTerminus_.RequireAssign(this, other.muteTerminus_);
        this->muteState_ = other.muteState_;
        this->ClearListConnections_();

        this->listControl_ = other.listControl_;
        this->observer_ = observer;

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());
        }

        if (other.signalConnection_)
        {
            this->signalConnection_.emplace(
                observer,
                other.signalConnection_->GetCallable());
        }

        if (other.valueConnection_ || other.signalConnection_)
        {
            this->MakeListConnections_();
        }

        this->memberWillRemoveTerminus_.RequireAssign(
            this,
            other.memberWillRemoveTerminus_);

        this->memberAddedTerminus_.RequireAssign(
            this,
            other.memberAddedTerminus_);

        this->isNotifying_ = other.isNotifying_;

        this->isNotifyingTerminus_.RequireAssign(
            this,
            other.isNotifyingTerminus_);

        this->cached_ = other.cached_;

        return *this;
    }

    void Connect(ValueCallable callable)
    {
        if (!this->observer_)
        {
            throw std::runtime_error("ListConnect has no observer.");
        }

        this->valueConnection_.emplace(this->observer_, callable);

        if (this->connectables_.empty())
        {
            this->MakeListConnections_();
        }
    }

    void Connect(Observer *observer, ValueCallable callable)
    {
        this->observer_ = observer;
        this->Connect(callable);
    }

    void Connect(SignalCallable callable)
    {
        if (!this->observer_)
        {
            throw std::runtime_error("ListConnect has no observer.");
        }

        this->signalConnection_.emplace(this->observer_, callable);

        if (this->connectables_.empty())
        {
            this->MakeListConnections_();
        }
    }

    void Connect(Observer *observer, SignalCallable callable)
    {
        this->observer_ = observer;
        this->Connect(callable);
    }

    ~ListConnect()
    {
        this->ClearListConnections_();
    }

    const ListControl & GetControl() const
    {
        return this->listControl_;
    }

    ListControl & GetControl()
    {
        return this->listControl_;
    }

    explicit operator ListControl () const
    {
        return this->listControl_;
    }

    void Disconnect(Observer *)
    {
        this->Disconnect();
    }

    void Disconnect()
    {
        this->valueConnection_.reset();
        this->signalConnection_.reset();
        this->ClearListConnections_();
    }

    Plain Get() const
    {
        return this->listControl_.Get();
    }

private:
    void OnMute_(const Mute_ &muteState)
    {
        if (!muteState.isMuted && !muteState.isSilenced)
        {
            if (this->valueConnection_.has_value())
            {
                // Notify list observers when unmuted.
                (*this->valueConnection_)(this->cached_);
            }

            if (this->signalConnection_.has_value())
            {
                (*this->signalConnection_)();
            }
        }

        this->muteState_ = muteState;
    }

    static void OnItemChanged_(
        size_t index,
        void * context,
        ::pex::Argument<Item> item)
    {
        auto self = static_cast<ListConnect *>(context);

        self->cached_.at(index) = item;

        if (self->muteState_)
        {
            return;
        }

        if (self->isNotifying_)
        {
            // The observed list is notifying all members at once.
            // Wait until it is done to send one group notification.
            return;
        }

        if (self->valueConnection_.has_value())
        {
            (*self->valueConnection_)(self->cached_);
        }

        if (self->signalConnection_.has_value())
        {
            (*self->signalConnection_)();
        }
    }

    void MakeListConnections_()
    {
        size_t count = this->listControl_.count.Get();

        if (!count)
        {
            return;
        }

        for (size_t index = 0; index < count; ++index)
        {
            auto result = this->connectables_.try_emplace(
                index,
                this,
                this->listControl_[index],
                std::bind(
                    ListConnect::OnItemChanged_,
                    index,
                    std::placeholders::_1,
                    std::placeholders::_2));

            assert(result.second);
        }
    }

    void ClearListConnections_()
    {
        if (this->connectables_.empty())
        {
            return;
        }

        for (auto &pair: this->connectables_)
        {
            pair.second.Disconnect(this);
        }

        this->connectables_.clear();
    }

    void OnMemberWillRemove_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        if (this->connectables_.empty())
        {
            return;
        }

        this->connectables_.at(*index).Disconnect(this);
        this->connectables_.erase(*index);
    }

    void OnMemberAdded_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        this->cached_ = this->listControl_.Get();

        if (
            this->valueConnection_.has_value()
            || this->signalConnection_.has_value())
        {
            auto result = this->connectables_.try_emplace(
                *index,
                this,
                this->listControl_[*index],
                std::bind(
                    ListConnect::OnItemChanged_,
                    *index,
                    std::placeholders::_1,
                    std::placeholders::_2));

            assert(result.second);
        }
    }

    void OnIsNotifying_(bool isNotifying)
    {
        this->isNotifying_ = isNotifying;

        if (this->muteState_)
        {
            return;
        }

        if (!isNotifying)
        {
            // When isNotifying transitions to false, send a group
            // notification.
            if (this->valueConnection_.has_value())
            {
                (*this->valueConnection_)(this->cached_);
            }

            if (this->signalConnection_.has_value())
            {
                (*this->signalConnection_)();
            }
        }
    }

private:
    using MuteTerminus = pex::Terminus<ListConnect, MuteModel>;
    MuteTerminus muteTerminus_;
    Mute_ muteState_;
    ListControl listControl_;
    Connectables connectables_;
    Observer *observer_;
    std::optional<ValueConnection_> valueConnection_;
    std::optional<SignalConnection_> signalConnection_;
    MemberWillRemoveTerminus memberWillRemoveTerminus_;
    MemberAddedTerminus memberAddedTerminus_;
    bool isNotifying_;
    ListFlagTerminus isNotifyingTerminus_;
    ListType cached_;
};


} // end namespace detail


template<typename T>
concept IsListConnect = requires { T::isListConnect; };


} // end namespace pex
