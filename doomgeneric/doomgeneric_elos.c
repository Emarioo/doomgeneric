//doomgeneric for cross-platform development library 'Simple DirectMedia Layer'


#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include "elos/syscalls.h"
#include "prism/prism.h"
#include "elos/common/intrinsics.h"

#include "stdio.h"
#include "string.h"


PrismInstance* g_instance;
PrismSurface* g_surface;
PrismSurfaceInfo g_surfaceInfo;
u64 ticks_per_second;
u64 tick_offset;
ELOS_UserEventBuffer* userEvents;


static unsigned char convertToDoomKey(unsigned int key){
    switch (key)
        {
        case ELOSKEY_ENTER:
            key = KEY_ENTER;
            break;
        case ELOSKEY_ESCAPE:
            key = KEY_ESCAPE;
            break;
        case ELOSKEY_LEFT_ARROW:
            key = KEY_LEFTARROW;
            break;
        case ELOSKEY_RIGHT_ARROW:
            key = KEY_RIGHTARROW;
            break;
        case ELOSKEY_UP_ARROW:
            key = KEY_UPARROW;
            break;
        case ELOSKEY_DOWN_ARROW:
            key = KEY_DOWNARROW;
            break;
        case ELOSKEY_LEFT_CTRL:
        case ELOSKEY_RIGHT_CTRL:
            key = KEY_FIRE;
            break;
        case ELOSKEY_SPACE:
            key = KEY_USE;
            break;
        case ELOSKEY_LEFT_SHIFT:
        case ELOSKEY_RIGHT_SHIFT:
            key = KEY_RSHIFT;
            break;
        case ELOSKEY_LEFT_ALT:
        case ELOSKEY_RIGHT_ALT:
            key = KEY_LALT;
            break;
        case ELOSKEY_F2:
            key = KEY_F2;
            break;
        case ELOSKEY_F3:
            key = KEY_F3;
            break;
        case ELOSKEY_F4:
            key = KEY_F4;
            break;
        case ELOSKEY_F5:
            key = KEY_F5;
            break;
        case ELOSKEY_F6:
            key = KEY_F6;
            break;
        case ELOSKEY_F7:
            key = KEY_F7;
            break;
        case ELOSKEY_F8:
            key = KEY_F8;
            break;
        case ELOSKEY_F9:
            key = KEY_F9;
            break;
        case ELOSKEY_F10:
            key = KEY_F10;
            break;
        case ELOSKEY_F11:
            key = KEY_F11;
            break;
        case ELOSKEY_EQUAL:
        case ELOSKEY_PLUS:
            key = KEY_EQUALS;
            break;
        case ELOSKEY_MINUS:
            key = KEY_MINUS;
            break;
        case ELOSKEY_BACKSPACE:
            key = KEY_BACKSPACE;
            break;
        case ELOSKEY_DELETE:
            key = KEY_DEL;
            break;
        case ELOSKEY_HOME:
            key = KEY_HOME;
            break;
        case ELOSKEY_END:
            key = KEY_END;
            break;
        default:
            if (key >= ELOSKEY_A && key <= ELOSKEY_Z) {
                key = key - ELOSKEY_A + 'a';
            } else if (key >= ELOSKEY_0 && key <= ELOSKEY_9) {
                key = key - ELOSKEY_A + 'a';
            } else {
                key = tolower(key);
            }
            break;
    }

    return key;
}



bool get_event(ELOS_UserEvent* event) {
    // @TODO Not thread or context switch safe.
    u32 tail = userEvents->tail % userEvents->maxEvents;
    u32 head = userEvents->head % userEvents->maxEvents;
    if (tail == head) {
        return false;
    }
    *event = userEvents->events[tail];
    userEvents->tail++;
    return true;
}



void DG_Init(){
    tick_offset = rdtsc(); 
    SYS_ticks_per_second(&ticks_per_second);

    g_instance = prism_init();
    if (!g_instance) {
        printf("terminal: Could not init PRISM client\n");
        exit(1);
    }

    g_surface = prism_createSurface(g_instance, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    if (!g_surface) {
        printf("terminal: Could not create surface\n");
        exit(1);
    }

    prism_surfaceInfo(g_surface, &g_surfaceInfo);

    prism_moveSurface(g_surface, g_surfaceInfo.width - DOOMGENERIC_RESX, g_surfaceInfo.height - DOOMGENERIC_RESY);
    
    ELOS_Error error;
    error = SYS_request_user_event_buffer(1000, &userEvents);
    if (error != ELOS_OK) {
        printf("terminal: Could not create user event buffer\n");
        exit(1);
    }
}

void DG_DrawFrame() {
    u32 x = 0;
    u32 y = 0;
    u32 w = DOOMGENERIC_RESX;
    u32 h = DOOMGENERIC_RESY;
    uint32_t* const dst  = g_surfaceInfo.buffer;
    uint32_t  const dst_stride  = g_surfaceInfo.stride;
    uint32_t  const src_stride  = g_surfaceInfo.stride;
    uint32_t*  const src = (void*)DG_ScreenBuffer;


    // @TODO We may want to write to an internal buffer which we then write to the frame buffer in one fell swoop.

    for (int iy = y; iy < y + h; iy++) {
        void* begin = &dst[x + iy * dst_stride];
        void* from = &src[(iy-y) * src_stride];
        int size = 4 * w;
        memcpy(begin, from, size);
    }

    prism_presentSurface(g_surface);
}

void DG_SleepMs(uint32_t ms) {
    SYS_sleep_ns(1000000 * (u64)ms);
}

uint32_t DG_GetTicksMs() {
    return (1000 * (rdtsc() - tick_offset)) / ticks_per_second;
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
    ELOS_UserEvent event;
    bool has = get_event(&event);
    if (!has) {
        return 0;
    }

    if (event.type == ELOS_USER_EVENT_KEY) {
        unsigned char key = convertToDoomKey(event.key.keycode);
        *pressed = event.key.value != 0;
        *doomKey = key;
        return 1;
    }

    return 0;
}

void DG_SetWindowTitle(const char * title) {
    // if (window != NULL){
    //   SDL_SetWindowTitle(window, title);
    // }
}

void _start() {
    char* argv[] = {
        "doom",
        "-iwad",
        "/pkg/doom/doom1.wad",
        NULL
    };
    int res;
    res = mkdir("/home", 0);
    if (res) {
        printf("Could not make /home\n");
    }
    res = mkdir("/tmp", 0);
    if (res) {
        printf("Could not make /tmp\n");
    }
    res = chdir("/home");
    if (res) {
        printf("Could not set cwd = /home\n");
    }

    int argc = sizeof(argv)/sizeof(*argv) - 1;
    doomgeneric_Create(argc, argv);

    for (int i = 0; ; i++)
    {
        doomgeneric_Tick();
    }
    
}