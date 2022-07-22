#pragma once

#include "pex/wx/wxshim.h"
#include <string>


namespace pex
{


namespace wx
{


class Window
{
public:
    Window(): window_(nullptr) {}

    Window(wxWindow *window)
        :
        window_(window)
    {
        this->BindCloseHandler_();
    }

    Window(const Window &other) = delete;

    Window & operator=(const Window &other) = delete;

    Window(Window &&other)
        :
        window_(other.window_)
    {
        other.UnbindCloseHandler_();
        this->BindCloseHandler_();
        other.window_ = nullptr;
    }

    Window & operator=(Window &&other)
    {
        this->UnbindCloseHandler_();
        other.UnbindCloseHandler_();

        if (this->window_)
        {
            this->window_->Close();
        }

        this->window_ = other.window_;
        this->BindCloseHandler_();

        other.window_ = nullptr;

        return *this;
    }

    ~Window()
    {
        this->Close();
    }

    void Close()
    {
        if (this->window_)
        {
            if (!this->window_->IsBeingDeleted())
            {
                this->UnbindCloseHandler_();
                this->window_->Close(true);
            }

            this->window_ = nullptr;
        }
    }

    wxWindow * Get()
    {
        return this->window_;
    }

    void Clear()
    {
        if (this->window_)
        {
            if (!this->window_->IsBeingDeleted())
            {
                this->UnbindCloseHandler_();
            }

            this->window_ = nullptr;
        }
    }

    operator bool () const
    {
        return (this->window_ != nullptr);
    }

private:
    void OnClose_(wxCloseEvent &event)
    {
        this->window_ = nullptr;
        event.Skip();
    }

    void BindCloseHandler_()
    {
        assert(this->window_ != nullptr);
        
        if (this->window_)
        {
            this->window_->Bind(wxEVT_CLOSE_WINDOW, &Window::OnClose_, this);
        }
    }

    void UnbindCloseHandler_()
    {
        if (this->window_)
        {
            this->window_->Unbind(wxEVT_CLOSE_WINDOW, &Window::OnClose_, this);
        }
    }

private:
    wxWindow *window_;
};


} // end namespace wx


} // end namespace pex
