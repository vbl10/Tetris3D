#pragma once
#include "guipp.h"
#include <ext_scalar.h>

namespace guipp
{
	class Switch : public Object
	{
	public:
		struct Page
		{
			Page(
				std::shared_ptr<Object> obj = nullptr,
				ext::Scalar             origin = { { 0.5f,0.5f },{ 0.0f,0.0f } },
				ext::Scalar             size = { { 1.0f,1.0f },{ 0.0f,0.0f } },
				bool                    bConstAspRatio = false,
				ext::vec2d<float>       min_size = { 0.0f,0.0f })
				:
				obj(obj),
				origin(origin),
				size(size),
				bConstAspRatio(bConstAspRatio),
				min_size(min_size)
			{}

			std::shared_ptr<Object> obj;
			ext::Scalar origin, size;
			bool bConstAspRatio;
			ext::vec2d<float> min_size;
			int previous = 0;
		};

		void Insert(int page_id, const Page& page);

		void Set(Window& wnd, int page_id);
		void Previous();

	private:
		void OnDraw(ext::D2DGraphics& gfx) override;

		void OnInitialize(Window& wnd, bool bInitialize) override;
		Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) override;
		Object* OnKbdFocus(Window& wnd, bool bFirst) override;

		void OnSetPos() override;
		void OnSetSize() override;
		ext::vec2d<float> OnMinSizeUpdate() override;

		std::unordered_map<int, Page> pages;
		int cur_page = 0;
		bool bActive = false;
	};
};