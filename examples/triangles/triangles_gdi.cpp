#include <cstdint>
#include <cstdlib>

#include <windows.h>

#include <cascade/fragment_ops.h>
#include <cascade/rasterizer.h>

using namespace cascade;

constexpr const auto WC_NAME = TEXT("rendered_window");
constexpr const auto WIN_TITLE = TEXT("Rendered Window");
constexpr int INIT_WIDTH = 640;
constexpr int INIT_HEIGHT = 480;

struct WindowState {
    HDC mem_dc;
    void* pixel_buf;
    HBITMAP dib;
    LONG width;
    LONG height;
};

static void registerWindowClass(WNDPROC win_proc_func, HINSTANCE h_instance) {
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW; // repaint the whole window if it's resized
    wc.lpfnWndProc = win_proc_func;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = h_instance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = WC_NAME;
    RegisterClass(&wc);
}

void createPixelBuffer(LONG width, LONG height, WindowState* win_state) {
    if (win_state->dib) {
        DeleteObject(win_state->dib);
    }
    if (win_state->mem_dc) {
        DeleteDC(win_state->mem_dc);
    }

    BITMAPINFOHEADER header = {};
    header.biSize = sizeof(BITMAPINFOHEADER);
    header.biWidth = width;
    header.biHeight = -height;
    header.biPlanes = 1;
    header.biBitCount = 32;
    header.biCompression = BI_RGB;

    BITMAPINFO info = {};
    info.bmiHeader = header;

    HBITMAP dib = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &win_state->pixel_buf, nullptr, 0);
    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, dib);
    win_state->mem_dc = dc;
    win_state->dib = dib;
    win_state->width = width;
    win_state->height = height;
}

static void update(void* buf, int width, int height) {
    float vertex_data[] = {50.0f,  200.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // vertex 0 (50, 200, 1, 1) RED
                           100.0f, 200.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // vertex 1 (100, 200, 1, 1) BLUE
                           75.0f,  100.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // vertex 2 (75, 100, 1, 1) GREEN
                           300.0f, 300.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // vertex 3 (300, 300, 1, 1) RED
                           300.0f, 100.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // vertex 4 (300, 100, 1, 1) RED
                           500.0f, 200.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f}; // vertex 5 (500, 200, 1, 1) BLUE
    uint32_t indices[] = {0, 2, 1, 3, 4, 5}; // Make sure the vertices are in clockwise winding order
    uint32_t index_count = 6;
    uint32_t stride_bytes = 8 * sizeof(float);
    ViewportBounds bounds = {{0, 0}, {width - 1, height - 1}};
    RasterizerInput input = {vertex_data, indices, index_count, stride_bytes, bounds};

    constexpr uint32_t FRAG_BUF_SIZE = 1024;
    void* fragment_buffer = std::malloc(FRAG_BUF_SIZE);
    if (!fragment_buffer) {
        abort();
    }

    uint32_t fragment_stride_bytes = 2 * sizeof(uint32_t) + 5 * sizeof(float);
    OutputContext context = {buf, fragment_stride_bytes, width};

    FragmentBufferInfo fbi = {fragment_buffer, FRAG_BUF_SIZE, processFragmentsWithoutDepth, &context};

    rasterize(input, fbi);

    std::free(fragment_buffer);
}

static void draw(HWND hwnd, HDC mem_dc, int width, int height) {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    BitBlt(ps.hdc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {
    if (u_msg == WM_NCCREATE) {
        // Attach window state data to the window
        CREATESTRUCT* create_params = reinterpret_cast<CREATESTRUCT*>(l_param);
        WindowState* win_state = static_cast<WindowState*>(create_params->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win_state));
        return TRUE;
    }

    WindowState* ws = reinterpret_cast<WindowState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    int width;
    int height;
    switch (u_msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            width = ws->width;
            height = ws->height;
            update(ws->pixel_buf, width, height);
            draw(hwnd, ws->mem_dc, width, height);
            return 0;
        case WM_SIZE:
            if (LOWORD(l_param) > 0 && HIWORD(l_param) > 0) {
                createPixelBuffer(LOWORD(l_param), HIWORD(l_param), ws);
            }
            DefWindowProc(hwnd, u_msg, w_param, l_param);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        default:
            return DefWindowProc(hwnd, u_msg, w_param, l_param);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int cmdShow) {
    registerWindowClass(windowProc, hInstance);
    WindowState win_state = {};

    HWND window = CreateWindow(WC_NAME, WIN_TITLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                               INIT_WIDTH, INIT_HEIGHT, NULL, NULL, hInstance, &win_state);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}
