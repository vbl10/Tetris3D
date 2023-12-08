#pragma once
#include "guipp.h"

namespace guipp
{
	class Sizer : public Object
	{
	public:
		struct Scalar
		{
			static Scalar Make(ext::vec2d<float> m, ext::vec2d<float> c) { return { m,c }; }
			//as in f(x) = mx + c
			ext::vec2d<float> m = { 0.0f,0.0f };
			//as in f(x) = mx + c
			ext::vec2d<float> c = { 0.0f,0.0f };
		};
		Sizer(
			Object*           obj            = nullptr,
			Scalar            origin         = { { 0.5f,0.5f },{ 0.0f,0.0f } },
			Scalar            size           = { { 0.0f,0.0f },{ 0.0f,0.0f } },
			bool              bConstAspRatio = false,
			ext::vec2d<float> _min_size      = { 0.0f,0.0f })
			:
			obj(obj),
			origin(origin),
			size(size),
			bConstAspRatio(bConstAspRatio),
			_min_size(_min_size)
		{}

		Object* obj;
		Scalar origin, size;
		bool bConstAspRatio;
		ext::vec2d<float> _min_size;

	private:
		void OnDraw(ext::D2DGraphics& gfx) override;

		Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) override { return obj->OnMouseHitTest(mpos_t); }

		Object* OnKbdFocus(Window& wnd, bool bFirst) override { return obj->OnKbdFocus(wnd, bFirst); }
		Object* OnKbdNext(Window& wnd, const Object* child, bool bNext) override { return obj->OnKbdNext(wnd, child, bNext); }

		void OnSetPos() override;
		void OnSetSize() override;
		ext::vec2d<float> OnMinSizeUpdate() override;
	};
};