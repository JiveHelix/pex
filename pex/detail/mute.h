#pragma once


#include <fields/fields.h>
#include "pex/model_value.h"
#include "pex/control_value.h"


namespace pex
{

namespace detail
{


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

    static Mute_ Default()
    {
        return {false, false};
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
        mute_(Mute_::Default())
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


class Mute
{
public:
    Mute()
        :
        muteControl_()
    {

    }

    Mute(MuteControl muteControl)
        :
        muteControl_(muteControl)
    {

    }

    Mute(const Mute &other)
        :
        muteControl_(other.muteControl_)
    {

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

private:
    Upstream *upstream_;
    bool isMuted_;
};


} // end namespace detail


} // end namespace pex
