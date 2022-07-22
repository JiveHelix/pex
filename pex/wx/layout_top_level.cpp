#include "pex/wx/layout_top_level.h"


namespace pex
{


namespace wx
{


// Call Layout on the top level parent of window.
void LayoutTopLevel(wxWindow *window)
{
    auto topLevel = wxGetTopLevelParent(window);

    if (!topLevel)
    {
        return;
    }

    topLevel->Layout();
}


} // end namespace wx


} // end namespace pex
