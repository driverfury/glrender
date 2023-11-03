#include <windows.h>
#include "../glrender.h"
#include "../glrender.c"

static int app_is_running;

void win32_swap_buffers(HWND window_handle);

static LRESULT CALLBACK
win32_window_proc(
    HWND   window,
    UINT   message,
    WPARAM w_param,
    LPARAM l_param)
{
    LRESULT result = 0;

    switch(message)
    {
        case WM_QUIT:
        case WM_DESTROY:
        case WM_CLOSE:
        {
            app_is_running = 0;
        } break;

        case WM_PAINT:
        {
            win32_swap_buffers(window);
            result = DefWindowProcA(window, message, w_param, l_param);
        } break;

        default:
        {
            result = DefWindowProcA(window, message, w_param, l_param);
        } break;
    }

    return(result);
}

int
win32_init_gl_context(HWND window_handle)
{
    HDC window_dc = GetDC(window_handle);
    if(!window_dc) return(0);

    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.iPixelType = PFD_TYPE_RGBA;

    int suggested_pfd_num = ChoosePixelFormat(window_dc, &pfd);
    if(!suggested_pfd_num) return(0);

    PIXELFORMATDESCRIPTOR suggested_pfd = {0};
    if(!DescribePixelFormat(window_dc, suggested_pfd_num, sizeof(suggested_pfd), &suggested_pfd))
    {
        return(0);
    }

    if(!SetPixelFormat(window_dc, suggested_pfd_num, &suggested_pfd))
    {
        return(0);
    }

    HGLRC gl_render_context = wglCreateContext(window_dc);
    if(!gl_render_context)
    {
        return(0);
    }

    if(!wglMakeCurrent(window_dc, gl_render_context))
    {
        return(0);
    }

    ReleaseDC(window_handle, window_dc);

    return(1);
}

void
win32_swap_buffers(HWND window_handle)
{
    HDC window_dc = GetDC(window_handle);
    SwapBuffers(window_dc);
    ReleaseDC(window_handle, window_dc);
}

glr_renderer renderer;

#define IMG_WIDTH 2
#define IMG_HEIGHT 2
unsigned char img_raw_data[IMG_WIDTH*IMG_HEIGHT][4] = {
    /* R     G     B     A         R      G     B     A */
    { 0xff, 0x00, 0x00, 0xff }, { 0x00, 0xff, 0x00, 0xff }, 
    { 0x00, 0x00, 0xff, 0xff }, { 0xff, 0xff, 0xff, 0xff }
};

int
main()
{
    int screen_width = 800;
    int screen_height = 600;

    char *window_class_name = "glr_testbed_win_class";
    DWORD window_style = WS_OVERLAPPEDWINDOW;
    char *window_title = "glrender testbed";

    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = 0;
    window_class.hIcon = LoadIconA(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursorA(0, IDC_ARROW);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = window_class_name;
    if(!RegisterClassA(&window_class))
    {
        return(0);
    }

    RECT window_rect = {0};
    window_rect.left   = 0;
    window_rect.top    = 0;
    window_rect.right  = screen_width + window_rect.left;
    window_rect.bottom = screen_height + window_rect.top;
    if(!AdjustWindowRect(&window_rect, window_style, 0))
    {
        // TODO: Log
        return(0);
    }
    int window_width  = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;

    HWND window_handle = CreateWindowExA(
        0,
        window_class_name,
        window_title,
        window_style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_width, window_height,
        0, 0, 0, 0);
    if(!window_handle)
    {
        // TODO: Log
        return(0);
    }

    // NOTE: If we don't do this non-sense, the GetClientRect() function keeps returning the wrong values.
    SetWindowPos(window_handle, 0, 0, 0, 0, 0, SWP_NOMOVE);
    SetWindowPos(window_handle, 0, 0, 0, window_width, window_height, SWP_NOMOVE);

    if(!win32_init_gl_context(window_handle))
    {
        return(0);
    }

    glr_init(&renderer, GLR_ALPHA_BLEND | GLR_DEPTH_TEST);
    glr_texture texture = glr_texture_create_from_raw_data(
        (void *)img_raw_data,
        IMG_WIDTH,
        IMG_HEIGHT,
        GLR_PIXEL_FORMAT_RGBA);
    if(!texture)
    {
        return(0);
    }

    ShowWindow(window_handle, SW_SHOW);

    app_is_running = 1;
    while(app_is_running)
    {
        MSG message;
        while(PeekMessageA(&message, window_handle, 0, 0, PM_REMOVE))
        {
            switch(message.message)
            {
                case WM_QUIT:
                case WM_DESTROY:
                case WM_CLOSE:
                {
                    app_is_running = 0;
                } break;

                default:
                {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                } break;
            }
        }

        glr_begin(&renderer);

        glr_clear(&renderer, 0.1f, 0.1f, 0.2f, 1.0f);

        glr_vertex quad1[4] = {
            { -0.5f, -0.5f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, texture },
            {  0.5f, -0.5f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, texture },
            {  0.5f,  0.5f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, texture },
            { -0.5f,  0.5f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, texture }
        };
        glr_quad_push(&renderer, quad1);

        glr_vertex quad2[4] = {
            { -0.75f,  0.8f, 0.8f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0 },
            {  0.75f,  0.8f, 0.8f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0 },
            {  0.75f,  1.0f, 0.8f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f, 1.0f, 0 },
            { -0.75f,  1.0f, 0.8f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 0 }
        };
        glr_quad_push(&renderer, quad2);

        glr_vertex triangle1[3] = {
            { -1.0f, -1.0f, 0.8f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0 },
            { -0.8f, -1.0f, 0.8f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0 },
            { -1.0f, -0.8f, 0.8f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0 },
        };
        glr_triangle_push(&renderer, triangle1);

        glr_end(&renderer);

        win32_swap_buffers(window_handle);
    }

    return(0);
}
