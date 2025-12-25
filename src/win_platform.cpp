#include <windows.h>

constexpr const auto WC_NAME = TEXT("rendered_window");
constexpr const auto WIN_TITLE = TEXT("Rendered Window");
constexpr int WIN_WIDTH = 640;
constexpr int WIN_HEIGHT = 480;

void* pixel_buf;
HDC mem_dc;

static void registerWindowClass(WNDPROC win_proc_func, HINSTANCE h_instance) {
	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;  // repaint the whole window if it's resized
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

static void fillBlue(void* buf) {
	UINT32* pixels =  static_cast<UINT32*>(buf);
	for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
		pixels[i] = 0x000000FF;
	}
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {
	switch (u_msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		BitBlt(ps.hdc, 0, 0, WIN_WIDTH, WIN_HEIGHT, mem_dc, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
		return 0;
	default:
		return DefWindowProc(hwnd, u_msg, w_param, l_param);
	}
}

void createPixelBuffer(LONG width, LONG height, void** pixel_buf, HDC* mem_dc) {
	BITMAPINFOHEADER header = {};
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = width;
	header.biHeight = -height;
	header.biPlanes = 1;
	header.biBitCount = 32;
	header.biCompression = BI_RGB;

	BITMAPINFO info = {};
	info.bmiHeader = header;

	HBITMAP dib = CreateDIBSection(
		nullptr, &info, DIB_RGB_COLORS,
		pixel_buf, nullptr, 0
	);
	HDC dc = CreateCompatibleDC(nullptr);
	SelectObject(dc, dib);
	*mem_dc = dc;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int cmdShow) {
	registerWindowClass(windowProc, hInstance);
	HWND window = CreateWindow(
		WC_NAME, WIN_TITLE,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WIN_WIDTH, WIN_HEIGHT,
		NULL, NULL, hInstance, NULL
	);
	
	createPixelBuffer(WIN_WIDTH, WIN_HEIGHT, &pixel_buf, &mem_dc);
	fillBlue(pixel_buf);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
