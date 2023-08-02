#pragma once


#include "pex/model_value.h"
#include "pex/control_value.h"
#include "pex/terminus.h"


namespace pex
{

namespace detail
{


using MuteModel = typename ::pex::model::Value<bool>;
using MuteControl = typename ::pex::control::Value<MuteModel>;

template<typename Observer>
using MuteTerminus = ::pex::Terminus<Observer, MuteModel>;


class MuteOwner
{
public:
    MuteOwner()
        :
        mute_(false)
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

private:
    MuteModel mute_;
};


class MuteGroup
{
public:
    MuteGroup()
        :
        muteControl_()
    {

    }

    MuteGroup(MuteControl muteControl)
        :
        muteControl_(muteControl)
    {

    }

    MuteGroup(const MuteGroup &other)
        :
        muteControl_(other.muteControl_)
    {

    }

    MuteGroup & operator=(const MuteGroup &other)
    {
        this->muteControl_ = other.muteControl_;
        return *this;
    }

    MuteControl CloneMuteControl()
    {
        return this->muteControl_;
    }

    bool IsMuted() const
    {
        return this->muteControl_.Get();
    }

    void DoMute()
    {
        this->muteControl_.Set(true);
    }

    void DoUnmute()
    {
        this->muteControl_.Set(false);
    }

private:
    MuteControl muteControl_;
};


} // end namespace detail


} // end namespace pex
