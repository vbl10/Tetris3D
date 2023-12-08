#include "guipp_button.h"

guipp::ButtonBase::ButtonBase(std::function<void(int)> func, int id)
	:func(func), id(id)
{
}
guipp::Object* guipp::ButtonBase::OnMouseHitTest(const ext::vec2d<float>& mpos_t)
{
	return 
		mpos_t.x >= GetPos().x && mpos_t.x < GetPos().x + GetSize().x &&
		mpos_t.y >= GetPos().y && mpos_t.y < GetPos().y + GetSize().y ?
		this : nullptr;
}
void guipp::ButtonBase::OnMouseFocus(Window& app, bool bFocus)
{
	bHeld &= bHover = bFocus;
	app.RequestRedraw();
}
bool guipp::ButtonBase::OnMouseMessage(Window& app, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		bHeld = true;
		app.RequestRedraw();
		break;
	case WM_LBUTTONUP:
		if (bHeld)
		{
			bHeld = false;
			app.RequestRedraw();
			func(id);
		}
		break;
	default:
		return false;
		break;
	}
	return true;
}
guipp::Object* guipp::ButtonBase::OnKbdFocus(Window& app, bool)
{
	bKbdFocus = true;
	app.RequestRedraw();
	return this;
}
void guipp::ButtonBase::OnKbdUnfocus(Window& app)
{
	bKbdFocus = false;
	bSBHeld = false;
	app.RequestRedraw();
}
bool guipp::ButtonBase::OnKbdMessage(Window& app, UINT msg, unsigned key_code, LPARAM lParam)
{
	switch (key_code)
	{
	case ' ':
		__fallthrough;
	case '\r':
		if (msg == WM_KEYDOWN)
		{
			if (bSBHeld)
				return true;
			bSBHeld = true;
		}
		else if (msg == WM_KEYUP && bSBHeld)
		{
			bSBHeld = false;
			func(id);
		}
		break;
	default:
		return false;
		break;
	}
	app.RequestRedraw();
	return true;
}

D2D1::ColorF
	guipp::Button::color_fill_idle     = D2D1::ColorF(0x404040), 
	guipp::Button::color_fill_held     = D2D1::ColorF(0x606060),
	guipp::Button::color_outline_idle  = D2D1::ColorF(0x909090), 
	guipp::Button::color_outline_hover = D2D1::ColorF(0xf0f0f0);
guipp::Button::Button(std::shared_ptr<Label> label, std::function<void(int)> func, int id)
	:ButtonBase(func,id), label(label)
{
	label->SetParent(this);
}
guipp::Button::Button(std::shared_ptr<Icon> icon, std::function<void(int)> func, int id)
	: ButtonBase(func, id), icon(icon)
{
	icon->SetParent(this);
}
guipp::Button::Button(std::shared_ptr<Icon> icon, std::shared_ptr<Label> label, std::function<void(int)> func, int id)
	: ButtonBase(func, id), icon(icon), label(label)
{
	matrix = std::make_unique<Matrix>(Matrix::vec{ icon,label }, ext::vec2d<unsigned>{2, 1});
	matrix->SetParent(this);
	matrix->SetMain({ 0,0 });
}
void guipp::Button::OnDraw(ext::D2DGraphics& gfx)
{
	auto rrect = D2D1::RoundedRect(
		D2D1::RectF(
			GetPos().x,
			GetPos().y,
			GetPos().x + GetSize().x,
			GetPos().y + GetSize().y),
		fCornerRadius, fCornerRadius);
	
	gfx.pSolidBrush->SetColor(bHeld || bSBHeld ? color_fill_held : color_fill_idle);
	gfx.pRenderTarget->FillRoundedRectangle(rrect, gfx.pSolidBrush);

	gfx.pSolidBrush->SetColor(bHover || bKbdFocus ? color_outline_hover : color_outline_idle);
	gfx.pRenderTarget->DrawRoundedRectangle(rrect, gfx.pSolidBrush, fStrokeWidth);

	if (matrix)
		matrix->OnDraw(gfx);
	else if (label)
		label->OnDraw(gfx);
	else
		icon->OnDraw(gfx);
}
void guipp::Button::OnSetPos()
{
	if (matrix)
		matrix->SetPos(GetPos() + fSpacing);
	else if (label)
		label->SetPos(GetPos() + fSpacing);
	else
		icon->SetPos(GetPos() + fSpacing);
}
void guipp::Button::OnSetSize()
{
	if (matrix)
		matrix->SetSize(GetSize() - 2.0f * fSpacing);
	else if (label)
		label->SetSize(GetSize() - 2.0f * fSpacing);
	else
		icon->SetSize(GetSize() - 2.0f * fSpacing);
}
ext::vec2d<float> guipp::Button::OnMinSizeUpdate()
{
	if (matrix)
		return matrix->GetMinSize() + 2.0f * fSpacing;
	else if (label)
		return label->GetMinSize() + 2.0f * fSpacing;
	else
		return icon->GetMinSize() + 2.0f * fSpacing;
}