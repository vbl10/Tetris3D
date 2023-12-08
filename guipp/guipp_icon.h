#pragma once
#include "guipp.h"

namespace guipp
{
	class Icon : public Object
	{
	public:
		Icon(CComPtr<ID2D1Bitmap> pBitmap, ext::vec2d<float> min_size) 
			:pBitmap(pBitmap), min_size(min_size) {}
		CComPtr<ID2D1Bitmap> pBitmap;
		ext::vec2d<float> min_size;
		void OnDraw(ext::D2DGraphics& gfx) override;
	private:
		void OnSetSize() override;
		void OnSetPos() override;
		ext::vec2d<float> OnMinSizeUpdate() override { return min_size; }
		D2D1_RECT_F rc_frame = { 0 };
	};
};