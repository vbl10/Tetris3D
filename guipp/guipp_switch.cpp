#include "guipp_switch.h"
#include <assert.h>

using namespace guipp;

void Switch::Insert(int page_id, const Page& page)
{
	assert(!pages.contains(page_id));
	pages.insert({ page_id,page });
}
void Switch::Set(Window& wnd, int page_id)
{
	pages.at(page_id).previous = cur_page;
	if (bActive)
	{
		pages.at(cur_page).obj->OnInitialize(wnd, false);
		pages.at(page_id).obj->OnInitialize(wnd, true);
		cur_page = page_id;
		wnd.SetKbdTarget(nullptr);
		wnd.UpdateMouseTarget();
	}
	cur_page = page_id;
}
void Switch::Previous()
{
	cur_page = pages.at(cur_page).previous;
}

void Switch::OnDraw(ext::D2DGraphics& gfx)
{
	pages.at(cur_page).obj->OnDraw(gfx);
}

void guipp::Switch::OnInitialize(Window& wnd, bool bInitialize)
{
	bActive = bInitialize;
	pages.at(cur_page).obj->OnInitialize(wnd, bInitialize);
	wnd.SetKbdTarget(nullptr);
	wnd.UpdateMouseTarget();
}
Object* guipp::Switch::OnMouseHitTest(const ext::vec2d<float>& mpos_t)
{
	return pages.at(cur_page).obj->OnMouseHitTest(mpos_t);
}
Object* guipp::Switch::OnKbdFocus(Window& wnd, bool bFirst)
{
	return pages.at(cur_page).obj->OnKbdFocus(wnd, bFirst);
}

void Switch::OnSetPos()
{
	for (auto& [key, page] : pages)
		page.obj->SetPos(GetPos() + page.origin.c + (GetSize() - page.obj->GetSize()) * page.origin.m);
}
void Switch::OnSetSize()
{
	for (auto& [key, page] : pages)
		page.obj->SetSize({
			(page.size.m.x > 0.0f ? page.size.m.x * GetSize().x - page.size.c.x : page.obj->GetMinSize().x),
			(page.size.m.y > 0.0f ? page.size.m.y * GetSize().y - page.size.c.y : page.obj->GetMinSize().y)
			});
}
ext::vec2d<float> Switch::OnMinSizeUpdate()
{
	ext::vec2d<float> min = { 0.0f,0.0f };
	for (auto& [key, page] : pages)
	{
		min.x = std::max(min.x, std::max(page.min_size.x, page.size.c.x + page.obj->GetMinSize().x / (page.size.m.x > 0.0f ? page.size.m.x : 1.0f)));
		min.y = std::max(min.y, std::max(page.min_size.y, page.size.c.y + page.obj->GetMinSize().y / (page.size.m.y > 0.0f ? page.size.m.y : 1.0f)));
	}
	return min;
}