#pragma once
#include <ext_win32.h>
#include <ext_d2d1.h>
#include <ext_vec2d.h>
#include <memory>
#include <limits>
#include <chrono>
#include <list>


namespace guipp
{
	extern float fSpacing;
	extern float fCornerRadius;
	extern float fStrokeWidth;
	unsigned NewId();

	class Window;
	class Object
	{
	public:
		Object() = default;
		Object(const ext::vec2d<float>& min_size) :min_size(min_size), bMinSizeUpToDate(true) {}
		virtual ~Object() {}

		virtual void OnGfxCreated(ext::D2DGraphics& gfx) {}

		virtual void OnDraw(ext::D2DGraphics& gfx) {}

		//search call
		virtual void OnInitialize(Window& wnd, bool bInitialize) {}

		//return this to receive future mouse messages
		//search call
		virtual Object* OnMouseHitTest(const ext::vec2d<float>& mpos_t) { return nullptr; }

		//target call
		virtual void OnMouseFocus(Window& wnd, bool bFocus) {}

		//return true if msg was processed
		//returning true on WM_MOUSEMOVE will prevent the app from checking for mouse_target->OnMouseHitTest()
		//target call
		virtual bool OnMouseMessage(Window& wnd, UINT msg, WPARAM wParam, const ext::vec2d<float>& mpos_t) { return false; }

		//return this to receive future kbd messages
		//search call
		virtual Object* OnKbdFocus(Window& wnd, bool bFirst) { return nullptr; }
		//return nullptr if there are no more items to iterate
		//target call
		virtual Object* OnKbdNext(Window& wnd, const Object* child, bool bNext) { OnKbdUnfocus(wnd); return nullptr; }
		//target call
		virtual void OnKbdUnfocus(Window& wnd) {}
		//loses kbd focus as soon as cursor hovers over anything that accepts kbd focus
		//target call
		virtual bool IsKbdWeak() { return true; }
		//return true if msg was processed
		//target call
		virtual bool OnKbdMessage(Window& wnd, UINT msg, unsigned key_code, LPARAM lParam) { return false; }

		void SetParent(Object* new_parent) { parent = new_parent; }
		void SetPos(ext::vec2d<float> new_pos)
		{
			pos = new_pos;
			OnSetPos();
		}
		void SetSize(ext::vec2d<float> new_size)
		{
			size = new_size;
			OnSetSize();
		}
		Object* GetParent() { return parent; }
		const ext::vec2d<float>& GetSize() const { return size; }
		const ext::vec2d<float>& GetPos() const { return pos; }
		//resize and reposition this branch
		void Reshuffle();

		const ext::vec2d<float>& GetMinSize();
	private:
		virtual void OnSetPos() {}
		virtual void OnSetSize() {}
		virtual ext::vec2d<float> OnMinSizeUpdate() { return { 0.0f,0.0f }; }
		ext::vec2d<float> pos = { 0.0f,0.0f }, size = { 0.0f,0.0f }, min_size = { 0.0f,0.0f };
		bool bMinSizeUpToDate = false;
		Object* parent = nullptr;
	};

	class MessageProcedure
	{
	public:
		virtual void OnMessage(Window& wnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	};

	class Updatable
	{
	public:
		//return false to exit the loop
		virtual bool OnUpdate(Window& wnd, float fElpasedTime) = 0;
	};

	class Window : public ext::Window, public ext::D2DGraphics, private Object
	{
		friend void MessagePump(Window**, const std::wstring&, std::shared_ptr<Object>, const ext::vec2d<int>&, bool, const wchar_t*);
		friend void MakeWindow(Window**, bool, const std::wstring&, std::shared_ptr<Object>, const ext::vec2d<int>&, bool, const wchar_t*);

		Window(const std::wstring& title, std::shared_ptr<Object> source, const ext::vec2d<int>& init_size, bool bGraphicResize, const wchar_t* wnd_class);
		Window(const Window&) = delete;
		~Window();
	public:
		static D2D1::ColorF color;

		//returns false if object was already binded before the call
		bool Bind(UINT msg, MessageProcedure* proc);
		//returns false if object was already unbinded before the call
		bool Unbind(UINT msg, MessageProcedure* proc);
		void AddToUpdateLoop(Updatable* proc);
		void RequestRedraw();
		void SetKbdTarget(Object* new_kbd_target);
		void UpdateMouseTarget();
		float GetScale() const { return fScale; }

	private:
		LRESULT AppProc(HWND, UINT msg, WPARAM wParam, LPARAM lParam) override;
		bool Update();
		void OnSetSize() override;
		ext::vec2d<float> OnMinSizeUpdate() override;
		void OnDraw(ext::D2DGraphics&) override;

		const bool bGraphicResize;
		const ext::vec2d<int> init_size;
		float fScale = 1.0f;
		bool bRedraw = false;
		
		std::shared_ptr<Object> source;

		Object* kbd_target = nullptr;
		int last_key_code_processed = 0;
		Object* mouse_target = nullptr;

		std::vector<Updatable*> update_loop;
		std::unordered_map<UINT, std::vector<MessageProcedure*>> procedures;

		std::chrono::steady_clock::time_point tp1;
		float fElapsedTime = 0.0f;
		bool bFirstUpdateAfterWait = true;
	};

	
	void MakeWindow(Window** ppWnd, bool bJoin, const std::wstring& title, std::shared_ptr<Object> source, const ext::vec2d<int>& init_size = { 400,400 }, bool bGraphicResize = false, const wchar_t* wnd_class = nullptr);
};