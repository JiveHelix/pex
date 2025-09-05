#pragma once


#include <fields/fields.h>
#include "pex/model_value.h"
#include "pex/control_value.h"


namespace pex
{

namespace detail
{


// isMuted, when true, will not notify group observers when individual values
// change.
// When isMuted is changed to false, each group observer will receive one
// notification for the whole group unless 'isSilenced' has been set.
// Use 'isSilenced' carefully as it is the responsibility of the user to notify
// observers.


template<typename T>
struct MuteFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::isMuted, "isMuted"),
        fields::Field(&T::isSilenced, "isSilenced"));
};


struct Mute_
{
    bool isMuted;
    bool isSilenced;

    Mute_()
        :
        isMuted(false),
        isSilenced(false)
    {

    }

    Mute_(bool isMuted_, bool isSilenced_)
        :
        isMuted(isMuted_),
        isSilenced(isSilenced_)
    {

    }

    operator bool () const
    {
        return this->isMuted;
    }

    static constexpr auto fields = std::make_tuple(
        fields::Field(&Mute_::isMuted, "isMuted"),
        fields::Field(&Mute_::isSilenced, "isSilenced"));
};


using MuteModel = typename ::pex::model::Value<Mute_>;
using MuteControlType = typename ::pex::control::Value<MuteModel>;
using MuteMuxType = typename ::pex::control::Mux<MuteModel>;
using MuteFollowType = typename ::pex::control::Value<MuteMuxType>;


class MuteOwner
{
public:
    MuteOwner()
        :
        mute_()
    {

    }

    MuteOwner(const MuteOwner &) = delete;
    MuteOwner(MuteOwner &&) = delete;
    MuteOwner & operator=(const MuteOwner &) = delete;
    MuteOwner & operator=(MuteOwner &&) = delete;

#if 0
    template<typename MuteNode>
    MuteNode GetMuteNode()
    {
        return MuteNode(this->mute_);
    }
#endif

    MuteModel & GetMuteNode()
    {
        return this->mute_;
    }

    void DoMute(bool isSilenced)
    {
        this->mute_.Set({true, isSilenced});
    }

    void DoUnmute()
    {
        Mute_ muteState = this->mute_.Get();

        // Leave isSilenced unchanged.
        muteState.isMuted = false;

        this->mute_.Set(muteState);
    }

private:
    MuteModel mute_;
};


template<typename Upstream>
class Mute: Separator
{
public:
    using MuteNode = typename ::pex::control::Value<Upstream>;

    Mute()
        :
        muteNode_()
    {
        PEX_NAME("Mute");
        PEX_MEMBER(muteNode_);
    }

    Mute(Upstream &upstream)
        :
        muteNode_(upstream)
    {
        PEX_NAME("Mute");
        PEX_MEMBER(muteNode_);
    }

    Mute(const MuteNode &muteNode)
        :
        muteNode_(muteNode)
    {
        PEX_NAME("Mute");
        PEX_MEMBER(muteNode_);
    }

    Mute(const Mute &other)
        :
        muteNode_(other.muteNode_)
    {
        PEX_NAME("Mute");
        PEX_MEMBER(muteNode_);
    }

    ~Mute()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->muteNode_);
    }

    Mute & operator=(const Mute &other)
    {
        this->muteNode_ = other.muteNode_;
        return *this;
    }

    MuteNode CloneMuteNode() const
    {
        return this->muteNode_;
    }

    MuteNode & GetMuteNodeReference()
    {
        return this->muteNode_;
    }

    bool IsMuted() const
    {
        return this->muteNode_.Get();
    }

    bool IsSilenced() const
    {
        return this->muteNode_.Get().isSilenced;
    }

    void DoMute(bool isSilenced)
    {
        this->muteNode_.Set({true, isSilenced});
    }

    void DoUnmute()
    {
        Mute_ muteState = this->muteNode_.Get();

        // Leave isSilenced unchanged.
        muteState.isMuted = false;

        this->muteNode_.Set(muteState);
    }

