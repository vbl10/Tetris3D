#include "Tetris3D.h"
#include "resource.h"
#include <guipp.h>
#include <guipp_button.h>
#include <guipp_matrix.h>
#include <guipp_stack.h>
#include <guipp_switch.h>

using namespace guipp;

int CALLBACK wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	WNDCLASSEXW wc = ext::Window::DefClass::Get();
	wc.lpszClassName = L"tetris";
	wc.hIcon = LoadIconW(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	RegisterClassExW(&wc);

	std::shared_ptr<Window*> ppWnd(new Window*);

	ext::TextFormat 
		consolas70(L"consolas", 70.0f, DWRITE_WORD_WRAPPING_NO_WRAP, DWRITE_FONT_WEIGHT_EXTRA_BLACK),
		consolas20(L"consolas", 20.0f, DWRITE_WORD_WRAPPING_NO_WRAP, DWRITE_FONT_WEIGHT_EXTRA_BLACK);

	consolas70->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	consolas70->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, 68.0f, 68.0f * 0.75f);

	using std::make_shared, std::shared_ptr, ext::vec2d, ext::vec3d;

	enum PAGES { PAGE_HOME, PAGE_GAME };
	shared_ptr<Switch> sw_main = make_shared<Switch>();

	enum LAYERS { LAYER_GAME, LAYER_PAUSE, LAYER_GAME_OVER };
	shared_ptr<Stack> stk_game = make_shared<Stack>();
	shared_ptr<Tetris3D> game = make_shared<Tetris3D>(L"consolas", vec3d<int>{ 4, 10, 4 },
		[stk_game, ppWnd](Tetris3D::EVENT evt)
		{
			switch (evt)
			{
			case Tetris3D::EVENT::PAUSE:
				stk_game->ShowLayer(**ppWnd, LAYER_PAUSE, true);
				break;
			case Tetris3D::EVENT::GAME_OVER:
				stk_game->ShowLayer(**ppWnd, LAYER_GAME_OVER, true);
				break;
			}
		});
	{
		shared_ptr<Matrix> mat_panel = make_shared<Matrix>(Matrix::vec{
			game->GetStats(),
			game->GetNextDisplay(),
			game->GetProgressBar()
		}, vec2d<unsigned>{1,3});
		mat_panel->SetRow(0).proportion = 0;

		stk_game->Insert(LAYER_GAME,
			Stack::Layer(make_shared<Matrix>(Matrix::vec{
				game,
				mat_panel
				}, vec2d<unsigned>{2,1}, Matrix::STYLE_THICKFRAME),
				false, { 0.5f,0.5f,0.0f,0.0f }, { 0.0f,0.0f,0.0f,0.0f }));
		stk_game->At(LAYER_GAME).bConstAspRatio = true;

		stk_game->Insert(LAYER_PAUSE,
			Stack::Layer(make_shared<Matrix>(Matrix::vec{
				make_shared<Label>(consolas70, L"Pausa"),
				make_shared<Button>(make_shared<Label>(consolas20, L"Continuar"),
					[stk_game, game, ppWnd](int) 
					{
						stk_game->ShowLayer(**ppWnd, LAYER_PAUSE, false);
						game->Resume(**ppWnd);
					}),
				make_shared<Button>(make_shared<Label>(consolas20, L"Reiniciar"),
					[stk_game, game, ppWnd](int)
					{
						stk_game->ShowLayer(**ppWnd, LAYER_PAUSE, false);
						game->Reset();
						game->Resume(**ppWnd);
					}),
				make_shared<Button>(make_shared<Label>(consolas20, L"Sair"),
					[sw_main, game, ppWnd](int)
					{
						sw_main->Set(**ppWnd, PAGE_HOME);
						game->Reset();
					})
				}), 
				false, { 0.5f,0.5f,0.0f,0.0f }, { 0.0f,0.0f,0.0f,0.0f }),
			false);

		stk_game->Insert(LAYER_GAME_OVER,
			Stack::Layer(make_shared<Matrix>(Matrix::vec{
				make_shared<Label>(consolas70, L"Fim de\njogo"),
				make_shared<Button>(make_shared<Label>(consolas20, L"Novo jogo"),
					[stk_game, game, ppWnd](int) -> void {
						stk_game->ShowLayer(**ppWnd, LAYER_GAME_OVER, false);
						game->Reset();
						game->Resume(**ppWnd);
					}),
				make_shared<Button>(make_shared<Label>(consolas20, L"Sair"),
					[sw_main, game, ppWnd](int) -> void
					{
						sw_main->Set(**ppWnd, PAGE_HOME);
						game->Reset();
					})
				}),
				false, {0.5f,0.5f,0.0f,0.0f}, {0.0f,0.0f,0.0f,0.0f}),
			false);
	};

	sw_main->Insert(PAGE_GAME, Switch::Page(stk_game));
	sw_main->Insert(PAGE_HOME, Switch::Page(
		make_shared<Matrix>(Matrix::vec{
			make_shared<Label>(consolas70, L"Tetris3D"),
			make_shared<Button>(make_shared<Label>(consolas20, L"Jogar"),
				[stk_game, sw_main, game, ppWnd](int)
				{
					sw_main->Set(**ppWnd, PAGE_GAME);
					stk_game->ShowLayer(**ppWnd, LAYER_PAUSE, false);
					stk_game->ShowLayer(**ppWnd, LAYER_GAME_OVER, false);
					game->Resume(**ppWnd);
				}),
			make_shared<Button>(make_shared<Label>(consolas20, L"Como jogar"),
				[stk_game, sw_main, game, ppWnd](int)
				{
					sw_main->Set(**ppWnd, PAGE_GAME);
					stk_game->ShowLayer(**ppWnd, LAYER_PAUSE, false);
					stk_game->ShowLayer(**ppWnd, LAYER_GAME_OVER, false);
					game->Tutorial(**ppWnd);
				}),
			make_shared<Button>(make_shared<Label>(consolas20, L"Opções"),
				[](int)
				{

				})
		}), {0.5f,0.5f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}
	));

	sw_main->Set(**ppWnd, PAGE_HOME);

	guipp::MakeWindow(ppWnd.get(), true, L"Tetris3D", sw_main, {400,400}, true, wc.lpszClassName);

	UnregisterClassW(wc.lpszClassName, GetModuleHandle(NULL));
	return 0;
}