#pragma once

#include "pex/list.h"
#include "pex/selectors.h"
#include "pex/promote_control.h"
#include "pex/detail/forward.h"


#ifdef ENABLE_PEX_NAMES
#define PEX_LINK_NOTNULL(address, observer)   \
    if (observer)                             \
    {                                         \
        PEX_LINK_OBSERVER(address, observer); \
    }
#else
#define PEX_LINK_NOTNULL(address, observer)
#endif


namespace pex
{


namespace detail
{


template
<
    typename T,
    typename Enable = void
>
struct ConnectableSelector_
{
    using Type = typename PromoteControl<T>::Type;
};

template<typename T>
struct ConnectableSelector_
<
    T,
    std::enable_if_t<IsGroupNode<T>>
>
{
    using Type = GroupConnect<void, T>;
};


template<typename T>
struct ConnectableSelector_
<
    T,
    std::enable_if_t<IsListNode<T>>
>
{
    using Type = ListConnect<void, T>;
};


template<typename T>
using ConnectableSelector = typename ConnectableSelector_<T>::Type;


template<typename Observer, typename Item, typename = void>
struct IndexedCallable_
{
    using type =
        void (Observer::*)(size_t index, Argument<Item> value);
};

template<typename Observer, typename Item>
struct IndexedCallable_
<
    Observer,
    Item,
    std::enable_if_t<std::is_void_v<Observer>>
>
{
    using type =
        std::function<void(Observer *, size_t index, Argument<Item> value)>;
};


template<typename Observer, typename Item>
using IndexedCallable = typename IndexedCallable_<Observer, Item>::type;


template
<
    typename Observer,
    typename Upstream_
>
class ListConnect: Separator
{
public:
    static_assert(::pex::IsListNode<Upstream_>);

    static constexpr auto observerName = "pex::ListConnect";
    static constexpr bool isListConnect = true;

    using ListControl = typename PromoteControl<Upstream_>::Type;

    using Connectable =
        ConnectableSelector<typename ListControl::ListItem>;

    using Connectables = std::vector<Connectable>;

    using UpstreamControl = ListControl;
    using Upstream = typename PromoteControl<Upstream_>::Upstream;

    using ListType = typename ListControl::Type;
    using Plain = ListType;
    using Type = Plain;

    using Item = typename ListControl::Item;

    using MemberWillRemoveTerminus =
        Terminus<ListConnect, typename ListControl::MemberWillRemove>;

    using MemberRemovedTerminus =
        Terminus<ListConnect, typename ListControl::MemberRemoved>;

    using MemberAddedTerminus =
        ::pex::Terminus<ListConnect, typename ListControl::MemberAdded>;

    using MemberWillReplaceTerminus =
        Terminus<ListConnect, typename ListControl::MemberWillReplace>;

    using MemberReplacedTerminus =
        Terminus<ListConnect, typename ListControl::MemberReplaced>;

    using ListFlagTerminus =
        ::pex::Terminus<ListConnect, typename ListControl::ListFlag>;

    using ValueConnection_ = ValueConnection<Observer, ListType>;
    using ValueCallable = typename ValueConnection_::Callable;
    using Callable = ValueCallable;

    using IndexedCallable = ::pex::detail::IndexedCallable<Observer, Item>;
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
        indexedCallable_(),
        memberWillRemoveTerminus_(),
        memberRemovedTerminus_(),
        memberAddedTerminus_(),
        memberWillReplaceTerminus_(),
        memberReplacedTerminus_(),
        isNotifying_(false),
        isNotifyingTerminus_(),
        cached_()
    {

    }

