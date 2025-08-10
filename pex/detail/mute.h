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
using MuteControl = typename ::pex::control::Value<MuteModel>;


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

    MuteControl GetMuteControl()
    {
        return MuteControl(this->mute_);
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


class Mute: Separator
{
public:
    Mute()
        :
        muteControl_()
    {
        PEX_NAME("Mute");
        PEX_MEMBER(muteControl_);
    }

    Mute(MuteControl muteControl)
        :
        muteControl_(muteControl)
    {
        PEX_NAME("Mute");
        PEX_MEMBER(muteControl_);
    }

    Mute(const Mute &other)
        :
        muteControl_(other.muteControl_)
    {
        PEX_NAME("Mute");
        PEX_MEMBER(muteControl_);
    }

    ~Mute()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->muteControl_);
    }

    Mute & operator=(const Mute &other)
    {
        this->muteControl_ = other.muteControl_;
        return *this;
    }

    MuteControl CloneMuteControl() const
    {
        return this->muteControl_;
    }

    MuteControl & GetMuteControlReference()
    {
        return this->muteControl_;
    }

    bool IsMuted() const
    {
        return this->muteControl_.Get();
    }

    bool IsSilenced() const
    {
        return this->muteControl_.Get().isSilenced;
    }

    void DoMute(bool isSilenced)
    {
        this->muteControl_.Set({true, isSilenced});
    }

    void DoUnmute()
    {
        Mute_ muteState = this->muteControl_.Get();

        // Leave isSilenced unchanged.
        muteState.isMuted = false;

        this->muteControl_.Set(muteState);
    }

private:
    MuteControl muteControl_;
};


template<typename Upstream>
class ScopeMute
{
public:
    ScopeMute()
        :
        upstream_(nullptr),
        isMuted_(false)
    {

    }

    ScopeMute(Upstream &upstream, bool isSilenced)
        :
        upstream_(&upstream),
        isMuted_(false)
    {
        this->Mute(isSilenced);
    }

    ScopeMute(ScopeMute &&other)
        :
        upstream_(other.upstream_),
        isMuted_(other.isMuted_)
    {
        other.upstream_ = nullptr;
        other.isMuted_ = false;
    }

    ScopeMute & operator=(ScopeMute &&other)
    {
        if (this->upstream_ && this->isMuted_)
        {
            throw std::logic_error("Assign to armed ScopeMute");
        }

        this->upstream_ = other.upstream_;
        this->isMuted_ = other.isMuted_;
        other.upstream_ = nullptr;
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
        if (!this->upstream_)
        {
            throw std::logic_error("ScopeMute is uninitialized");
        }

        this->upstream_->GetMuteControlReference().Set({true, isSilenced});
        this->isMuted_ = true;
    }

    void Unmute()
    {
        if (this->isMuted_)
        {
            assert(this->upstream_);

            auto &muteControl =
                this->upstream_->GetMuteControlReference();

            Mute_ muteState = muteControl.Get();

            // Leave isSilenced unchanged.
            muteState.isMuted = false;

            muteControl.Set(muteState);
            this->isMuted_ = false;
        }
    }

    void Clear()
    {
        if (!this->upstream_ || !this->isMuted_)
        {
            return;
        }

        auto &muteControl =
            this->upstream_->GetMuteControlReference();

        Mute_ muteState;

        // Clear the mute without notifying by setting isSilenced to
        // true.
        muteState.isMuted = false;
        muteState.isSilenced = true;

        muteControl.Set(muteState);
        this->isMuted_ = false;
    }

private:
    Upstream *upstream_;
    bool isMuted_;
};


} // end namespace detail


} // end namespace pex
