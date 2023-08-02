#include "pex/control_value.h"


namespace pex
{


namespace control
{


template class Value_<model::Value_<bool, NoFilter>>;

template class Value_<model::Value_<int8_t, NoFilter>>;
template class Value_<model::Value_<int16_t, NoFilter>>;
template class Value_<model::Value_<int32_t, NoFilter>>;
template class Value_<model::Value_<int64_t, NoFilter>>;

template class Value_<model::Value_<uint8_t, NoFilter>>;
template class Value_<model::Value_<uint16_t, NoFilter>>;
template class Value_<model::Value_<uint32_t, NoFilter>>;
template class Value_<model::Value_<uint64_t, NoFilter>>;

template class Value_<model::Value_<float, NoFilter>>;
template class Value_<model::Value_<double, NoFilter>>;

template class Value_<model::Value_<std::string, NoFilter>>;


} // end namespace control


} // end namespace pex