    ListConnect(const ListControl &listControl)
        :
        muteTerminus_(
            PEX_THIS("ListConnect"),
            listControl.CloneMuteNode(),
            &ListConnect::OnMute_),
        muteState_(listControl.CloneMuteNode().Get()),
        listControl_(listControl),
        connectables_(),
        observer_(nullptr),
        valueConnection_(),
        signalConnection_(),
        indexedCallable_(),

        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberRemovedTerminus_(
            this,
            this->listControl_.memberRemoved,
            &ListConnect::OnMemberRemoved_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        memberWillReplaceTerminus_(
            this,
            this->listControl_.memberWillReplace,
            &ListConnect::OnMemberWillReplace_),

        memberReplacedTerminus_(
            this,
            this->listControl_.memberReplaced,
            &ListConnect::OnMemberReplaced_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_()
    {

    }

    ListConnect(
        Observer *observer,
        const ListControl &listControl,
        ValueCallable callable)
        :
        muteTerminus_(
            PEX_THIS("ListConnect"),
            listControl.CloneMuteNode(),
            &ListConnect::OnMute_),
        muteState_(listControl.CloneMuteNode().Get()),
        listControl_(listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(std::in_place_t{}, observer, callable),
        signalConnection_(),
        indexedCallable_(),

        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberRemovedTerminus_(
            this,
            this->listControl_.memberRemoved,
            &ListConnect::OnMemberRemoved_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        memberWillReplaceTerminus_(
            this,
            this->listControl_.memberWillReplace,
            &ListConnect::OnMemberWillReplace_),

        memberReplacedTerminus_(
            this,
            this->listControl_.memberReplaced,
            &ListConnect::OnMemberReplaced_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_(this->listControl_.Get())
    {
        assert(this->observer_ != nullptr);
        PEX_LINK_OBSERVER(this, this->observer_);
        this->RestoreConnections_(0);
    }

    ListConnect(
        Observer *observer,
        const ListControl &listControl,
        SignalCallable callable)
        :
        muteTerminus_(
            PEX_THIS("ListConnect"),
            listControl.CloneMuteNode(),
            &ListConnect::OnMute_),
        muteState_(listControl.CloneMuteNode().Get()),
        listControl_(listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(),
        signalConnection_(std::in_place_t{}, observer, callable),
        indexedCallable_(),

        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberRemovedTerminus_(
            this,
            this->listControl_.memberRemoved,
            &ListConnect::OnMemberRemoved_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        memberWillReplaceTerminus_(
            this,
            this->listControl_.memberWillReplace,
            &ListConnect::OnMemberWillReplace_),

        memberReplacedTerminus_(
            this,
            this->listControl_.memberReplaced,
            &ListConnect::OnMemberReplaced_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_()
    {
        assert(this->observer_ != nullptr);
        PEX_LINK_OBSERVER(this, this->observer_);
        this->RestoreConnections_(0);
    }

    ListConnect(Upstream &upstream)
        :
        ListConnect(ListControl(upstream))
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

    ListConnect(const ListConnect &other)
        :
        muteTerminus_(
            PEX_THIS("ListConnect"),
            other.listControl_.CloneMuteNode(),
            &ListConnect::OnMute_),
        muteState_(other.muteState_),
        listControl_(other.listControl_),
        connectables_(),
        observer_(nullptr),
        valueConnection_(),
        signalConnection_(),
        indexedCallable_(),

        memberWillRemoveTerminus_(
            this,
            this->listControl_.memberWillRemove,
            &ListConnect::OnMemberWillRemove_),

        memberRemovedTerminus_(
            this,
            this->listControl_.memberRemoved,
            &ListConnect::OnMemberRemoved_),

        memberAddedTerminus_(
            this,
            this->listControl_.memberAdded,
            &ListConnect::OnMemberAdded_),

        memberWillReplaceTerminus_(
            this,
            this->listControl_.memberWillReplace,
            &ListConnect::OnMemberWillReplace_),

        memberReplacedTerminus_(
            this,
            this->listControl_.memberReplaced,
            &ListConnect::OnMemberReplaced_),

        isNotifying_(false),

        isNotifyingTerminus_(
            this,
            this->listControl_.isNotifying,
            &ListConnect::OnIsNotifying_),

        cached_(other.cached_)
    {
        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                other.observer_,
                other.valueConnection_->GetCallable());
        }

        if (other.signalConnection_)
        {
            this->signalConnection_.emplace(
                other.observer_,
                other.signalConnection_->GetCallable());
        }

        if (other.indexedCallable_)
        {
            assert(other.observer_);
            this->indexedCallable_ = other.indexedCallable_;
        }

        if (
            other.valueConnection_
            || other.signalConnection_
            || other.indexedCallable_)
        {
            assert(other.observer_ != nullptr);
            this->observer_ = other.observer_;
            PEX_LINK_OBSERVER(this, this->observer_);
            this->RestoreConnections_(0);
        }
    }

    ListConnect & operator=(const ListConnect &other)
    {
        return this->Assign(other.observer_, other);
    }

    ListConnect & Assign(Observer *observer, const ListConnect &other)
    {
        PEX_LINK_NOTNULL(this, observer);

        this->Disconnect();

        this->muteTerminus_.RequireAssign(this, other.muteTerminus_);
        this->muteState_ = other.muteState_;

        this->listControl_ = other.listControl_;

        if (other.valueConnection_)
        {
            assert(observer != nullptr);

            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());
        }

        if (other.signalConnection_)
        {
            assert(observer != nullptr);

            this->signalConnection_.emplace(
                observer,
                other.signalConnection_->GetCallable());
        }

        if (other.indexedCallable_)
        {
            assert(observer != nullptr);
            this->indexedCallable_ = other.indexedCallable_;
        }

        if (
            other.valueConnection_
            || other.signalConnection_
            || other.indexedCallable_)
        {
            this->observer_ = observer;
            this->RestoreConnections_(0);
        }

        this->memberWillRemoveTerminus_.RequireAssign(
            this,
            other.memberWillRemoveTerminus_);

        this->memberRemovedTerminus_.RequireAssign(
            this,
            other.memberRemovedTerminus_);

        this->memberAddedTerminus_.RequireAssign(
            this,
            other.memberAddedTerminus_);

        this->memberWillReplaceTerminus_.RequireAssign(
            this,
            other.memberWillReplaceTerminus_);

        this->memberReplacedTerminus_.RequireAssign(
            this,
            other.memberReplacedTerminus_);

        this->isNotifying_ = other.isNotifying_;

        this->isNotifyingTerminus_.RequireAssign(
            this,
            other.isNotifyingTerminus_);

        this->cached_ = other.cached_;

        return *this;
    }

    void Connect(Observer *observer, ValueCallable callable)
    {
        PEX_LINK_OBSERVER(this, observer);
        this->cached_ = this->listControl_.Get();
        this->observer_ = observer;
        this->valueConnection_.emplace(observer, callable);

        if (this->connectables_.empty())
        {
            this->RestoreConnections_(0);
        }
    }

    void Connect(Observer *observer, SignalCallable callable)
    {
        PEX_LINK_OBSERVER(this, observer);
        this->observer_ = observer;
        this->signalConnection_.emplace(this->observer_, callable);

        if (this->connectables_.empty())
        {
            this->RestoreConnections_(0);
        }
    }

    void Connect(Observer *observer, IndexedCallable callable)
    {
        PEX_LINK_OBSERVER(this, observer);
        this->observer_ = observer;
        this->indexedCallable_ = callable;

        if (this->connectables_.empty())
        {
            this->RestoreConnections_(0);
        }
    }

    bool HasObservers() const
    {
        bool result =
            this->valueConnection_.has_value()
            || this->signalConnection_.has_value()
            || this->indexedCallable_.has_value();

#ifndef NDEBUG
        if (!result)
        {
            assert(this->connectables_.empty());
        }
#endif

        return result;
    }

    bool NeedsCache_() const
    {
        return this->valueConnection_.has_value();
    }

    ~ListConnect()
    {
        this->Disconnect();
        PEX_CLEAR_NAME(this);
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
        if (!this->observer_)
        {
            assert(!this->valueConnection_.has_value());
            assert(!this->signalConnection_.has_value());
            assert(!this->indexedCallable_.has_value());
            assert(this->connectables_.empty());

            return;
        }

        this->valueConnection_.reset();
        this->signalConnection_.reset();
        this->indexedCallable_.reset();
        this->ClearListConnections_();
        this->observer_ = nullptr;
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

        if (self->NeedsCache_())
        {
            self->cached_.at(index) = item;
        }

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

        if (self->indexedCallable_.has_value())
        {
            assert(self->observer_ != NULL);

            if constexpr (std::is_void_v<Observer>)
            {
                (*self->indexedCallable_)(self->observer_, index, item);
            }
            else
            {
                (self->observer_->*(*self->indexedCallable_))(index, item);
            }
        }
    }

    void ClearListConnections_()
    {
        if (this->connectables_.empty())
        {
            return;
        }

        for (auto &connectable: this->connectables_)
        {
            connectable.Disconnect(this);
        }

        this->connectables_.clear();
    }

    void OnMemberWillRemove_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        if (!this->HasObservers())
        {
            return;
        }

        this->ClearInvalidatedConnections_(*index);
    }

    void OnMemberRemoved_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        if (this->NeedsCache_())
        {
            // Update the cache.
            jive::SafeErase(this->cached_, *index);
        }

        if (!this->HasObservers())
        {
            return;
        }

        this->RestoreConnections_(*index);
    }

    void ClearConnection_(size_t index)
    {
        this->connectables_.at(index).Disconnect(this);
    }

    void ClearInvalidatedConnections_(size_t firstToClear)
    {
        size_t connectionCount = this->connectables_.size();

        if (firstToClear >= connectionCount)
        {
            // Nothing to do.
            // An item was added at the end of the list.

            return;
        }

        for (size_t index = firstToClear; index < connectionCount; ++index)
        {
            this->connectables_.at(index).Disconnect(this);
        }

        this->connectables_.erase(
            jive::SafeEraseIterator(this->connectables_, firstToClear),
            std::end(this->connectables_));
    }

    void RestoreConnection_(size_t index)
    {
        this->connectables_.at(index) =
            Connectable(
                this,
                this->listControl_.at(index),
                std::bind(
                    ListConnect::OnItemChanged_,
                    index,
                    std::placeholders::_1,
                    std::placeholders::_2));
    }

    void RestoreConnectionAtEnd_(size_t index)
    {
        this->connectables_.emplace_back(
            this,
            this->listControl_[index],
            std::bind(
                ListConnect::OnItemChanged_,
                index,
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void RestoreConnections_(size_t firstIndex)
    {
        size_t listCount = this->listControl_.size();
        assert(this->connectables_.size() == firstIndex);
        static_assert(std::is_nothrow_move_constructible_v<Connectable>);

        for (size_t i = firstIndex; i < listCount; ++i)
        {
            this->RestoreConnectionAtEnd_(i);
        }
    }

    void OnMemberAdded_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        if (this->NeedsCache_())
        {
            // Maintain the cache.
            this->cached_.insert(
                jive::SafeInsertIterator(this->cached_, *index),
                this->listControl_.at(*index).Get());
        }

        if (this->HasObservers())
        {
            this->ClearInvalidatedConnections_(*index);
            this->RestoreConnections_(*index);
        }
    }

    void OnMemberWillReplace_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        if (this->HasObservers())
        {
            this->ClearConnection_(*index);
        }
        else
        {
            assert(this->connectables_.empty());
        }
    }

    void OnMemberReplaced_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            return;
        }

        if (this->HasObservers())
        {
            this->RestoreConnection_(*index);
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
    using MuteNode = decltype(std::declval<ListControl>().CloneMuteNode());
    using MuteTerminus = pex::Terminus<ListConnect, MuteNode>;

    MuteTerminus muteTerminus_;
    Mute_ muteState_;
    ListControl listControl_;
    Connectables connectables_;
    Observer *observer_;
    std::optional<ValueConnection_> valueConnection_;
    std::optional<SignalConnection_> signalConnection_;
    std::optional<IndexedCallable> indexedCallable_;
    MemberWillRemoveTerminus memberWillRemoveTerminus_;
    MemberRemovedTerminus memberRemovedTerminus_;
    MemberAddedTerminus memberAddedTerminus_;
    MemberWillReplaceTerminus memberWillReplaceTerminus_;
    MemberReplacedTerminus memberReplacedTerminus_;
    bool isNotifying_;
    ListFlagTerminus isNotifyingTerminus_;
    ListType cached_;
};


} // end namespace detail


template<typename T>
concept IsListConnect = requires { T::isListConnect; };


} // end namespace pex
