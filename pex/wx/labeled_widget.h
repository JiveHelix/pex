/**
  * @file labeled_widget.h
  *
  * @brief Attach a label to any pex widget.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 09 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "pex/wx/wxshim.h"
#include "pex/value.h"
#include "pex/is_compatible.h"

namespace pex
{

namespace wx
{

/**
 ** Combines a label and a pex widget.
 **/
template<typename MakeWidget>
class LabeledWidget
{
public:
    using Widget = typename MakeWidget::Type;

    LabeledWidget(
        wxWindow *parent,
        const MakeWidget &makeWidget,
        const std::string &label)
    {
        this->label_ = new wxStaticText(parent, wxID_ANY, label);
        this->widget_ = makeWidget(parent);
    }

    wxStaticText * GetLabel()
    {
        return this->label_;
    }

    Widget * GetWidget()
    {
        return this->widget_;
    }

    std::unique_ptr<wxSizer> Layout(int orient = wxHORIZONTAL) const
    {
        auto sizer = std::make_unique<wxBoxSizer>(orient);

        auto flag = (orient == wxHORIZONTAL)
            ? wxRIGHT
            : wxBOTTOM | wxEXPAND;

        sizer->Add(this->label_, 0, flag, 5);
        sizer->Add(this->widget_, 1, flag);

        return sizer;
    }

private:
    wxStaticText *label_;
    Widget *widget_;
};


/** Stores the control and style arguments to defer creation of the widget. **/
template<template<typename...> typename Widget, typename Control>
struct MakeWidget
{
    using Type = Widget<Control>;

    Control control_;
    long style_ = 0;

    template<typename Compatible>
    MakeWidget(Compatible control, long style = 0)
        :
        control_(control),
        style_(style)
    {
        static_assert(pex::IsCompatibleV<Control, Compatible>);
    }

    Type * operator()(wxWindow *parent) const
    {
        return new Type(
            parent,
            this->control_,
            this->style_);
    }
};


struct LayoutOptions
{
    int orient = wxVERTICAL;
    int labelAlign = wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL;
    int widgetAlign = wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL;
    int verticalGap = 3;
    int horizontalGap = 3;
};


template<typename ...Labeled>
auto GetLabels(Labeled &&...labeled)
{
    return std::make_tuple(labeled.GetLabel()...);
}


template<typename ...Labeled>
auto GetWidgets(Labeled &&...labeled)
{
    return std::make_tuple(labeled.GetWidget()...);
}


template<typename Label, typename Widget>
void AddLabelAndWidget(
    wxSizer *sizer,
    const LayoutOptions &options,
    Label &&label,
    Widget &&widget)
{
    sizer->Add(std::forward<Label>(label), 0, options.labelAlign);
    sizer->Add(std::forward<Widget>(widget), 0, options.widgetAlign);
}



template<typename Labels, typename Widgets, size_t ...I>
void AddVertical(
    wxSizer *sizer,
    const LayoutOptions &options,
    Labels &&labels,
    Widgets &&widgets,
    std::index_sequence<I...>)
{
    (
        AddLabelAndWidget(
            sizer,
            options,
            std::get<I>(std::forward<Labels>(labels)),
            std::get<I>(std::forward<Widgets>(widgets))),
        ...
    );
}


template<typename Items, size_t ...I>
void AddRow(
    wxSizer *sizer,
    int flags,
    Items &&items,
    std::index_sequence<I...>)
{
    (sizer->Add(std::get<I>(std::forward<Items>(items)), 0, flags), ...);
}



template<typename ...Labeled>
std::unique_ptr<wxSizer> LayoutLabeled(
    LayoutOptions options,
    Labeled &&...labeled)
{
    auto groupSizer = std::make_unique<wxFlexGridSizer>(
        (options.orient == wxVERTICAL) ? 2 : sizeof...(Labeled),
        options.verticalGap,
        options.horizontalGap);

    auto labels = GetLabels(std::forward<Labeled>(labeled)...);
    auto widgets = GetWidgets(std::forward<Labeled>(labeled)...);

    if (options.orient == wxHORIZONTAL)
    {
        // Layout in a row with labels above their respective widgets.
        AddRow(
            groupSizer.get(),
            options.labelAlign,
            labels,
            std::make_index_sequence<sizeof...(Labeled)>{});

        AddRow(
            groupSizer.get(),
            options.widgetAlign,
            widgets,
            std::make_index_sequence<sizeof...(Labeled)>{});
    }
    else
    {
        // Layout in a stack with labels on the left.
        assert(options.orient == wxVERTICAL);

        AddVertical(
            groupSizer.get(),
            options,
            labels,
            widgets,
            std::make_index_sequence<sizeof...(Labeled)>{});
    }

    return groupSizer;
}


} // wx

} // pex
