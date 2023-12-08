#pragma once
#include "guipp.h"
#include "guipp_scroll_bar.h"
#include <functional>

namespace guipp
{
	class TextBox : public Object, private MessageProcedure
	{
	public:
		TextBox(ext::TextFormat font, int nMinLines = 1, ScrollBar* sbv = nullptr, ScrollBar* sbh = nullptr);

		void OnDraw(ext::D2DGraphics& gfx) override;
		
		ext::TextFormat font;
		
		//return false to let default procedure take place
		std::function<bool(TextBox&, wchar_t)> OnChar;
		std::function<void(TextBox&)> OnLayoutRecreated;

	private:
		void OnInitialize(Window& wnd, bool bInitialize) override;
		bool OnKbdMessage(Window& wnd, UINT msg, unsigned key_code, LPARAM lParam) override;
		bool OnMouseMessage(Window& wnd, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t) override;
		Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) override;
		Object* OnKbdFocus(Window& wnd, bool bFirst) override;
		void OnKbdUnfocus(Window& wnd) override;
		void OnMessage(Window& wnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
		ext::vec2d<float> OnMinSizeUpdate() override;
		bool IsKbdWeak() override { return false; }

		ScrollBar* sbh;
		ScrollBar* sbv;

		float fPxFontHeight;
		int nMinLines = 1;

		struct Timer
		{
			Timer(int id, int ms) :id(id), ms(ms) {}
			void Reset(HWND hWnd);
			void Stop(HWND hWnd);
			const int id;
			int ms;
		private:
			bool bSet = false;
		}timer_blink;

		struct Caret
		{
			int nShow = 0;
			bool bEmplace = false;
			int nPos = 0;
		};
		bool bActive = false;
	public:
		CComPtr<IDWriteTextLayout> layout;
		std::wstring text;
		Caret caret;
	};
};