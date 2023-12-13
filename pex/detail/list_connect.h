#pragma once

#include "pex/list.h"
#include "pex/selectors.h"
#include "pex/make_control.h"
#include "pex/detail/forward.h"


namespace pex
{

//
// Specializations of MakeControl for List.
template<typename P>
struct MakeControl<P, std::enable_if_t<IsListModel<P>>>
{
    using Control = ::pex::control::List<P, typename P::Model::ControlType>;
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

// template<typename T>
// struct ConnectableSelector_<T, std::enable_if_t<IsGroupModel<T>>>
// {
//     using Type = GroupConnect<void, typename T::ControlType>
// };

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
    static_assert(::pex::IsList<Upstream_>);

    static constexpr auto observerName = "pex::detail::ListConnect";

    using ListControl = typename MakeControl<Upstream_>::Control;
    using Connectable = ConnectableSelector<typename ListControl::ItemControl>;
    using Vector = std::vector<Connectable>;

    using UpstreamControl = ListControl;
    using Upstream = typename MakeControl<Upstream_>::Upstream;

    using ListType = typename ListControl::Type;
    using Plain = ListType;
    using Type = Plain;

    using Item = typename ListControl::Item;

    using CountWillChangeTerminus =
        Terminus<ListConnect, typename ListControl::CountWillChange>;

    using Count = typename ListControl::Count;
    using CountTerminus = ::pex::Terminus<ListConnect, Count>;

    using ValueConnection = detail::ValueConnection<Observer, ListType>;
    using Callable = typename ValueConnection::Callable;

    static_assert(std::is_same_v<Item, typename Connectable::Plain>);

    ListConnect()
        :
        muteTerminus_(),
        isMuted_(false),
        hasListConnections_(false),
        listControl_(),
        connectables_(),
        observer_(nullptr),
        valueConnection_(),
        countWillChange_(),
        count_(),
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
        isMuted_(listControl.CloneMuteControl().Get()),
        hasListConnections_(false),
        listControl_(listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(),
        countWillChange_(
            this,
            this->listControl_.countWillChange,
            &ListConnect::OnCountWillChange_),
        count_(
            this,
            this->listControl_.count,
            &ListConnect::OnCount_),
        cached_(this->listControl_.Get())
    {

    }

    ListConnect(
        Observer *observer,
        const ListControl &listControl,
        Callable callable)
        :
        muteTerminus_(
            this,
            listControl.CloneMuteControl(),
            &ListConnect::OnMute_),
        isMuted_(listControl.CloneMuteControl().Get()),
        hasListConnections_(false),
        listControl_(listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(std::in_place_t{}, observer, callable),
        countWillChange_(
            this,
            this->listControl_.countWillChange,
            &ListConnect::OnCountWillChange_),
        count_(
            this,
            this->listControl_.count,
            &ListConnect::OnCount_),
        cached_(this->listControl_.Get())
    {
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
        Callable callable)
        :
        ListConnect(observer, ListControl(upstream), callable)
    {

    }

    ListConnect(Observer *observer, const ListConnect &other)
        :
        muteTerminus_(
            this,
            other.CloneMuteControl(),
            &ListConnect::OnMute_),
        isMuted_(other.isMuted_),
        hasListConnections_(false),
        listControl_(other.listControl),
        connectables_(),
        observer_(observer),
        valueConnection_(),
        countWillChange_(
            this,
            this->listControl_.countWillChange,
            &ListConnect::OnCountWillChange_),
        count_(
            this,
            this->listControl_.count,
            &ListConnect::OnCount_),
        cached_(this->listControl_.Get())
    {
        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());

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

    }

    ListConnect & Assign(Observer *observer, const ListConnect &other)
    {
        this->muteTerminus_.Assign(this, other.muteTerminus_);
        this->isMuted_ = other.isMuted_;
        this->ClearListConnections_();
        this->cached_ = other.cached_;

        this->listControl_ = other.listControl_;
        this->observer_ = observer;

        if (other.valueConnection_)
        {
            this->valueConnection_.emplace(
                observer,
                other.valueConnection_->GetCallable());

            this->MakeListConnections_();
        }

        return *this;
    }

    void Connect(Callable callable)
    {
        if (!this->observer_)
        {
            throw std::runtime_error("ListConnect has no observer.");
        }

        this->valueConnection_.emplace(this->observer_, callable);

        if (!this->hasListConnections_)
        {
            this->MakeListConnections_();
        }
    }

    void Connect(Observer *observer, Callable callable)
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
        this->valueConnection_.reset();
        this->ClearListConnections_();
    }

    Plain Get() const
    {
        return this->listControl_.Get();
    }

private:
    void OnMute_(bool isMuted)
    {
        if (!isMuted && this->valueConnection_.has_value())
        {
            // Notify group observers when unmuted.
            (*this->valueConnection_)(this->cached_);
        }

        this->isMuted_ = isMuted;
    }

    static void OnItemChanged_(
        size_t index,
        void * context,
        ::pex::Argument<Item> item)
    {
        auto self = static_cast<ListConnect *>(context);
        self->cached_.at(index) = item;

        if (self->isMuted_)
        {
            return;
        }

        assert(self->valueConnection_.has_value());
        (*self->valueConnection_)(self->cached_);
    }

    void MakeListConnections_()
    {
        size_t count = this->listControl_.count.Get();

        if (!count)
        {
            return;
        }

        this->hasListConnections_ = true;
        this->connectables_.reserve(count);

        for (size_t index = 0; index < count; ++index)
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
    }

    void ClearListConnections_()
    {
        if (!this->hasListConnections_)
        {
            return;
        }

        for (auto &control: this->connectables_)
        {
            control.Disconnect(this);
        }

        this->connectables_.clear();
        this->hasListConnections_ = false;
    }

    void OnCountWillChange_()
    {
        this->ClearListConnections_();
    }

    void OnCount_(size_t)
    {
        this->cached_ = this->listControl_.Get();

        if (this->valueConnection_.has_value())
        {
            this->MakeListConnections_();
        }

        if (!this->isMuted_ && this->valueConnection_.has_value())
        {
            (*this->valueConnection_)(this->cached_);
        }
    }


private:
    MuteTerminus<ListConnect> muteTerminus_;
    bool isMuted_;
    bool hasListConnections_;
    ListControl listControl_;
    Vector connectables_;
    Observer *observer_;
    std::optional<ValueConnection> valueConnection_;
    CountWillChangeTerminus countWillChange_;
    CountTerminus count_;
    ListType cached_;
};


} // end namespace detail


} // end namespace pex
