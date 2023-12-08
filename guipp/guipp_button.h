#pragma once
#include "guipp.h"
#include "guipp_label.h"
#include "guipp_icon.h"
#include "guipp_matrix.h"
#include <functional>

namespace guipp
{
	class ButtonBase : public Object
	{
	protected:
		ButtonBase(std::function<void(int)> func, int id = 0);
		Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) override;
		void OnMouseFocus(Window& app, bool bFocus) override;
		bool OnMouseMessage(Window& app, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t) override;
		Object* OnKbdFocus(Window& app, bool) override;
		void OnKbdUnfocus(Window& app) override;
		bool OnKbdMessage(Window& app, UINT msg, unsigned key_code, LPARAM lParam) override;
		bool bHover = false, bHeld = false;
		bool bKbdFocus = false, bSBHeld = false;
	public:
		std::function<void(int)> func;
		int id;
	};
	class Button : public ButtonBase
	{
	public:
		static D2D1::ColorF
			color_fill_idle, color_fill_held,
			color_outline_idle, color_outline_hover;

		Button(std::shared_ptr<Label> label, std::function<void(int)> func, int id = 0);
		Button(std::shared_ptr<Icon> icon, std::function<void(int)> func, int id = 0);
		Button(std::shared_ptr<Icon> icon, std::shared_ptr<Label> label, std::function<void(int)> func, int id = 0);

		std::shared_ptr<Label> label;
		std::shared_ptr<Icon> icon;
	private:
		std::unique_ptr<Matrix> matrix;
		void OnDraw(ext::D2DGraphics& gfx) override;
		void OnSetPos() override;
		void OnSetSize() override;
		ext::vec2d<float> OnMinSizeUpdate() override;
	};
};