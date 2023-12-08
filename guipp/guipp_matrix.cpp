#include "guipp_matrix.h"
#include <assert.h>
#include <algorithm>

D2D1::ColorF guipp::Matrix::color_fill = 0x303030, guipp::Matrix::color_outline = 0x707070;

guipp::Matrix::Matrix(vec items, ext::vec2d<unsigned> layout, char style)
	:items(items), style(style)
{
	if (!items.empty())
	{
		if (layout.x == 0 && layout.y == 0)
		{
			layout.x = 1;
			layout.y = items.size();
		}
		else if (layout.x == 0)
		{
			layout.x = (items.size() + layout.y - 1) / layout.y;
		}
		else if (layout.y == 0)
		{
			layout.y = (items.size() + layout.x - 1) / layout.x;
		}
	}
	this->layout = layout;
	assert(layout.x * layout.y == items.size());

	cols.resize(layout.x);
	rows.resize(layout.y);

	for (auto item : items)
		item->SetParent(this);
}
guipp::Matrix::Axis& guipp::Matrix::SetCol(int col)
{
	assert(col >= 0 && col < GetLayout().x);
	return cols[col];
}
guipp::Matrix::Axis& guipp::Matrix::SetRow(int row)
{
	assert(row >= 0 && row < GetLayout().y);
	return rows[row];
}
void guipp::Matrix::SetMain(ext::vec2d<int> main_cell)
{
	if (main_cell != -1)
		assert(
			main_cell.x >= 0 && 
			main_cell.x < layout.x && 
			main_cell.y >= 0 && 
			main_cell.y < layout.y);

	this->main_cell = main_cell;
}

std::shared_ptr<guipp::Object> guipp::Matrix::At(ext::vec2d<unsigned> coord)
{
	return items[ToIndex(coord)];
}

void guipp::Matrix::InsertCol(unsigned before, std::vector<std::shared_ptr<Object>> new_items, const Axis& properties)
{
	if (layout.y = 0)
	{
		layout.y = new_items.size();
		rows.resize(layout.y);
	}
	assert(layout.y == new_items.size());
	for (int i = 0; i < layout.y; i++)
	{
		new_items[i]->SetParent(this);
		items.insert(items.begin() + before + i * (layout.x + 1), new_items[i]);
	}
	cols.insert(cols.begin() + before, properties);
}
void guipp::Matrix::InsertRow(unsigned before, std::vector<std::shared_ptr<Object>> new_items, const Axis& properties)
{
	if (layout.x == 0)
	{
		layout.x = new_items.size();
		cols.resize(layout.x);
	}
	assert(layout.x == new_items.size());
	for (auto item : new_items)
		item->SetParent(this);
	items.insert(items.begin() + before * layout.x, new_items.begin(), new_items.end());
	rows.insert(rows.begin() + before, properties);
	layout.y++;
}
std::pair<guipp::Matrix::vec, guipp::Matrix::Axis> guipp::Matrix::RemoveCol(unsigned col)
{
	std::pair<vec, Axis> result;
	for (int i = 0; i < layout.y; i++)
	{
		result.first.push_back(items[col + i * (layout.x - 1)]);
		items.erase(items.begin() + col + i * (layout.x - 1));
	}
	result.second = cols[col];
	cols.erase(cols.begin() + col);
	layout.x--;
	return result;
}
std::pair<guipp::Matrix::vec, guipp::Matrix::Axis> guipp::Matrix::RemoveRow(unsigned row)
{
	std::pair<vec, Axis> result;
	auto first = items.begin() + row * layout.x;
	auto last = items.begin() + (row + 1) * layout.x;
	result.first.insert(result.first.begin(), first, last);
	items.erase(first, last);
	result.second = rows[row];
	rows.erase(rows.begin() + row);
	layout.y--;
	return result;
}

