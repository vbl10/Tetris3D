#pragma once
#include "guipp.h"

namespace guipp
{
	class Label : public virtual Object
	{
	public:
		static D2D1::ColorF color;
		Label(ext::TextFormat font, const std::wstring& text, const ext::vec2d<float>& alignment = { 0.5f,0.5f });

		Label& SetText(const std::wstring& text);
		const std::wstring& GetText() const { return text; }
		Label& SetFont(ext::TextFormat font);
		Label& SetAlignment(const ext::vec2d<float>& new_alignment);

		void OnDraw(ext::D2DGraphics& gfx) override;
	private:
		void OnSetPos() override;
		ext::vec2d<float> OnMinSizeUpdate() override;

		std::wstring text;
		ext::TextFormat font;
		CComPtr<IDWriteTextLayout> layout;
		ext::vec2d<float> lsize, lpos, alignment;
	};
}