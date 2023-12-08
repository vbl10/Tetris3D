#include "guipp.h"
#include <chrono>
#include <thread>

using namespace guipp;
using ext::vec2d;

float guipp::fSpacing = 5.0f;
float guipp::fCornerRadius = 3.0f;
float guipp::fStrokeWidth = 2.0f;

unsigned nIdCount = 0;
unsigned guipp::NewId()
{
	return nIdCount++;
}

void guipp::MessagePump(Window** ppWnd, const std::wstring& title, std::shared_ptr<Object> source, const ext::vec2d<int>& init_size, bool bGraphicResize, const wchar_t* wnd_class)
{
	Window* pWnd = new Window(title, source, init_size, bGraphicResize, wnd_class);

	if (ppWnd)
		*ppWnd = pWnd;

	MSG msg;
	while (1)
	{
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
			{
				delete pWnd;
				return;
			}
		}
		if (!pWnd->Update())
		{
			WaitMessage();
		}
	}
}
void guipp::MakeWindow(Window** ppWnd, bool bJoin, const std::wstring& title, std::shared_ptr<Object> source, const ext::vec2d<int>& init_size, bool bGraphicResize, const wchar_t* wnd_class)
{
	std::thread pump(MessagePump, ppWnd, title, source, init_size, bGraphicResize, wnd_class);
	if (bJoin)
		pump.join();
	else
		pump.detach();
}

void Object::Reshuffle()
{
	Object* p;
	for (p = this; p->parent; p = p->parent)
	{
		p->bMinSizeUpToDate = false;
	}
	p->bMinSizeUpToDate = false;
	vec2d<float> new_size = p->GetMinSize();
	new_size.x = std::max(new_size.x, p->GetSize().x);
	new_size.y = std::max(new_size.y, p->GetSize().y);
	p->SetSize(new_size);
}
const vec2d<float>& Object::GetMinSize()
{
	if (!bMinSizeUpToDate)
		min_size = OnMinSizeUpdate();
	bMinSizeUpToDate = true;
	return min_size;
}

D2D1::ColorF Window::color = D2D1::ColorF(0x151515);
Window::Window(const std::wstring& title, std::shared_ptr<Object> _source, const vec2d<int>& init_size, bool bGraphicResize, const wchar_t* wnd_class)
	:
	ext::Window(title.c_str(), init_size, wnd_class, WS_OVERLAPPEDWINDOW),
	D2DGraphics(hWnd),
	bGraphicResize(bGraphicResize),
	init_size(init_size),
	source(_source)
{
	source->SetParent(this);
	source->OnGfxCreated(*this);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE);
	if (cdim != GetSize() + 0.5f)
	{
		SetSize(cdim.to<float>());
	}
	source->OnInitialize(*this, true);
}
Window::~Window()
{
	if (source)
		source->OnInitialize(*this, false);
}

bool Window::Bind(UINT msg, MessageProcedure* proc)
{
	if (std::find(
		procedures[msg].begin(),
		procedures[msg].end(),
		proc) == procedures[msg].end())
	{
		procedures[msg].push_back(proc);
		return true;
	}
	return false;
}
bool Window::Unbind(UINT msg, MessageProcedure* proc)
{
	if (auto it = std::find(procedures[msg].begin(), procedures[msg].end(), proc);
		it != procedures[msg].end())
	{
		procedures[msg].erase(it);
		if (procedures[msg].empty())
			procedures.erase(msg);
		return true;
	}
	return false;
}
void Window::AddToUpdateLoop(Updatable* proc)
{
	update_loop.push_back(proc);
}
void Window::RequestRedraw()
{
	bRedraw = true;
}
void Window::SetKbdTarget(Object* new_kbd_target)
{
	if (kbd_target != new_kbd_target)
	{
		if (kbd_target)
			kbd_target->OnKbdUnfocus(*this);
		if (new_kbd_target)
			new_kbd_target->OnKbdFocus(*this, true);
		kbd_target = new_kbd_target;
	}
}
void Window::UpdateMouseTarget()
{
	if (mouse_target)
		mouse_target->OnMouseFocus(*this, false);
	mouse_target = source->OnMouseHitTest(mpos.to<float>() / fScale);
	if (mouse_target)
		mouse_target->OnMouseFocus(*this, true);
}

