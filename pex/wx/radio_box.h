/**
  * @file radio_box.h
  *
  * @brief A wx.RadioBox connected to a pex.Value interface node.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include <vector>
#include <algorithm>

#include "wxshim.h"
#include "pex/value.h"
#include "pex/wx/pex_window.h"
#include "pex/converter.h"
#include "pex/find_index.h"
#include "pex/wx/array_string.h"

namespace pex
{

namespace wx
{



template<typename Value, typename Convert = Converter<typename Value::Type>>
class RadioBox: public PexWindow<wxRadioBox>
{
public:
    using Base = PexWindow<wxRadioBox>;
    using Type = typename Value::Type;

    template<typename CompatibleValue>
    RadioBox(
        wxWindow *parent,
        CompatibleValue value,
        const std::vector<Type> &choices,
        const std::string &label = "",
        long style = wxRA_SPECIFY_ROWS)
        :
        Base(
            parent,
            wxID_ANY,
            label,
            wxDefaultPosition,
            wxDefaultSize,
            MakeArrayString<Convert>(choices),
            0,
            style),
        value_(value),
        choices_(choices)
    {
        assert(this->ValueInChoices_(value));
        this->SetSelection(this->GetIndex_(value.Get()));
        this->value_.Connect(this, &RadioBox::OnValueChanged_);
        this->Bind(wxEVT_RADIOBOX, &RadioBox::OnRadioBox_, this);
    }

    void OnValueChanged_(typename detail::Argument<Type>::Type value)
    {
        this->SetSelection(this->GetIndex_(value));
    }

    void OnRadioBox_(wxCommandEvent &event)
    {
        auto index = event.GetSelection();

        if (index < 0)
        {
            return;
        }

        this->value_.Set(this->choices_.at(static_cast<size_t>(index)));
    }

private:
    int GetIndex_(typename detail::Argument<Type>::Type value)
    {
        auto index = FindIndex(value, this->choices_);

        if (-1 == index)
        {
            throw std::out_of_range("Value not found");
        }

        if (index > std::numeric_limits<int>::max())
        {
            throw std::out_of_range("Index is out of range");
        }

        return static_cast<int>(index);
    }

    bool ValueInChoices_(const Value &value)
    {
        auto index = FindIndex(value, this->choices_);
        return (index != -1);
    }

    using Observer = RadioBox<Value, Convert>;
    typename pex::interface::ObservedValue<Observer, Value>::Type value_;

    std::vector<Type> choices_;
};


} // namespace wx

} // namespace pex
