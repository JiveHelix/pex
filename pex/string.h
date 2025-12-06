#pragma once

#include <pex/control_value.h>
#include <pex/endpoint.h>
#include <string>



namespace pex
{


using StringModel = pex::model::Value<std::string>;
using StringControl = pex::control::Value_<StringModel>;

template<typename Observer>
using StringEndpoint = pex::Endpoint<Observer, StringControl>;


} // end namespace pex
