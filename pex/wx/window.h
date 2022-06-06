#pragma once

#include "pex/wx/wxshim.h"


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

    Window(const Window &) = delete;

    Window & operator=(const Window &) = delete;

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
        if (this->window_)
        {
            this->UnbindCloseHandler_();
            this->window_->Close();
        }
    }

    void Close()
    {
        if (this->window_)
        {
            this->window_->Close(true);
        }
    }

    wxWindow * Get()
    {
        return this->window_;
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
        this->window_->Bind(wxEVT_CLOSE_WINDOW, &Window::OnClose_, this);
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
