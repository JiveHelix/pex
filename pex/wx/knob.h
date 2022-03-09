#pragma once

#include "pex/wx/wxshim.h"

#include "pex/range.h"
#include "pex/converter.h"


namespace pex
{

namespace wx
{


template<typename RangeControl>
class Knob: public wxPanel
{
public:
    using Base = wxPanel;
    using This = Knob<RangeControl>;

    // Value and Bound are observed by This
    using Value = typename
        pex::control::ChangeObserver
        <
            This,
            typename RangeControl::Value
        >::Type;

    using Bound = typename
        pex::control::ChangeObserver
        <
            This,
            typename RangeControl::Bound
        >::Type;

    Knob(wxWindow *parent)
        :
        wxPanel(parent, wxID_ANY)
    {
        this->Bind(wxEVT_PAINT, &Knob::OnPaint_);
        this->Bind(WXEVT_SIZE, &Knob::OnSize_);
    }

private:
    void OnValueChanged_(Type value)
    {

    }

    void OnSize_(wxSizeEvent &event)
    {

    }

    void OnPaint_(wxPaintEvent &)
    {
        wxPaintDC dc(this);
        
    }
};


} // namespace wx

} // namespace pex
