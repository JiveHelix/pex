/**
  * @file pex_window.h
  *
  * @brief Registers Values and Signals so that they can be disconnected when
  * the window is destroyed.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 06 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <string>
#include <vector>
#include "wxshim.h"
#include <iostream>


namespace pex
{

namespace wx
{

class TubeInterface
{
public:
    virtual ~TubeInterface() {}
    virtual void Disconnect() = 0;
};


template<typename NodeType>
class Tube: public TubeInterface
{
public:
    Tube(NodeType * node)
        :
        node_{node}
    {

    }

    ~Tube()
    {
        this->Disconnect();
    }

    void Disconnect() override
    {
        if (this->node_)
        {
            this->node_->Disconnect();
            this->node_ = nullptr;
        }
    }

    Tube(const Tube &other) = delete;
    Tube(Tube &&other) = delete;
    Tube & operator=(const Tube &other) = delete;
    Tube & operator=(Tube &&other) = delete;

private:
    NodeType * node_;
};


struct WindowProperties
{
    std::string label = "";
    wxPoint position = wxDefaultPosition;
    wxSize size = wxDefaultSize;
    long style;
    std::string name = "";
};


/**
 ** @param WxBase The wx class to use as base class, which itself must derive
 ** from wxWindow.
 **/
template<typename WxBase>
class PexWindow: public WxBase
{
public:
    using WxBase::WxBase;

protected:
    /** Recursively register all pex Signals and Values that must be
     ** disconnected in the Destroy method.
     **/
    template<typename First, typename... Others>
    void RegisterTubes(First &&first, Others &&...others)
    {
        this->tubes_.push_back(
            std::make_unique<Tube<std::remove_reference_t<First>>>(&first));

        if constexpr (sizeof...(Others) > 0)
        {
            this->template RegisterTubes(std::forward<Others>(others)...);
        }
    }

private:
    std::vector<std::unique_ptr<TubeInterface>> tubes_;
};


} // namespace wx

} // namespace pex


