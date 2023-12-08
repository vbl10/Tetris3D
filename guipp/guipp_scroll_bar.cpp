#include "guipp_scroll_bar.h"

guipp::ScrollBar::ScrollBar(TYPE type, float fSliderValue, float fSliderSize)
	:type(type), fSliderValue(fSliderValue), fSliderSize(fSliderSize)
{

}

void guipp::ScrollBar::OnDraw(ext::D2DGraphics& gfx)
{
	auto rrect = D2D1::RoundedRect(
		D2D1::RectF(
			GetPos().x,
			GetPos().y,
			GetPos().x + GetSize().x,
			GetPos().y + GetSize().y),
		guipp::fCornerRadius, guipp::fCornerRadius
	);

	gfx.pSolidBrush->SetColor(D2D1::ColorF(0x202020));
	gfx.pRenderTarget->FillRoundedRectangle(rrect, gfx.pSolidBrush);

	if (type == TYPE_HORIZONTAL)
	{
		rrect.rect.left = GetPos().x + fSliderValue * GetSize().x * (1.0f - fSliderSize);
		rrect.rect.right = rrect.rect.left + GetSize().x * fSliderSize;
	}
	else
	{
		rrect.rect.top = GetPos().y + fSliderValue * GetSize().y * (1.0f - fSliderSize);
		rrect.rect.bottom = rrect.rect.top + GetSize().y * fSliderSize;
	}
	gfx.pSolidBrush->SetColor(D2D1::ColorF(0x606060));
	gfx.pRenderTarget->FillRoundedRectangle(rrect, gfx.pSolidBrush);
}
guipp::Object* guipp::ScrollBar::OnMouseHitTest(const ext::vec2d<float>& mpos_t)
{
	if (
		mpos_t.x >= GetPos().x &&
		mpos_t.y >= GetPos().y &&
		mpos_t.x < GetPos().x + GetSize().x &&
		mpos_t.y < GetPos().y + GetSize().y
		)
	{
		return this;
	}
	return nullptr;
}
bool guipp::ScrollBar::OnMouseMessage(Window& wnd, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t)
{
	switch (msg)
	{
	case WM_MOUSEMOVE:
		if (wParam & MK_LBUTTON)
		{
			auto offset = mpos_t - last_mpos;
			fSliderValue = fOldSliderValue +
				(type == TYPE_HORIZONTAL ?
				offset.x / (GetSize().x * (1.0f - fSliderSize)) :
				offset.y / (GetSize().y * (1.0f - fSliderSize)));
			fSliderValue = std::max(0.0f, std::min(1.0f, fSliderValue));
			wnd.RequestRedraw();
		}
		else
		{
			return false;
		}
		break;
	case WM_LBUTTONDOWN:
		last_mpos = mpos_t;
		fOldSliderValue = fSliderValue;
		break;
	default:
		return false;
		break;
	}
	return true;
}
ext::vec2d<float> guipp::ScrollBar::OnMinSizeUpdate()
{
	return type == TYPE_HORIZONTAL ?
		ext::vec2d<float>{ 0.0f,20.0f } :
		ext::vec2d<float>{ 20.0f,0.0f };
}