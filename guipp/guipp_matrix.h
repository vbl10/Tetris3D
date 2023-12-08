#pragma once
#include "guipp.h"

namespace guipp
{
	class Matrix : public Object
	{
	public:
		using vec = std::vector<std::shared_ptr<Object>>;
		static D2D1::ColorF color_fill, color_outline;
		enum STYLE { 
			STYLE_THICKFRAME = 0b1, 
			STYLE_BACKGND    = 0b10, 
			STYLE_OUTLINE    = 0b100,
			//minimum size respects proportions
			STYLE_MIN_PROP   = 0b1000
		};
		// combine Matrix::STYLE's to set style parameter
		Matrix(vec items = {}, ext::vec2d<unsigned> layout = { 0,0 }, char style = 0);
		Matrix(const Matrix&) = delete;

		struct Axis
		{
			int proportion = 1;
			bool separator = false;
			float max = std::numeric_limits<float>::max();
		};
		Axis& SetCol(int col);
		Axis& SetRow(int row);
		//keeps the cell at constant aspect ratio with full priority on resizing
		void SetMain(ext::vec2d<int> main_cell = { -1,-1 });

		std::shared_ptr<Object> At(ext::vec2d<unsigned> coord);
		void InsertCol(unsigned before, std::vector<std::shared_ptr<Object>> new_items, const Axis& properties = Axis());
		void InsertRow(unsigned before, std::vector<std::shared_ptr<Object>> new_items, const Axis& properties = Axis());
		std::pair<vec, Axis> RemoveCol(unsigned col);
		std::pair<vec, Axis> RemoveRow(unsigned row);

		void OnDraw(ext::D2DGraphics& gfx);
	private:
		void OnInitialize(Window& app, bool bInitialize) override;
		void OnGfxCreated(ext::D2DGraphics& gfx) override;

		Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) override;

		Object* OnKbdFocus(Window& app, bool bFirst) override;
		Object* OnKbdNext(Window& app, const Object* child, bool bNext) override;

		void OnSetPos() override;
		void OnSetSize() override;
		ext::vec2d<float> OnMinSizeUpdate() override;
	public:
		const ext::vec2d<unsigned>& GetLayout() const;
	private:
		ext::vec2d<unsigned> ToCoord(unsigned i) const;
		unsigned ToIndex(const ext::vec2d<unsigned>& coord) const;

		vec items;
		std::vector<Axis> cols, rows;
		ext::vec2d<unsigned> layout;
		ext::vec2d<int> main_cell = { -1,-1 };
		char style;
	};
};