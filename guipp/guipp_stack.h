#pragma once
#include "guipp.h"
#include <ext_scalar.h>
#include <unordered_map>
#include <list>

namespace guipp
{
	class Stack : public Object
	{
	public:
		struct Layer
		{
			Layer(
				std::shared_ptr<Object> obj      = nullptr,
				bool                    bPermeable     = true,
				ext::Scalar             origin         = { { 0.5f,0.5f },{ 0.0f,0.0f } },
				ext::Scalar             size           = { { 1.0f,1.0f },{ 0.0f,0.0f } },
				bool                    bConstAspRatio = false,
				ext::vec2d<float>       min_size       = { 0.0f,0.0f })
				:
				obj(obj),
				bPermeable(bPermeable),
				origin(origin),
				size(size),
				bConstAspRatio(bConstAspRatio),
				min_size(min_size)
			{}

			std::shared_ptr<Object> obj;
			bool bPermeable;
			ext::Scalar origin, size;
			bool bConstAspRatio;
			ext::vec2d<float> min_size;
		};

		void Insert(int id, const Layer& layer, bool bShow = true);
		Layer& At(int id);
		void ShowLayer(Window& wnd, int id, bool bShow);

	private:
		void OnDraw(ext::D2DGraphics& gfx) override;
		void OnInitialize(Window& wnd, bool bInitialize) override;
		void OnGfxCreated(ext::D2DGraphics& gfx) override;
		Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) override;
		void OnMouseFocus(Window& wnd, bool bFocus) override;
		bool OnMouseMessage(Window& wnd, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t) override;

		Object* OnKbdFocus(Window& wnd, bool bFirst) override;
		Object* OnKbdNext(Window& wnd, const Object* child, bool bNext) override;

		void OnSetPos() override;
		void OnSetSize() override;
		ext::vec2d<float> OnMinSizeUpdate() override;

		std::unordered_map<int, Layer> layers;
		std::list<int> active_layers;

		guipp::Object* mouse_target = nullptr;
		int mouse_depth = 0;
	};
};