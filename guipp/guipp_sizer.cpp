#include "guipp_sizer.h"

void guipp::Sizer::OnDraw(ext::D2DGraphics& gfx) 
{
	//gfx.pSolidBrush->SetColor(D2D1::ColorF(0xff0000));
	//gfx->DrawRectangle(
	//	D2D1::RectF(GetPos().x, GetPos().y, GetPos().x + GetSize().x, GetPos().y + GetSize().y),
	//	gfx);

	obj->OnDraw(gfx);
}

void guipp::Sizer::OnSetPos()
{
	obj->SetPos(GetPos() + origin.c + (GetSize() - obj->GetSize()) * origin.m);
}
void guipp::Sizer::OnSetSize()
{
	obj->SetSize({
		(size.m.x > 0.0f ? size.m.x * GetSize().x - size.c.x : obj->GetMinSize().x),
		(size.m.y > 0.0f ? size.m.y * GetSize().y - size.c.y : obj->GetMinSize().y)
		});
}
ext::vec2d<float> guipp::Sizer::OnMinSizeUpdate()
{
	return {
		std::max(_min_size.x, size.c.x + obj->GetMinSize().x / (size.m.x > 0.0f ? size.m.x : 1.0f)),
		std::max(_min_size.y, size.c.y + obj->GetMinSize().y / (size.m.y > 0.0f ? size.m.y : 1.0f))
	};
}
