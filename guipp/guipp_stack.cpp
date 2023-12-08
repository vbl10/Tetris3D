#include "guipp_stack.h"
#include <assert.h>

void guipp::Stack::Insert(int id, const Layer& layer, bool bShow)
{
	assert(!layers.contains(id));
	layer.obj->SetParent(this);
	layers.insert({ id, layer });
	if (bShow)
		active_layers.push_front(id);
}
guipp::Stack::Layer& guipp::Stack::At(int id)
{
	assert(layers.contains(id));
	return layers.at(id);
}
void guipp::Stack::ShowLayer(Window& wnd, int id, bool bShow)
{
	assert(layers.contains(id));
	auto itFound = std::find(active_layers.begin(), active_layers.end(), id);
	bool bFound = itFound != active_layers.end();
	if (bShow != bFound)
	{
		for (int key : active_layers)
		{
			layers[key].obj->OnInitialize(wnd, false);
			if (!layers[key].bPermeable)
				break;
		}
		wnd.SetKbdTarget(nullptr);
		if (bShow)
		{
			if (bFound)
			{
				active_layers.erase(itFound);
			}
			active_layers.push_front(id);
			for (int key : active_layers)
			{
				layers[key].obj->OnInitialize(wnd, true);
				if (!layers[key].bPermeable)
					break;
			}
		}
		else if (bFound)
		{
			active_layers.erase(itFound);
			for (int key : active_layers)
			{
				layers[key].obj->OnInitialize(wnd, true);
				if (!layers[key].bPermeable)
					break;
			}
		}
		if (mouse_target)
		{
			mouse_target->OnMouseFocus(wnd, false);
			mouse_target = nullptr;
			mouse_depth = 0;
		}
		wnd.UpdateMouseTarget();
		wnd.RequestRedraw();
	}
}
void guipp::Stack::OnDraw(ext::D2DGraphics& gfx)
{
	int nNonPermeableMax = -1, i = 0;
	std::list<int>::reverse_iterator itId = active_layers.rbegin();
	for (itId++, i++; itId != active_layers.rend(); itId++)
	{
		if (!layers.at(*itId).bPermeable)
			nNonPermeableMax = i;
		i++;
	}
	itId = active_layers.rbegin();
	if (nNonPermeableMax != -1)
	{
		//apply blur
		FLOAT opacity = gfx.pSolidBrush->GetOpacity();
		gfx.pSolidBrush->SetOpacity(opacity * 0.5f);
		i = 0;
		for (; itId != active_layers.rend() && i != nNonPermeableMax; itId++, i++)
		{
			layers.at(*itId).obj->OnDraw(gfx);
		}
		gfx.pSolidBrush->SetOpacity(opacity);
	}
	for (; itId != active_layers.rend(); itId++)
	{
		layers.at(*itId).obj->OnDraw(gfx);
	}
}
void guipp::Stack::OnInitialize(Window& wnd, bool bInitialize)
{
	for (auto id : active_layers)
	{
		layers[id].obj->OnInitialize(wnd, bInitialize);
		if (!layers[id].bPermeable)
			break;
	}
}
void guipp::Stack::OnGfxCreated(ext::D2DGraphics& gfx)
{
	for (auto& [k, l] : layers)
		l.obj->OnGfxCreated(gfx);
}
guipp::Object* guipp::Stack::OnMouseHitTest(const ext::vec2d<float>& mpos_t)
{
	if (mpos_t.x >= GetPos().x &&
		mpos_t.y >= GetPos().y &&
		mpos_t.x < GetPos().x + GetSize().x &&
		mpos_t.y < GetPos().y + GetSize().y)
	{
		for (auto id : active_layers)
		{
			if (auto result = layers[id].obj->OnMouseHitTest(mpos_t); result)
			{
				mouse_target = result;
				break;
			}
			if (!layers[id].bPermeable)
			{
				break;
			}
		}
		return this;
	}
	return nullptr;
}
void guipp::Stack::OnMouseFocus(Window& wnd, bool bFocus)
{
	if (mouse_target)
		mouse_target->OnMouseFocus(wnd, bFocus);
}
bool guipp::Stack::OnMouseMessage(Window& wnd, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t)
{
	if (msg == WM_MOUSEMOVE)
	{
		//update mouse_target
		int depth = 0;
		for (const auto& id : active_layers)
		{
			//Interrogate the mouse_target directly if the depths match.
			//Return immediately if mouse_target responds to either
			//OnMouseMessage(WM_MOUSEMOVE) or OnMouseHitTest().
			if (depth == mouse_depth)
			{
				if (mouse_target)
				{
					if (mouse_target->OnMouseMessage(wnd, msg, wParam, mpos_t))
					{
						return true;
					}
					else if (auto new_target = mouse_target->OnMouseHitTest(mpos_t); new_target)
					{
						if (new_target != mouse_target)
						{
							mouse_target->OnMouseFocus(wnd, false);
							mouse_target = new_target;
							mouse_target->OnMouseFocus(wnd, true);
							return mouse_target->OnMouseMessage(wnd, msg, wParam, mpos_t);
						}
						return false;
					}
					else
					{
						mouse_target->OnMouseFocus(wnd, false);
						mouse_target = nullptr;
					}
				}
			}

			//If this layer responds to  OnMouseHitTest()
			//then a new mouse_target has been found and the
			//function can exit.
			if (auto new_target = layers[id].obj->OnMouseHitTest(mpos_t); new_target)
			{
				if (mouse_target) mouse_target->OnMouseFocus(wnd, false);
				mouse_target = new_target;
				mouse_target->OnMouseFocus(wnd, true);
				mouse_depth = depth;
				return mouse_target->OnMouseMessage(wnd, msg, wParam, mpos_t);
			}

			if (!layers[id].bPermeable)
				return false;

			depth++;
		}
		return false;
	}
	else if (mouse_target)
		return mouse_target->OnMouseMessage(wnd, msg, wParam, mpos_t);
	return false;
}
guipp::Object* guipp::Stack::OnKbdFocus(Window& wnd, bool bFirst)
{
	if (bFirst)
	{
		for (auto id : active_layers)
		{
			if (auto target = layers[id].obj->OnKbdFocus(wnd, true))
			{
				return target;
			}
			if (!layers[id].bPermeable)
			{
				break;
			}
		}
	}
	else
	{
		auto itLastNonPermeableId = active_layers.rbegin();
		for (auto itId = active_layers.rbegin(); itId != active_layers.rend(); itId++)
		{
			if (!layers[*itId].bPermeable)
			{
				itLastNonPermeableId = itId;
			}
		}
		for (; itLastNonPermeableId != active_layers.rend(); itLastNonPermeableId++)
		{
			if (auto target = layers[*itLastNonPermeableId].obj->OnKbdFocus(wnd, false); target)
			{
				return target;
			}
		}
	}
	return nullptr;
}
guipp::Object* guipp::Stack::OnKbdNext(Window& wnd, const Object* child, bool bNext)
{
	if (!active_layers.empty())
	{
		auto itChildId = std::find_if(layers.begin(), layers.end(),
			[&](std::pair<const int, Layer> pair) -> bool
			{
				return pair.second.obj.get() == child;
			}
		);

		if (bNext)
		{
			auto it = std::find(active_layers.begin(), active_layers.end(), itChildId->first);
			while (it != active_layers.end())
			{
				it++;
				if (it == active_layers.end() || !layers[*it].bPermeable)
					break;
				if (auto target = layers[*it].obj->OnKbdFocus(wnd, true); target)
					return target;
			}
		}
		else
		{
			auto end = std::find(active_layers.begin(), active_layers.end(), itChildId->first);
			if (end == active_layers.begin())
				return nullptr;
			auto it = active_layers.begin();
			for (; it != end && layers[*it].bPermeable; it++);
			if (it != end)
				it++;
			do
			{
				it--;
				if (auto target = layers[*it].obj->OnKbdFocus(wnd, false); target)
				{
					return target;
				}
			} while (it != active_layers.begin());
		}
	}
	return nullptr;
}
void guipp::Stack::OnSetPos()
{
	for (auto& [key, layer] : layers)
		layer.obj->SetPos(GetPos() + layer.origin.c + (GetSize() - layer.obj->GetSize()) * layer.origin.m);
}
void guipp::Stack::OnSetSize()
{
	for (auto& [key, layer] : layers)
		layer.obj->SetSize({
			(layer.size.m.x > 0.0f ? layer.size.m.x * GetSize().x - layer.size.c.x : std::max(layer.size.c.x, layer.obj->GetMinSize().x)),
			(layer.size.m.y > 0.0f ? layer.size.m.y * GetSize().y - layer.size.c.y : std::max(layer.size.c.y, layer.obj->GetMinSize().y))
			});
}
ext::vec2d<float> guipp::Stack::OnMinSizeUpdate()
{
	ext::vec2d<float> min = { 0.0f,0.0f };
	for (auto& [key, layer] : layers)
	{
		min.x = std::max(min.x, std::max(layer.min_size.x, layer.size.c.x + layer.obj->GetMinSize().x / (layer.size.m.x > 0.0f ? layer.size.m.x : 1.0f)));
		min.y = std::max(min.y, std::max(layer.min_size.y, layer.size.c.y + layer.obj->GetMinSize().y / (layer.size.m.y > 0.0f ? layer.size.m.y : 1.0f)));
	}
	return min;
}