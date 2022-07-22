#pragma once


#include "pex/wx/wxshim.h"


namespace pex
{


namespace wx
{


// Call Layout on the top level parent of window.
void LayoutTopLevel(wxWindow *window);


} // end namespace wx


} // end namespace pex