void guipp::Matrix::OnDraw(ext::D2DGraphics& gfx)
{
	if (style & STYLE_BACKGND)
	{
		gfx.pSolidBrush->SetColor(color_fill);
		gfx.pRenderTarget->FillRoundedRectangle(
			D2D1::RoundedRect(
				D2D1::RectF(GetPos().x, GetPos().y, GetPos().x + GetSize().x, GetPos().y + GetSize().y),
				fCornerRadius, fCornerRadius
			), gfx.pSolidBrush.p
		);
	}
	if (style & STYLE_OUTLINE)
	{
		gfx.pSolidBrush->SetColor(color_outline);
		gfx.pRenderTarget->DrawRoundedRectangle(
			D2D1::RoundedRect(
				D2D1::RectF(GetPos().x, GetPos().y, GetPos().x + GetSize().x, GetPos().y + GetSize().y),
				fCornerRadius, fCornerRadius
			), gfx.pSolidBrush.p, 2.0f
		);
	}
	for (auto item : items)
		item->OnDraw(gfx);
	
	//separators
	gfx.pSolidBrush->SetColor(color_outline);
	for (unsigned y = 1; y < layout.y; y++)
	{
		if (rows[y].separator)
		{
			float py = items[ToIndex({ 0,y })]->GetPos().y - fSpacing;
			gfx.pRenderTarget->DrawLine(
				D2D1::Point2F(GetPos().x + guipp::fSpacing * bool(style & STYLE_THICKFRAME), py),
				D2D1::Point2F(GetPos().x + GetSize().x - guipp::fSpacing * bool(style & STYLE_THICKFRAME), py),
				gfx.pSolidBrush, 2.0f);
		}
	}
	for (unsigned x = 1; x < layout.x; x++)
	{
		if (cols[x].separator)
		{
			float px = items[ToIndex({ x,0 })]->GetPos().x - fSpacing;
			gfx.pRenderTarget->DrawLine(
				D2D1::Point2F(px, GetPos().y + guipp::fSpacing * bool(style & STYLE_THICKFRAME)),
				D2D1::Point2F(px, GetPos().y + GetSize().y - guipp::fSpacing * bool(style & STYLE_THICKFRAME)),
				gfx.pSolidBrush, 2.0f);
		}
	}
}

void guipp::Matrix::OnInitialize(Window& app, bool bInitialize)
{
	for (auto item : items)
		item->OnInitialize(app, bInitialize);
}
void guipp::Matrix::OnGfxCreated(ext::D2DGraphics& gfx)
{
	for (auto item : items)
		item->OnGfxCreated(gfx);
}

guipp::Object* guipp::Matrix::OnMouseHitTest(const ext::vec2d<float>& mpos_t)
{
	if (!items.empty()
		&& mpos_t.x >= GetPos().x && mpos_t.x < GetPos().x + GetSize().x
		&& mpos_t.y >= GetPos().y && mpos_t.y < GetPos().y + GetSize().y)
	{
		auto layout = GetLayout();
		unsigned x1 = 0;
		for (unsigned x2 = layout.x; x1 != x2 - 1;)
		{
			if (mpos_t.x < items[ToIndex({ (x1 + x2) / 2,0 })]->GetPos().x - fSpacing)
			{
				x2 = (x1 + x2) / 2;
			}
			else if (mpos_t.x >= items[ToIndex({ (x1 + x2) / 2,0 })]->GetPos().x)
			{
				x1 = (x1 + x2) / 2;
			}
			else
			{
				x1 = -1;
				break;
			}
		}
		unsigned y1 = 0;
		if (x1 != -1)
		{
			for (unsigned y2 = layout.y; y1 != y2 - 1;)
			{
				if (mpos_t.y < items[ToIndex({ 0,(y1 + y2) / 2 })]->GetPos().y - fSpacing)
				{
					y2 = (y1 + y2) / 2;
				}
				else if (mpos_t.y >= items[ToIndex({ 0,(y1 + y2) / 2 })]->GetPos().y)
				{
					y1 = (y1 + y2) / 2;
				}
				else
				{
					y1 = -1;
					break;
				}
			}
		}
		unsigned i = x1 != -1 && y1 != -1 ? ToIndex({ x1,y1 }) : -1;
		if (i != -1)
			return items[i]->OnMouseHitTest(mpos_t);
	}
	return nullptr;
}

