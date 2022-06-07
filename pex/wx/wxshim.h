/**
 ** The wx code base is littered with old-style code that fails to
 ** compile with our strict compiler warnings. Wrap wx #includes in this shim
 ** to ignore warnings only for that include.
 **/
#pragma once

#ifndef wxIMPLEMENT_WXWIN_MAIN

#ifdef __WXMAC__

#include <ApplicationServices/ApplicationServices.h>

#define wxIMPLEMENT_WXWIN_MAIN                                                \
    int main(int argc, char **argv)                                           \
    {                                                                         \
        wxDISABLE_DEBUG_SUPPORT();                                            \
                                                                              \
        ProcessSerialNumber psn = {0, kCurrentProcess};                       \
        TransformProcessType(&psn, kProcessTransformToForegroundApplication); \
        return wxEntry(argc, argv);                                           \
    }
#endif // __WXMAC__

#endif // not defined wxIMPLEMENT_WXWIN_MAIN

#include "pex/wx/ignores.h"


WXSHIM_PUSH_IGNORES
#include "wx/wx.h"
WXSHIM_POP_IGNORES


#define wxshimIMPLEMENT_APP(appClass) \
    WXSHIM_PUSH_IGNORES \
    wxIMPLEMENT_APP(appClass); \
    WXSHIM_POP_IGNORES

#define wxshimIMPLEMENT_APP_CONSOLE(appClass) \
    WXSHIM_PUSH_IGNORES \
    wxIMPLEMENT_APP_CONSOLE(appClass); \
    WXSHIM_POP_IGNORES

