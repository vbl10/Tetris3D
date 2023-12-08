#include "guipp_text_box.h"
#include <iostream>
#include <cwctype>

guipp::TextBox::TextBox(ext::TextFormat font, int nMinLines, ScrollBar* sbv, ScrollBar* sbh)
	:font(font), sbv(sbv), sbh(sbh), nMinLines(nMinLines), timer_blink(guipp::NewId(), 500)
{
	font->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	font->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	font->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	layout = font(L"");
	DWRITE_TEXT_METRICS metrics;
	layout->GetMetrics(&metrics);
	fPxFontHeight = metrics.height;
}


void guipp::TextBox::OnDraw(ext::D2DGraphics& gfx)
{
	auto rrect = D2D1::RoundedRect(
		D2D1::RectF(GetPos().x, GetPos().y, GetPos().x + GetSize().x, GetPos().y + GetSize().y),
		guipp::fCornerRadius, guipp::fCornerRadius);

	gfx.pSolidBrush->SetColor(D2D1::ColorF(0));
	gfx.pRenderTarget->FillRoundedRectangle(rrect, gfx.pSolidBrush);

	gfx.pSolidBrush->SetColor(D2D1::ColorF(bActive ? 0xffffff : 0x505050));
	gfx.pRenderTarget->DrawRoundedRectangle(rrect, gfx.pSolidBrush, 1.5f);

	gfx.pRenderTarget->PushAxisAlignedClip(
		D2D1::RectF(
			GetPos().x,
			GetPos().y,
			GetPos().x + GetSize().x - guipp::fSpacing,
			GetPos().y + GetSize().y - guipp::fSpacing),
		D2D1_ANTIALIAS_MODE_ALIASED);

	ext::vec2d<float> offset = { 0.0f,0.0f };
	DWRITE_TEXT_METRICS metrics;
	if (sbv || sbh)
		layout->GetMetrics(&metrics);
	if (sbv)
	{
		offset.y = std::min(0.0f, -sbv->fSliderValue * (metrics.height - (GetSize().y - 2.0f * guipp::fSpacing)));
	}
	if (sbh)
	{
		offset.x = std::min(0.0f, -sbh->fSliderValue * (metrics.widthIncludingTrailingWhitespace - (GetSize().x - 2.0f * guipp::fSpacing)));
	}

	gfx.pSolidBrush->SetColor(D2D1::ColorF(0xffffff));
	gfx.pRenderTarget->DrawTextLayout(
		D2D1::Point2F(GetPos().x + offset.x + guipp::fSpacing, GetPos().y + offset.y + guipp::fSpacing),
		layout, gfx.pSolidBrush);

	if (caret.nShow % 2)
	{
		DWRITE_HIT_TEST_METRICS htm;
		ext::vec2d<float> mark;
		layout->HitTestTextPosition(caret.nPos, false, &mark.x, &mark.y, &htm);
		mark += GetPos() + offset + guipp::fSpacing;

		gfx.pRenderTarget->DrawLine(
			D2D1::Point2F(mark.x, mark.y),
			D2D1::Point2F(mark.x, mark.y + htm.height),
			gfx.pSolidBrush, 1.5f);
	}

	gfx.pRenderTarget->PopAxisAlignedClip();
}

