/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#define VLC2X 

#ifdef VLC2X
    
    #pragma include_alias (<vlc/vlc.h>                      , <vlc2x/vlc.h>                        )
    #pragma include_alias (<vlc/libvlc_structures.h>        , <vlc2x/libvlc_structures.h>          )
    #pragma include_alias (<vlc/libvlc.h>                   , <vlc2x/libvlc.h>                     )
    #pragma include_alias (<vlc/libvlc_media.h>             , <vlc2x/libvlc_media.h>               )
    #pragma include_alias (<vlc/libvlc_media_player.h>      , <vlc2x/libvlc_media_player.h>        )
    #pragma include_alias (<vlc/libvlc_media_list.h>        , <vlc2x/libvlc_media_list.h>          )
    #pragma include_alias (<vlc/libvlc_media_list_player.h> , <vlc2x/libvlc_media_list_player.h>   )
    #pragma include_alias (<vlc/libvlc_media_library.h>     , <vlc2x/libvlc_media_library.h>       )
    #pragma include_alias (<vlc/libvlc_media_discoverer.h>  , <vlc2x/libvlc_media_discoverer.h>    )
    #pragma include_alias (<vlc/libvlc_events.h>            , <vlc2x/libvlc_events.h>              )
    #pragma include_alias (<vlc/libvlc_vlm.h>               , <vlc2x/libvlc_vlm.h>                 )
    #pragma include_alias (<vlc/deprecated.h>               , <vlc2x/deprecated.h>                 )

    #include "vlc2x/vlc.h"
    
    #ifdef _WIN64
        #pragma comment(lib, "libvlc2x-x64.lib")
    #else
        #pragma comment(lib, "libvlc2x-x86.lib")
    #endif

#else //VLC20

    #include "vlc/vlc.h"

    #ifdef _WIN64
        #pragma comment(lib, "libvlc20-x64.lib")
    #else
        #pragma comment(lib, "libvlc20-x86.lib")
    #endif
    
#endif