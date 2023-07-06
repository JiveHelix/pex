#include "pex/model_value.h"


namespace pex
{


namespace model
{


template class Value_<bool, NoFilter>;

template class Value_<int8_t, NoFilter>;
template class Value_<int16_t, NoFilter>;
template class Value_<int32_t, NoFilter>;
template class Value_<int64_t, NoFilter>;

template class Value_<uint8_t, NoFilter>;
template class Value_<uint16_t, NoFilter>;
template class Value_<uint32_t, NoFilter>;
template class Value_<uint64_t, NoFilter>;

template class Value_<float, NoFilter>;
template class Value_<double, NoFilter>;

template class Value_<std::string, NoFilter>;


} // end namespace model


} // end namespace pex
