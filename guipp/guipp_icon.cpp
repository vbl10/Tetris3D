#include "guipp_icon.h"
#include <assert.h>

using namespace guipp;

void Icon::OnDraw(ext::D2DGraphics& gfx)
{
	assert(pBitmap);
	gfx.pRenderTarget->DrawBitmap(pBitmap, rc_frame);
}

void Icon::OnSetSize() 
{
	auto scale = GetSize() / GetMinSize();
	float fScale = std::min(scale.x, scale.y);
	rc_frame.right = rc_frame.left + GetMinSize().x * fScale;
	rc_frame.bottom = rc_frame.top + GetMinSize().y * fScale;
}

void Icon::OnSetPos()
{
	ext::vec2d<float> rc_size = { rc_frame.right - rc_frame.left,rc_frame.bottom - rc_frame.top };
	auto top_left = GetPos() + (GetSize() - rc_size) * 0.5f;
	rc_frame.right += top_left.x - rc_frame.left;
	rc_frame.left = top_left.x;
	rc_frame.bottom += top_left.y - rc_frame.top;
	rc_frame.top = top_left.y;
}