void guipp::TextBox::OnInitialize(guipp::Window& wnd, bool bInitialize)
{
	if (bActive && !bInitialize)
		wnd.Unbind(WM_TIMER, this);
}
bool guipp::TextBox::OnKbdMessage(guipp::Window& wnd, UINT msg, unsigned key_code, LPARAM lParam)
{
	if (msg == WM_CHAR)
	{
		if (!OnChar || !OnChar(*this, key_code))
		{
			switch (key_code)
			{
			default:
				if (std::isprint(key_code) || std::iswspace(key_code) || key_code > 160)
				{
					if (key_code == '\r')
						key_code = '\n';
					text.insert(text.begin() + caret.nPos, key_code);
					caret.nPos++;
				}
				break;
			case '\n':
				for (caret.nPos -= caret.nPos > 0; caret.nPos > 0 && text[caret.nPos - 1] != '\n'; caret.nPos--);
				text.insert(text.begin() + caret.nPos, L'\n');
				break;
			case '\b':
				if (caret.nPos > 0)
					text.erase(text.begin() + --caret.nPos);
				break;
			case 127:
				//ctrl + backspace (delete whole word)
				if (caret.nPos > 0)
				{
					auto first = text.begin() + --caret.nPos, last = first;
					for (bool last_is_ws = std::iswspace(*last); first != text.begin(); first--, caret.nPos--)
						if ((last_is_ws) ^ bool(std::iswspace(*(first - 1))))
							break;
					text.erase(first, last + 1);
				}
				break;
			}
		}
		layout = font(text, GetSize().x - 2.0f * guipp::fSpacing, GetSize().y - 2.0f * guipp::fSpacing);

		if (OnLayoutRecreated)
			OnLayoutRecreated(*this);

		DWRITE_TEXT_METRICS metrics;
		if (sbv || sbh)
			layout->GetMetrics(&metrics);
		if (sbv)
		{
			sbv->fSliderSize =
				std::min(
					1.0f,
					(GetSize().y - 2.0f * guipp::fSpacing) / metrics.height
				);
		}
		if (sbh)
		{
			sbh->fSliderSize =
				std::min(
					1.0f,
					(GetSize().x - 2.0f * guipp::fSpacing) / metrics.widthIncludingTrailingWhitespace
				);
		}
	}
	else if (msg == WM_KEYDOWN)
	{
		switch (key_code)
		{
		case VK_DELETE:
			if (caret.nPos < text.size())
			{
				if (GetAsyncKeyState(VK_CONTROL))
				{
					auto first = text.begin() + caret.nPos, last = first;
					for (bool first_is_ws = std::iswspace(*first); last + 1 != text.end(); last++)
						if ((first_is_ws) ^ bool(std::iswspace(*(last + 1))))
							break;
					text.erase(first, last + 1);
				}
				else
				{
					text.erase(text.begin() + caret.nPos);
				}
				layout = font(text, GetSize().x - 2.0f * guipp::fSpacing, GetSize().y - 2.0f * guipp::fSpacing);
			}
			break;
		case VK_RIGHT:
			if (GetAsyncKeyState(VK_CONTROL))
			{
				if (caret.nPos < text.size())
				{
					auto it = text.begin() + caret.nPos;
					for (bool was_ws = std::iswspace(*it); it + 1 != text.end(); it++, caret.nPos++)
						if ((was_ws) ^ bool(std::iswspace(*(it + 1))))
							break;
					caret.nPos++;
				}
			}
			else
				caret.nPos += caret.nPos < text.size();
			break;
		case VK_LEFT:
			if (GetAsyncKeyState(VK_CONTROL))
			{
				if (caret.nPos > 0)
				{
					auto it = text.begin() + --caret.nPos;
					for (bool was_ws = std::iswspace(*it); it != text.begin(); it--, caret.nPos--)
						if ((was_ws) ^ bool(std::iswspace(*(it - 1))))
							break;
				}
			}
			else
				caret.nPos -= caret.nPos > 0;
			break;
		case VK_UP:
			if (size_t n1 = text.find_last_of('\n', caret.nPos); n1 != text.npos)
			{
				int offset = caret.nPos - n1 - 1;
				caret.nPos = text.find_last_of('\n', n1 - 1);
				caret.nPos = caret.nPos == text.npos ? 0 : caret.nPos + 1;
				caret.nPos += std::min((int)n1 - caret.nPos, offset);
			}
			break;
		case VK_DOWN:
			if (size_t n1 = text.find_first_of('\n', caret.nPos); n1 != text.npos)
			{
				size_t n0 = caret.nPos > 0 ? text.find_last_of('\n', caret.nPos - 1) : text.npos;
				int offset = caret.nPos - (n0 == text.npos ? 0 : n0 + 1);
				size_t n2 = text.find_first_of('\n', n1 + 1);
				n2 = n2 == text.npos ? text.size() - 1 : n2;
				int length = n2 - n1;
				offset = std::min(offset, length);
				caret.nPos = n1 + 1 + offset;
			}
			break;
		case VK_TAB:
			return true;
			break;
		default:
			return false;
			break;
		}
	}
	else
	{
		return false;
	}
	wnd.RequestRedraw();
	caret.nShow = 1;
	timer_blink.Reset(wnd.hWnd);
	return true;
}
bool guipp::TextBox::OnMouseMessage(guipp::Window& wnd, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		//start selection
		break;
	default:
		return false;
		break;
	}
	return true;
}
guipp::Object* guipp::TextBox::OnMouseHitTest(const ext::vec2d<float>& mpos_t)
{
	if (mpos_t.x >= GetPos().x && mpos_t.y >= GetPos().y &&
		mpos_t.x < GetPos().x + GetSize().x && mpos_t.y < GetPos().y + GetSize().y)
	{
		return this;
	}
	return nullptr;
}
guipp::Object* guipp::TextBox::OnKbdFocus(Window& wnd, bool bFirst)
{
	wnd.Bind(WM_TIMER, this);
	timer_blink.Reset(wnd.hWnd);
	caret.nShow = 1;
	bActive = true;
	wnd.RequestRedraw();
	return this;
}
void guipp::TextBox::OnKbdUnfocus(Window& wnd)
{
	timer_blink.Stop(wnd.hWnd);
	caret.nShow = 0;
	bActive = false;
	wnd.RequestRedraw();
}
void guipp::TextBox::OnMessage(Window& wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (wParam == timer_blink.id)
	{
		if (caret.nShow < 9)
		{
			caret.nShow++;
		}
		else
		{
			timer_blink.Stop(wnd.hWnd);
		}
		wnd.RequestRedraw();
	}
}

ext::vec2d<float> guipp::TextBox::OnMinSizeUpdate()
{
	return ext::vec2d<float>{ 0.0f,fPxFontHeight * nMinLines } + 2.0f * guipp::fSpacing;
}

void guipp::TextBox::Timer::Reset(HWND hWnd)
{
	if (bSet)
	{
		KillTimer(hWnd, id);
	}
	SetTimer(hWnd, id, ms, NULL);
	bSet = true;
}
void guipp::TextBox::Timer::Stop(HWND hWnd)
{
	if (bSet)
	{
		KillTimer(hWnd, id);
		bSet = false;
	}
}