LRESULT Window::AppProc(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool bContains = false;
	switch (msg)
	{
	case WM_CLOSE:
		if (!(bContains |= procedures.contains(msg)))
			PostQuitMessage(0);
		break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* info = (MINMAXINFO*)lParam;
		RECT rc = { 0 };
		rc.right = std::ceil(GetMinSize().x);
		rc.bottom = std::ceil(GetMinSize().y);
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		info->ptMinTrackSize.x = rc.right - rc.left;
		info->ptMinTrackSize.y = rc.bottom - rc.top;
		return 0;
	}break;
	case WM_SIZE:
		if (pRenderTarget)
		{
			((ID2D1HwndRenderTarget*)pRenderTarget.p)->Resize(D2D1::SizeU(cdim.x, cdim.y));
		}
		SetSize(cdim.to<float>());
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);

		OnDraw(*this);

		EndPaint(hWnd, &ps);
	}
	break;
	}

	if (bContains || procedures.contains(msg))
		for (auto proc : procedures[msg])
			proc->OnMessage(*this, msg, wParam, lParam);

	if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST)
	{
		if (last_key_code_processed != wParam)
		{
			if ((GetKeyState(VK_CONTROL) && (wParam == VK_TAB || wParam == VK_HOME || wParam == VK_END))
				|| !kbd_target || !kbd_target->OnKbdMessage(*this, msg, wParam, lParam))
			{
				switch (wParam)
				{
				case VK_TAB:
				{
					if (msg == WM_KEYDOWN)
					{
						Object* child = nullptr;
						while (kbd_target)
						{
							if (auto next = kbd_target->OnKbdNext(*this, child, !GetAsyncKeyState(VK_SHIFT)); next)
							{
								kbd_target = next;
								break;
							}
							else
							{
								child = kbd_target;
								kbd_target = kbd_target->GetParent();
							}
						}
						if (!kbd_target)
						{
							kbd_target = source->OnKbdFocus(*this, !GetAsyncKeyState(VK_SHIFT));
						}
						last_key_code_processed = wParam;
					}
				}break;
				default:
					if (msg == WM_KEYUP)
					{
						if (wParam == VK_HOME || wParam == VK_END)
						{
							if (kbd_target)
								kbd_target->OnKbdUnfocus(*this);
							kbd_target = source->OnKbdFocus(*this, wParam == VK_HOME);
						}
					}
					break;
				}
			}
		}
		if (msg == WM_KEYUP)
		{
			last_key_code_processed = 0;
		}
	}
	else if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
	{
		auto mpos_t = mpos.to<float>() / fScale;
		if (!mouse_target || !mouse_target->OnMouseMessage(*this, msg, wParam, mpos_t))
		{
			if (msg == WM_MOUSEMOVE)
			{
				if (!mouse_target || !mouse_target->OnMouseHitTest(mpos_t))
				{
					if (mouse_target)
						mouse_target->OnMouseFocus(*this, false);
					mouse_target = source->OnMouseHitTest(mpos_t);
					if (mouse_target)
					{
						mouse_target->OnMouseFocus(*this, true);
						mouse_target->OnMouseMessage(*this, WM_MOUSEMOVE, wParam, mpos_t);
					}
				}
			}
		}
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}
bool Window::Update()
{
	if (!update_loop.empty())
	{
		if (bFirstUpdateAfterWait)
		{
			bFirstUpdateAfterWait = false;
			fElapsedTime = 0.0f;
			tp1 = std::chrono::steady_clock::now();
		}

		for (auto it = update_loop.begin(); it != update_loop.end();)
		{
			if ((*it)->OnUpdate(*this, fElapsedTime))
				it++;
			else
				it = update_loop.erase(it);
		}
	}
	if (bRedraw)
	{
		OnDraw(*this);
	}

	if (update_loop.empty())
	{
		bFirstUpdateAfterWait = true;
		return false;
	}
	else
	{
		fElapsedTime = std::chrono::duration<float>(std::chrono::steady_clock::now() - tp1).count();
		tp1 = std::chrono::steady_clock::now();
		return true;
	}
}
void Window::OnSetSize()
{
	if (cdim != GetSize() + 0.5f)
	{
		RECT rc = { 0 };
		cdim.x = rc.right = std::ceil(GetSize().x);
		cdim.y = rc.bottom = std::ceil(GetSize().y);
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE);
		if (pRenderTarget)
			((ID2D1HwndRenderTarget*)pRenderTarget.p)->Resize(D2D1::SizeU(cdim.x, cdim.y));
	}

	if (bGraphicResize && (GetSize().x != 0.0f && GetSize().y != 0.0f))
	{
		float fScaleX = GetSize().x / source->GetMinSize().x;
		float fScaleY = GetSize().y / source->GetMinSize().y;
		if (fScaleX < fScaleY)
		{
			fScale = fScaleX;
			source->SetSize({ source->GetMinSize().x, GetSize().y / fScale});
		}
		else
		{
			fScale = fScaleY;
			source->SetSize({ GetSize().x / fScale,source->GetMinSize().y });
		}
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Scale(fScale, fScale));
	}
	else
	{
		source->SetSize(GetSize());
	}
	source->SetPos({ 0.0f,0.0f });
	OnDraw(*this);
}
vec2d<float> Window::OnMinSizeUpdate()
{
	return {
		std::max((float)init_size.x, source->GetMinSize().x),
		std::max((float)init_size.y, source->GetMinSize().y)
	};
}
void Window::OnDraw(D2DGraphics&)
{
	if (pRenderTarget)
	{
		pRenderTarget->BeginDraw();
		pRenderTarget->Clear(color);
		source->OnDraw(*this);
		pRenderTarget->EndDraw();
		bRedraw = false;
	}
}