private:
    MuteNode muteNode_;
};


using MuteControl = Mute<MuteModel>;
using MuteFollow = Mute<MuteMuxType>;


class MuteMux: Separator
{
public:
    using MuteNode = MuteMuxType;

    MuteMux()
        :
        muteNode_()
    {
        PEX_NAME("MuteMux");
        PEX_MEMBER(muteNode_);
    }

    MuteMux(MuteModel &muteModel)
        :
        muteNode_(muteModel)
    {
        PEX_NAME("MuteMux");
        PEX_MEMBER(muteNode_);
    }

    ~MuteMux()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->muteNode_);
    }

    void ChangeUpstream(MuteModel &upstream)
    {
        this->muteNode_.ChangeUpstream(upstream);
    }

    MuteFollowType CloneMuteNode()
    {
        return MuteFollowType(this->muteNode_);
    }

    MuteNode & GetMuteNode()
    {
        return this->muteNode_;
    }

    bool IsMuted() const
    {
        return this->muteNode_.Get();
    }

    bool IsSilenced() const
    {
        return this->muteNode_.Get().isSilenced;
    }

    void DoMute(bool isSilenced)
    {
        this->muteNode_.Set({true, isSilenced});
    }

    void DoUnmute()
    {
        Mute_ muteState = this->muteNode_.Get();

        // Leave isSilenced unchanged.
        muteState.isMuted = false;

        this->muteNode_.Set(muteState);
    }

private:
    MuteNode muteNode_;
};


template<typename MuteNode>
class ScopeMute
{
public:
    ScopeMute()
        :
        muteNode_(nullptr),
        isMuted_(false)
    {

    }

    ScopeMute(MuteNode &upstream, bool isSilenced)
        :
        muteNode_(&upstream),
        isMuted_(false)
    {
        this->Mute(isSilenced);
    }

    ScopeMute(ScopeMute &&other)
        :
        muteNode_(other.muteNode_),
        isMuted_(other.isMuted_)
    {
        other.muteNode_ = nullptr;
        other.isMuted_ = false;
    }

    ScopeMute & operator=(ScopeMute &&other)
    {
        if (this->muteNode_ && this->isMuted_)
        {
            throw std::logic_error("Assign to armed ScopeMute");
        }

        this->muteNode_ = other.muteNode_;
        this->isMuted_ = other.isMuted_;
        other.muteNode_ = nullptr;
        other.isMuted_ = false;

        return *this;
    }

    bool IsMuted() const
    {
        return this->isMuted_;
    }

    ~ScopeMute()
    {
        this->Unmute();
    }

    void Mute(bool isSilenced)
    {
        if (!this->muteNode_)
        {
            throw std::logic_error("ScopeMute is uninitialized");
        }

        this->muteNode_->GetMuteNodeReference().Set({true, isSilenced});
        this->isMuted_ = true;
    }

    void Unmute()
    {
        if (this->isMuted_)
        {
            assert(this->muteNode_);

            auto &muteNode =
                this->muteNode_->GetMuteNodeReference();

            Mute_ muteState = muteNode.Get();

            // Leave isSilenced unchanged.
            muteState.isMuted = false;

            muteNode.Set(muteState);
            this->isMuted_ = false;
        }
    }

    void Clear()
    {
        if (!this->muteNode_ || !this->isMuted_)
        {
            return;
        }

        auto &muteNode =
            this->muteNode_->GetMuteNodeReference();

        Mute_ muteState;

        // Clear the mute without notifying by setting isSilenced to
        // true.
        muteState.isMuted = false;
        muteState.isSilenced = true;

        muteNode.Set(muteState);
        this->isMuted_ = false;
    }

private:
    MuteNode *muteNode_;
    bool isMuted_;
};


} // end namespace detail


} // end namespace pex