guipp::Object* guipp::Matrix::OnKbdFocus(Window& app, bool bFirst)
{
	if (bFirst) {
		for (auto item : items) {
			if (auto result = item->OnKbdFocus(app, bFirst); result) {
				return result;
			}
		}
	}
	else {
		for (auto it = items.rbegin(); it != items.rend(); it++) {
			if (auto result = (*it)->OnKbdFocus(app, bFirst); result) {
				return result;
			}
		}
	}
	return nullptr;
}
guipp::Object* guipp::Matrix::OnKbdNext(Window& app, const Object* child, bool bNext)
{
	auto match = [&child](std::shared_ptr<Object> obj) -> bool
	{
		return obj.get() == child;
	};
	for (auto it = std::find_if(items.begin(), items.end(), match);
		it >= items.begin() + !bNext 
		&& it <= items.end() - 1 - bNext;
		it += bNext * 2 - 1)
		if (auto next = (*(it + bNext * 2 - 1))->OnKbdFocus(app, bNext); next)
			return next;

	return nullptr;
}

void guipp::Matrix::OnSetPos()
{
	if (!items.empty())
	{
		auto pos = GetPos() + fSpacing * bool(style & STYLE_THICKFRAME);
		const float top = pos.y;
		for (unsigned x = 0; x < layout.x; x++)
		{
			for (unsigned y = 0; y < layout.y; y++)
			{
				int i = ToIndex({ x,y });
				items[i]->SetPos(pos);
				pos.y += items[i]->GetSize().y + fSpacing;
			}
			pos.y = top;
			pos.x += items[ToIndex({ x,0 })]->GetSize().x + fSpacing;
		}
	}
}
void guipp::Matrix::OnSetSize()
{
	if (!items.empty())
	{
		auto new_size = GetSize() - fSpacing * (layout + (style & STYLE_THICKFRAME ? 1 : -1));

		//remove extra spacing destined to spacers
		for (int x = 1; x < layout.x; x++)
			if (cols[x].separator)
				new_size.x -= fSpacing;

		for (int y = 1; y < layout.y; y++)
			if (rows[y].separator)
				new_size.y -= fSpacing;

		std::vector<float> widths(layout.x);
		std::vector<float> heights(layout.y);

		if (main_cell.x != -1)
		{
			auto mc_min_size = items[ToIndex(main_cell.to<unsigned>())]->GetMinSize();
			auto avb_size = GetSize() - (GetMinSize() - mc_min_size);
			auto scale = avb_size / mc_min_size;
			new_size.x -= (widths[main_cell.x] = mc_min_size.x * std::min(scale.x, scale.y));
			new_size.y -= (heights[main_cell.y] = mc_min_size.y * std::min(scale.x, scale.y));
		}

		//widths
		{
			std::vector<int> dec_min; //decrescent order for vec_min
			int prop_sum = 0;
			float extra_space = new_size.x;
			float avb_space = new_size.x;
			for (unsigned x = 0; x < layout.x; x++)
			{
				if (main_cell.x != x)
				{
					prop_sum += cols[x].proportion;
					dec_min.push_back(x);
					float min = 0.0f;
					for (unsigned y = 0; y < layout.y; y++)
						min = std::max(min, items[ToIndex({ x,y })]->GetMinSize().x);
					extra_space -= widths[x] = min;
					if (cols[x].proportion == 0)
						avb_space -= min;
				}
			}

			std::sort(dec_min.begin(), dec_min.end(),
				[&](int a, int b) -> bool {
					return widths[a] > widths[b];
				});

			for (int i = 0; i < dec_min.size(); i++)
			{
				if (cols[dec_min[i]].proportion > 0)
				{
					float min = widths[dec_min[i]];
					widths[dec_min[i]] =
						std::min(
							extra_space + widths[dec_min[i]],
							std::max(min, avb_space * (float)cols[dec_min[i]].proportion / (float)prop_sum)
						);
					extra_space -= widths[dec_min[i]] - min;
					avb_space -= widths[dec_min[i]];
					prop_sum -= cols[dec_min[i]].proportion;
				}
			}
		}
		//heights
		{
			std::vector<int> dec_min; //decrescent order for vec_min
			int prop_sum = 0;
			float extra_space = new_size.y;
			float avb_space = new_size.y;
			for (unsigned y = 0; y < layout.y; y++)
			{
				if (main_cell.y != y)
				{
					prop_sum += rows[y].proportion;
					dec_min.push_back(y);
					float min = 0.0f;
					for (unsigned x = 0; x < layout.x; x++)
						min = std::max(min, items[ToIndex({ x,y })]->GetMinSize().y);
					extra_space -= heights[y] = min;
					if (rows[y].proportion == 0)
						avb_space -= min;
				}
			}

			std::sort(dec_min.begin(), dec_min.end(),
				[&](int a, int b) -> bool {
					return heights[a] > heights[b];
				});

			for (int i = 0; i < dec_min.size(); i++)
			{
				if (rows[dec_min[i]].proportion > 0)
				{
					const float min = heights[dec_min[i]];
					heights[dec_min[i]] =
						std::min(
							extra_space + heights[dec_min[i]],
							std::max(min, avb_space * (float)rows[dec_min[i]].proportion / (float)prop_sum)
						);
					extra_space -= heights[dec_min[i]] - min;
					avb_space -= heights[dec_min[i]];
					prop_sum -= rows[dec_min[i]].proportion;
				}
			}
		}

		for (unsigned i = 0; i < items.size(); i++)
		{
			auto coord = ToCoord(i);
			items[i]->SetSize({ widths[coord.x],heights[coord.y] });
		}
	}
}
ext::vec2d<float> guipp::Matrix::OnMinSizeUpdate()
{
	ext::vec2d<float> min = { 0.0f,0.0f };
	if (!items.empty())
	{
		if (style & STYLE_MIN_PROP)
		{
			ext::vec2d<int> prop_sum = { 0,0 };
			for (int x = 0; x < layout.x; x++)
				prop_sum.x += cols[x].proportion;
			for (int y = 0; y < layout.y; y++)
				prop_sum.y += rows[y].proportion;

			for (unsigned x = 0; x < layout.x; x++)
			{
				float col_min = 0.0f;
				for (unsigned y = 0; y < layout.y; y++)
				{
					col_min = std::max(col_min, items[ToIndex({ x,y })]->GetMinSize().x);
				}
				min.x += col_min;
			}
			for (unsigned x = 0; x < layout.x; x++)
			{
				if (cols[x].proportion > 0)
				{
					float col_min = 0.0f;
					for (unsigned y = 0; y < layout.y; y++)
					{
						col_min = std::max(col_min, items[ToIndex({ x,y })]->GetMinSize().x);
					}
					min.x = std::max(min.x, col_min * (float)prop_sum.x / (float)cols[x].proportion);
				}
			}

			for (unsigned y = 0; y < layout.y; y++)
			{
				float row_min = 0.0f;
				for (unsigned x = 0; x < layout.x; x++)
				{
					row_min = std::max(row_min, items[ToIndex({ x,y })]->GetMinSize().y);
				}
				min.y += row_min;
			}
			for (unsigned y = 0; y < layout.y; y++)
			{
				if (rows[y].proportion > 0)
				{
					float row_min = 0.0f;
					for (unsigned x = 0; x < layout.x; x++)
					{
						row_min = std::max(row_min, items[ToIndex({ x,y })]->GetMinSize().y);
					}
					min.y = std::max(min.y, row_min * (float)prop_sum.x / (float)rows[y].proportion);
				}
			}

			min += fSpacing * (layout + (style & STYLE_THICKFRAME ? 1 : -1));
		}
		else
		{
			for (unsigned x = 0; x < layout.x; x++)
			{
				float col_min = 0.0f;
				for (unsigned y = 0; y < layout.y; y++)
				{
					col_min = std::max(col_min, items[ToIndex({ x,y })]->GetMinSize().x);
				}
				min.x += col_min;
			}
			for (unsigned y = 0; y < layout.y; y++)
			{
				float row_min = 0.0f;
				for (unsigned x = 0; x < layout.x; x++)
				{
					row_min = std::max(row_min, items[ToIndex({ x,y })]->GetMinSize().y);
				}
				min.y += row_min;
			}

			min += fSpacing * (layout + (style & STYLE_THICKFRAME ? 1 : -1));
		}
	}
	return min;
}

const ext::vec2d<unsigned>& guipp::Matrix::GetLayout() const
{
	return layout;
}
ext::vec2d<unsigned> guipp::Matrix::ToCoord(unsigned i) const
{
	return { i % layout.x, i / layout.x };
}
unsigned guipp::Matrix::ToIndex(const ext::vec2d<unsigned>& coord) const
{
	return coord.x + coord.y * layout.x;
}
