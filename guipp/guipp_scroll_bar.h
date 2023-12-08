#pragma once
#include "guipp.h"

namespace guipp
{
	class ScrollBar : public Object
	{
	public:
		enum TYPE { TYPE_VERTICAL,TYPE_HORIZONTAL };
		ScrollBar(TYPE type, float fSliderValue = 0.0f, float fSliderSize = 1.0f);

		const TYPE type;
		float fSliderValue = 0.0f;
		float fSliderSize = 1.0f;
	private:
		void OnDraw(ext::D2DGraphics& gfx) override;
		Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) override;
		bool OnMouseMessage(Window& wnd, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t) override;
		ext::vec2d<float> OnMinSizeUpdate() override;

		ext::vec2d<float> last_mpos;
		float fOldSliderValue = 0.0f;
	};
};