#include "guipp_label.h"

using namespace guipp;
using namespace ext;

D2D1::ColorF Label::color = D2D1::ColorF(0xFFFFFF);

Label::Label(TextFormat font, const std::wstring& text, const vec2d<float>& alignment)
	:text(text), font(font), layout(font(text)), alignment(alignment)
{
}
Label& Label::SetText(const std::wstring& text)
{
	this->text = text;
	layout = font(text);
	return *this;
}
Label& Label::SetFont(TextFormat font)
{
	this->font = font;
	return *this;
}
Label& Label::SetAlignment(const vec2d<float>& new_alignment)
{
	alignment = new_alignment;
	OnSetPos();
	return *this;
}
void Label::OnDraw(ext::D2DGraphics& gfx)
{
	/*gfx.pSolidBrush->SetColor(D2D1::ColorF(0xff0000));
	gfx->DrawRectangle(
		D2D1::RectF(GetPos().x, GetPos().y, GetPos().x + GetSize().x, GetPos().y + GetSize().y),
		gfx
	);*/
	gfx.pSolidBrush->SetColor(color);
	gfx.pRenderTarget->DrawTextLayout({ lpos.x,lpos.y }, layout, gfx.pSolidBrush);
}
void Label::OnSetPos()
{
	lpos =
		GetPos() +
		GetSize() * alignment - lsize *
		(alignment - vec2d<float>
	{
		layout->GetTextAlignment() ==
			DWRITE_TEXT_ALIGNMENT_CENTER ? 0.5f :
			layout->GetTextAlignment() ==
			DWRITE_TEXT_ALIGNMENT_TRAILING ? 1.0f : 0.0f,
			layout->GetParagraphAlignment() ==
			DWRITE_PARAGRAPH_ALIGNMENT_CENTER ? 0.5f :
			layout->GetParagraphAlignment() ==
			DWRITE_PARAGRAPH_ALIGNMENT_FAR ? 1.0f : 0.0f
	});
}
vec2d<float> Label::OnMinSizeUpdate()
{
	DWRITE_TEXT_METRICS metrics;
	layout->GetMetrics(&metrics);

	return lsize = { metrics.widthIncludingTrailingWhitespace, metrics.height };
}