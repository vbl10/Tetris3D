#include "Tetris3D.h"
#include <fstream>
#include <random>
#include <iostream>

using namespace ext;

std::wstring key_name(int key_code)
{
	switch (key_code)
	{
	case ' ':
		return L"Espaço";
		break;
	case VK_TAB:
		return L"Tab";
		break;
	case VK_SHIFT:
		return L"Shift";
		break;
	case VK_RSHIFT:
		return L"Shift direito";
		break;
	case VK_LSHIFT:
		return L"Shift esquerdo";
		break;
	case VK_BACK:
		return L"Backspace";
		break;
	case VK_ESCAPE:
		return L"Esc";
		break;
	case VK_CONTROL:
		return L"Ctrl";
		break;
	case VK_RCONTROL:
		return L"Ctrl direito";
		break;
	case VK_LCONTROL:
		return L"Ctrl esquerdo";
		break;
	case VK_RETURN:
		return L"Enter";
		break;
	default:
		if (key_code >= VK_F1 && key_code <= VK_F24)
			return L"F" + std::to_wstring(key_code - VK_F1 + 1);
		return std::wstring(1,(char)key_code);
		break;
	}
}

vec3d<float> Tetris3D::light_source;

const unsigned Tetris3D::Tetromino::colors[8][2] =
{
	//traço
	0xff5050, 0xaa0000,
	//bloco
	0xffff40, 0x888800,
	//L
	0xff8844, 0x884400,
	//T
	0x008000, 0x006000,
	//T3D
	0x50ff50, 0x107710,
	//escada
	0x3f48cc, 0x2b339d,
	//escada sobe direita
	0xa349a4, 0x7b377b,
	//escada sobe esquerda
	0x00a2e8, 0x0079ae
};
const Tetris3D::Tetromino::Shape Tetris3D::Tetromino::shapes[8] =
{
	//traço
	{0,-2,0, 0,-1,0, 0,0,0, 0,1,0},
	//bloco
	{-1,-1,0, 0,-1,0, -1,0,0, 0,0,0},
	//L
	{-1,-1,0, 0,-1,0, 0,0,0, 0,1,0},
	//T
	{-1,-1,0, 0,-1,0, 1,-1,0, 0,0,0},
	//T3D
	{-1,-1,-1, -1,-1,0, 0,-1,0, -1,0,0},
	//escada
	{-1,-1,0, 0,-1,0, 0,0,0, 1,0,0},
	//escada sobe direita
	{-1,-1,-1, -1,-1,0, 0,-1,0, 0,0,0},
	//escada sobe esquerda
	{0,-1,-1, -1,-1,0, 0,-1,0, -1,0,0}
};
Tetris3D::Mesh Tetris3D::Tetromino::meshes[8];

Tetris3D::Tetris3D(const std::wstring& font_name, vec3d<int> dim, std::function<void(EVENT)> OnEvent)
	:
	play_field(dim),
	OnEvent(OnEvent),
	next_display(new NextDisplay),
	progress_bar(new ProgressBar(font_name)),
	font(font_name, 20.0f),
	font_small(font_name, 16.0f)
{
	srand(std::time(0));

	std::ifstream iputfile("tetris3d.dat", std::ios_base::binary);
	if (iputfile.is_open())
	{
		iputfile.seekg(0, iputfile.end);
		if (iputfile.tellg() == sizeof(GameStats))
		{
			iputfile.seekg(0);
			iputfile.read((char*)&best_game, sizeof(GameStats));
		}
		iputfile.close();
	}

	TextFormat font(font_name, 25.0f, DWRITE_WORD_WRAPPING_NO_WRAP, DWRITE_FONT_WEIGHT_EXTRA_BLACK);
	lb_score = std::make_shared<guipp::Label>(font, L"     0", vec2d<float>{1.0f, 0.5f});
	auto str = std::to_wstring(best_game.nScore);
	if (str.size() < 6);
		str = std::wstring(6 - str.size(), ' ') + str;
	lb_best = std::make_shared<guipp::Label>(font, str, vec2d<float>{1.0f, 0.5f});
	mat_stats = std::make_shared<guipp::Matrix>(guipp::Matrix::vec{
			std::make_shared<guipp::Label>(font, L"Pontuação: ", vec2d<float>{0.0f,0.5f}), lb_score,
			std::make_shared<guipp::Label>(font, L"Melhor:    ", vec2d<float>{0.0f,0.5f}), lb_best}, 
		vec2d<unsigned>{2, 2}, 
		guipp::Matrix::STYLE_THICKFRAME | 
		guipp::Matrix::STYLE_BACKGND |
		guipp::Matrix::STYLE_OUTLINE);

	mat_next = std::make_shared<guipp::Matrix>(guipp::Matrix::vec{
			std::make_shared<guipp::Label>(font, L"Próximo:", vec2d<float>{0.0f,0.5f}),
			next_display},
		vec2d<unsigned>{1, 2},
		guipp::Matrix::STYLE_THICKFRAME |
		guipp::Matrix::STYLE_BACKGND |
		guipp::Matrix::STYLE_OUTLINE);
	mat_next->SetRow(0).proportion = 0;


	//graphics stuff

	play_field.pos = -0.5f * play_field.dim;
	play_field.pos.z = 20.0f;

	light_source = { 0.0f,-0.125f,1.0f };
	light_source /= light_source.mod();

	//build tetromino meshes
	const vec3d<char> vfdim = { 5,5,5 };
	auto id_encoder = [&vfdim](vec3d<char> pos) -> int
	{
		pos += 2;
		return pos.x + pos.z * vfdim.x + pos.y * vfdim.x * vfdim.z;
	};
	auto id_decoder = [&vfdim](int id) -> vec3d<char>
	{
		return vec3d<char>{
			char(id % vfdim.x),
			char(id / (vfdim.x * vfdim.z)),
			char((id % (vfdim.x * vfdim.z)) / vfdim.x)
		} - 2;
	};
	Plane plane;
	plane.outline_thickness = 1.8f;
	plane.vx_ids.resize(4);
	for (int i = 0; i < 8; i++)
	{
		plane.fill_color = D2D1::ColorF(Tetromino::colors[i][0]);
		plane.outline_color = D2D1::ColorF(Tetromino::colors[i][1]);
		//create planes
		for (const auto& a : Tetromino::shapes[i].voxels)
		{
			plane.vx_ids[0] = id_encoder(a + vec3d<char>{0, 0, 0});
			plane.vx_ids[1] = id_encoder(a + vec3d<char>{0, 1, 0});
			plane.vx_ids[2] = id_encoder(a + vec3d<char>{0, 1, 1});
			plane.vx_ids[3] = id_encoder(a + vec3d<char>{0, 0, 1});
			Tetromino::meshes[i].geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});

			plane.vx_ids[0] = id_encoder(a + vec3d<char>{0, 0, 1});
			plane.vx_ids[1] = id_encoder(a + vec3d<char>{0, 1, 1});
			plane.vx_ids[2] = id_encoder(a + vec3d<char>{1, 1, 1});
			plane.vx_ids[3] = id_encoder(a + vec3d<char>{1, 0, 1});
			Tetromino::meshes[i].geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});

			plane.vx_ids[0] = id_encoder(a + vec3d<char>{1, 0, 1});
			plane.vx_ids[1] = id_encoder(a + vec3d<char>{1, 1, 1});
			plane.vx_ids[2] = id_encoder(a + vec3d<char>{1, 1, 0});
			plane.vx_ids[3] = id_encoder(a + vec3d<char>{1, 0, 0});
			Tetromino::meshes[i].geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});

			plane.vx_ids[0] = id_encoder(a + vec3d<char>{1, 0, 0});
			plane.vx_ids[1] = id_encoder(a + vec3d<char>{1, 1, 0});
			plane.vx_ids[2] = id_encoder(a + vec3d<char>{0, 1, 0});
			plane.vx_ids[3] = id_encoder(a + vec3d<char>{0, 0, 0});
			Tetromino::meshes[i].geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});

			plane.vx_ids[0] = id_encoder(a + vec3d<char>{1, 1, 0});
			plane.vx_ids[1] = id_encoder(a + vec3d<char>{1, 1, 1});
			plane.vx_ids[2] = id_encoder(a + vec3d<char>{0, 1, 1});
			plane.vx_ids[3] = id_encoder(a + vec3d<char>{0, 1, 0});
			Tetromino::meshes[i].geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});

			plane.vx_ids[0] = id_encoder(a + vec3d<char>{0, 0, 0});
			plane.vx_ids[1] = id_encoder(a + vec3d<char>{0, 0, 1});
			plane.vx_ids[2] = id_encoder(a + vec3d<char>{1, 0, 1});
			plane.vx_ids[3] = id_encoder(a + vec3d<char>{1, 0, 0});
			Tetromino::meshes[i].geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});
		}
		//create verticies and remap keys
		std::unordered_map<int, int> vx_id_map;
		for (auto geo : Tetromino::meshes[i].geometries)
		{
			for (int& vx_key : geo->vx_ids)
			{
				if (!vx_id_map.contains(vx_key))
				{
					vx_id_map[vx_key] = Tetromino::meshes[i].verticies.size();
					Tetromino::meshes[i].verticies.push_back(id_decoder(vx_key).to<float>());
				}
				vx_key = vx_id_map[vx_key];
			}
		}
	}

	NewTetromino();
}
Tetris3D::~Tetris3D()
{
	//save game and score
	std::ofstream oputfile("tetris3d.dat", std::ios_base::binary);
	if (oputfile.is_open())
	{
		oputfile.write((char*)&best_game, sizeof(best_game));
		oputfile.close();
	}
}
void Tetris3D::Resize(vec3d<int> dim)
{
	play_field.Resize(dim);
	Reset();
}
void Tetris3D::Reset()
{
	nTutorialStage = 0;

	play_field.angle = { 0.0f,0.0f,0.0f };

	play_field.Clear();
	if (cur_game.nScore > best_game.nScore)
	{
		best_game = cur_game;
		auto str = std::to_wstring(best_game.nScore);
		if (str.size() < 6)
			str = std::wstring(6 - str.size(), ' ') + str;
		lb_best->SetText(str).Reshuffle();
	}
	cur_game.nScore = 0;
	cur_game.nTetrominos = 0;

	auto str = std::to_wstring(cur_game.nScore);
	if (str.size() < 6)
		str = std::wstring(6 - str.size(), ' ') + str;
	lb_score->SetText(str).Reshuffle();
	
	progress_bar->Update(1, 0.0f);

	fGravitySpeed = 1.0f;
	fGravityTimer = 0.0f;
	NewTetromino();
}
void Tetris3D::Resume(guipp::Window& wnd)
{
	POINT pt;
	pt.x = center.x * wnd.GetScale();
	pt.y = center.y * wnd.GetScale();
	ClientToScreen(wnd.hWnd, &pt);
	SetCursorPos(pt.x, pt.y);
	ShowCursor(false);

	wnd.SetKbdTarget(this);
	for (auto& [name, code] : key_codes)
		GetAsyncKeyState(code);
	for (auto& [name, key] : keys)
	{
		key.bPressed = false;
		key.bHeld = GetAsyncKeyState(key_codes[name]);
		key.bReleased = false;
		key.bAutoRepeat = false;
		key.fTimer1 = 0.0f;
		key.fTimer2 = 0.0f;
	}

	wnd.AddToUpdateLoop(this);
}
void Tetris3D::Tutorial(guipp::Window& wnd)
{
	bTutorial = true;
	tetromino.pos.y = play_field.dim.y / 2;
	Resume(wnd);
}

vec3d<float> Tetris3D::NextDisplay::tetro_pivot[8] =
{
	//traço
	{0.5f,0.0f,0.5f},
	//bloco
	{0.0f,0.0f,0.5f},
	//L
	{0.5f,0.5f,0.5f},
	//T
	{0.5f,0.0f,0.5f},
	//T3D
	{0.0f,0.0f,0.0f},
	//escada
	{0.5f,0.0f,0.5f},
	//escada sobe direita
	{0.0f,0.0f,0.0f},
	//escada sobe esquerda
	{0.0f,0.0f,0.0f}
};
int Tetris3D::NextDisplay::Get()
{
	int temp = next;
	next = ((float)rand() / float(RAND_MAX + 1)) * 8.0f;
	return temp;
}
void Tetris3D::NextDisplay::Update(float fElapsedTime)
{
	angle.y += (fElapsedTime / 6.0f) * 2.0f * pi;
	while (angle.y >= 2.0f * pi)
		angle.y -= 2.0f * pi;
}
void Tetris3D::NextDisplay::OnDraw(ext::D2DGraphics& gfx)
{
	auto mat =
		Mat4x4_Translate(vec3d<float>{0.0f, 0.0f, 20.0f})*
		Mat4x4_RotateZ(angle.z)*
		Mat4x4_RotateX(angle.x)*
		Mat4x4_RotateY(angle.y)*
		Mat4x4_Translate(-tetro_pivot[next]);

	//4 is the max size of a tetromino in any dimension
	float fPxSizeAtDepth20 = std::min(GetSize().x, GetSize().y) * 20.0f * 0.8f / 4.0f;
	Mesh mesh = Tetromino::meshes[next];

	//3d transform
	for (auto& vx : mesh.verticies) vx = mat * vx;

	//back face culling
	std::erase_if(mesh.geometries,
		[&mesh](std::shared_ptr<Geometry> ptr) {
			return ptr->Cull(mesh.verticies);
		});

	//painter's algorithm
	mesh.SortGeometriesByZ();

	//lighting
	for (auto& geo : mesh.geometries)
	{
		const auto& p0 = mesh.verticies[geo->vx_ids[0]];
		const auto& p1 = mesh.verticies[geo->vx_ids[1]];
		const auto& p2 = mesh.verticies[geo->vx_ids[2]];
		auto v1 = p1 - p0;
		auto v2 = p2 - p0;
		auto cross = v1.cross(v2);
		cross /= cross.mod();
		geo->fLightIntensity = cross.dot(light_source) * 0.5f + 0.5f;
	}

	//projection
	vec2d<float> center = GetPos() + GetSize() * 0.5f;
	for (auto& vx : mesh.verticies)
	{
		vx /= vx.z;
		vx.y = -vx.y;
		vx *= fPxSizeAtDepth20;
		vx.x += center.x;
		vx.y += center.y;
	}

	for (auto& geo : mesh.geometries)
		geo->Draw(gfx, mesh.verticies);
}

Tetris3D::ProgressBar::ProgressBar(const std::wstring& font_name)
{
	TextFormat 
		font1(font_name, 25.0f, DWRITE_WORD_WRAPPING_NO_WRAP, DWRITE_FONT_WEIGHT_EXTRA_BLACK),
		font2(font_name, 50.0f, DWRITE_WORD_WRAPPING_NO_WRAP, DWRITE_FONT_WEIGHT_EXTRA_BLACK);
	lb1 = std::make_shared<guipp::Label>(font1, L"Nível:");
	lb2 = std::make_shared<guipp::Label>(font2, L"1");
	mat = std::make_shared<guipp::Matrix>(
		guipp::Matrix::vec{ lb1, lb2 }, 
		vec2d<unsigned>{1,2}, 
		guipp::Matrix::STYLE_THICKFRAME);
	mat->SetParent(this);
	mat->SetSize(mat->GetMinSize());
}
void Tetris3D::ProgressBar::Update(int nLevel, float fProgress)
{
	lb2->SetText(std::to_wstring(nLevel)).Reshuffle();
	this->fProgress = fProgress;
}
void Tetris3D::ProgressBar::OnDraw(ext::D2DGraphics& gfx)
{
	gfx.pSolidBrush->SetColor(D2D1::ColorF(0xffffff));
	gfx.pRenderTarget->DrawRoundedRectangle(
		D2D1::RoundedRect(
			D2D1::RectF(
				GetPos().x,
				GetPos().y,
				GetPos().x + GetSize().x,
				GetPos().y + GetSize().y),
			guipp::fCornerRadius, guipp::fCornerRadius),
		gfx.pSolidBrush);
	gfx.pSolidBrush->SetColor(D2D1::ColorF(0x00aa00));
	gfx.pRenderTarget->FillRoundedRectangle(
		D2D1::RoundedRect(
			D2D1::RectF(
				GetPos().x + guipp::fSpacing,
				GetPos().y + guipp::fSpacing + (GetSize().y - guipp::fSpacing * 2.0f) * (1.0f - fProgress),
				GetPos().x + GetSize().x - guipp::fSpacing,
				GetPos().y + GetSize().y - guipp::fSpacing),
			guipp::fCornerRadius, guipp::fCornerRadius),
		gfx.pSolidBrush);
	mat->OnDraw(gfx);
}
vec2d<float> Tetris3D::ProgressBar::OnMinSizeUpdate()
{
	return mat->GetMinSize() + 4.0f * guipp::fSpacing;
}
void Tetris3D::ProgressBar::OnSetPos()
{
	mat->SetPos(GetPos() + (GetSize() - mat->GetSize()) * 0.5f);
}

void Tetris3D::OnDraw(D2DGraphics& gfx)
{
	auto pproject = [&](vec3d<float>& v) -> void
	{
		v.x = v.x * fScale / v.z;
		v.y = v.y * fScale / v.z;
		v.y = -v.y;
		v.x += center.x;
		v.y += center.y;
	};

	const auto mat_pf = play_field.Transform();

	Mesh mesh = play_field.GetMeshGrid() + play_field.GetMeshVoxels() + tetromino.GetMesh() + tetromino.GetShadowMesh(play_field);
	if (bShowGhost)
	{
		mesh = mesh + tetromino.GetGhostMesh(play_field);
	}

	//transform
	for (auto& vx : mesh.verticies) vx = mat_pf * vx;

	//back face culling
	std::erase_if(mesh.geometries,
		[&mesh](std::shared_ptr<Geometry> ptr) {
			return ptr->Cull(mesh.verticies);
		});

	//lighting
	for (auto& geo : mesh.geometries)
	{
		const auto& p0 = mesh.verticies[geo->vx_ids[0]];
		const auto& p1 = mesh.verticies[geo->vx_ids[1]];
		const auto& p2 = mesh.verticies[geo->vx_ids[2]];
		auto v1 = p1 - p0;
		auto v2 = p2 - p0;
		auto cross = v1.cross(v2);
		cross /= cross.mod();
		geo->fLightIntensity = cross.dot(light_source) * 0.5f + 0.5f;
	}

	//projection
	for (auto& vx : mesh.verticies) pproject(vx);

	mesh.SortGeometriesByZ();

	gfx.pRenderTarget->PushAxisAlignedClip(
		D2D1::RectF(
			GetPos().x,
			GetPos().y,
			GetPos().x + GetSize().x,
			GetPos().y + GetSize().y), 
		D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	for (auto geo : mesh.geometries) geo->Draw(gfx, mesh.verticies);

	gfx.pRenderTarget->PopAxisAlignedClip();

	gfx.pSolidBrush->SetColor(D2D1::ColorF(0xffffff));
	gfx.pRenderTarget->DrawRoundedRectangle(
		D2D1::RoundedRect(
			D2D1::RectF(
				GetPos().x,
				GetPos().y,
				GetPos().x + GetSize().x,
				GetPos().y + GetSize().y),
			guipp::fCornerRadius, guipp::fCornerRadius),
		gfx.pSolidBrush, 2.0f);

	if (bTutorial)
	{
		switch (nTutorialStage)
		{
		case 0:
		{
			DWRITE_TEXT_METRICS metrics;
			auto layout1 = font(
				L"Pressione " + key_name(key_codes[KN_PAUSE]) + L" para pausar\n"
				L"e " + key_name(key_codes[KN_RESET]) + L" para recomeçar."
			);
			layout1->GetMetrics(&metrics);
			float l1h = metrics.height;
			
			auto layout2 = font_small(L"Pressione espaço para avançar...");
			layout2->GetMetrics(&metrics);
			float l2h = metrics.height;
			

			gfx.pSolidBrush->SetColor(D2D1::ColorF(0x808080, 0.3f * std::min(1.0f, fTutorialTimers[0])));

			gfx.pRenderTarget->FillRoundedRectangle(
				D2D1::RoundedRect(
					D2D1::RectF(GetPos().x + 5.0f, GetPos().y + 5.0f, GetPos().x + GetSize().x - 5.0f, GetPos().y + 5.0f + l1h + l2h + 10.0f),
					guipp::fCornerRadius, guipp::fCornerRadius),
				gfx.pSolidBrush);

			gfx.pSolidBrush->SetColor(D2D1::ColorF(0xffffff, std::min(1.0f, fTutorialTimers[0])));

			gfx.pRenderTarget->DrawTextLayout(
				GetPos() + GetSize() * vec2d<float>{0.03f, 0.02f},
				layout1,
				gfx.pSolidBrush
			);
			
			gfx.pRenderTarget->DrawTextLayout(
				GetPos() + GetSize() * vec2d<float>{0.03f, 0.02f} + vec2d<float>{0.0f, l1h},
				layout2,
				gfx.pSolidBrush
			);
		}
			break;
		case 1:
			gfx.pRenderTarget->DrawTextLayout
			(
				GetPos() + GetSize() * vec2d<float>{0.03f, 0.02f},
				font(
					L"Utilize as teclas " + key_name(key_codes[KN_PUSH]) + key_name(key_codes[KN_PULL]) + key_name(key_codes[KN_RIGHT]) + key_name(key_codes[KN_LEFT]) + L"\n"
					L"para mover a peça.\n"
					L"Pressione espaço para\n"
					L"avançar..."
				),
				gfx.pSolidBrush
			);
			break;
		case 2:
			gfx.pRenderTarget->DrawTextLayout
			(
				GetPos() + GetSize() * vec2d<float>{0.03f, 0.02f},
				font(
					L"Utilize as teclas " + key_name(key_codes[KN_ROTATE_CW]) + key_name(key_codes[KN_ROTATE_CCW]) + key_name(key_codes[KN_ROTATE_YCW]) + key_name(key_codes[KN_ROTATE_YCCW]) + L"\n"
					L"para girar a peça.\n"
					L"Pressione espaço para\n"
					L"avançar..."
				),
				gfx.pSolidBrush
			);
			if (nTutorialInts[1])
			{
				std::wstring str;
				switch (nTutorialInts[1])
				{
				case 1:
					str =
						L"Rotação no sentido\n"
						L"horário [" + key_name(key_codes[KN_ROTATE_CW]) + L"] bloqueada\n"
						L"pois colidiria com o terreno.";
					break;
				case 2:
					str =
						L"Rotação no sentido\n"
						L"anti-horário [" + key_name(key_codes[KN_ROTATE_CCW]) + L"] bloqueada\n"
						L"pois colidiria com o terreno.";
					break;
				case 3:
					str =
						L"Rotação no eixo Y sentido\n"
						L"horário [" + key_name(key_codes[KN_ROTATE_YCW]) + L"] bloqueada\n"
						L"pois colidiria com o terreno.";
					break;
				case 4:
					str =
						L"Rotação no eixo Y sentido\n"
						L"anti-horário[" + key_name(key_codes[KN_ROTATE_YCCW]) + L"] bloqueada\n"
						L"pois colidiria com o terreno.";
					break;
				}
				gfx.pSolidBrush->SetColor(D2D1::ColorF(0xffffff, std::min(1.0f, fTutorialTimers[1])));
				gfx.pRenderTarget->DrawTextLayout
				(
					GetPos() + GetSize() * vec2d<float>{0.03f, 0.84f},
					font(str),
					gfx.pSolidBrush
				);
			}
			break;
		case 3:
			gfx.pRenderTarget->DrawTextLayout
			(
				GetPos() + GetSize() * vec2d<float>{0.03f, 0.02f},
				font(
					L"Mova o mouse para girar\n"
					L"o jogo.\n"
					L"Pressione espaço para\n"
					L"avançar..."
				),
				gfx.pSolidBrush
			);
			if (fTutorialTimers[2] > 0.0f)
			{
				gfx.pSolidBrush->SetColor(D2D1::ColorF(0xffffff, std::min(1.0f, fTutorialTimers[2])));
				gfx.pRenderTarget->DrawTextLayout
				(
					GetPos() + GetSize() * vec2d<float>{0.03f, 0.84f},
					font(
						L"Os movimentos de rotação\n"
						L"e translação são relativos\n"
						L"à posição da câmera."
					),
					gfx.pSolidBrush
				);
			}
			break;
		case 4:
			gfx.pRenderTarget->DrawTextLayout
			(
				GetPos() + GetSize() * vec2d<float>{0.03f, 0.02f},
				font(
					L"Mantenha " + key_name(key_codes[KN_DOWN]) + L" pressionado\n"
					L"para descer a peça até que\n"
					L"encoste no chão..."
				),
				gfx.pSolidBrush
			);
			break;
		}
	}
}
bool Tetris3D::OnUpdate(guipp::Window& wnd, float fElapsedTime)
{
	for (auto& [key_name, key] : keys)
	{
		if (bool(GetAsyncKeyState(key_codes[key_name])))
		{
			key.bPressed = !key.bHeld;
			key.bHeld = true;
			if ((key.fTimer1 += fElapsedTime) > InputKey::fAutoRepeatBeginThreshold)
			{
				if ((key.fTimer2 -= fElapsedTime) < 0.0f)
				{
					key.fTimer2 += InputKey::fAutoRepeatThreshold;
					key.bAutoRepeat = true;
				}
				else
				{
					key.bAutoRepeat = false;
				}
			}
		}
		else if (key.bHeld)
		{
			key.bPressed = false;
			key.bHeld = false;
			key.bReleased = true;

			key.bAutoRepeat = false;
			key.fTimer1 = 0.0f;
			key.fTimer2 = 0.0f;
		}
		else
		{
			key.bPressed = false;
			key.bHeld = false;
			key.bReleased = false;

			key.bAutoRepeat = false;
			key.fTimer1 = 0.0f;
			key.fTimer2 = 0.0f;
		}
	}

	if (bTutorial)
	{
		fTutorialTimers[0] += fElapsedTime;
		if (fTutorialTimers[0] > 1.0f)
			fTutorialTimers[0] = 1.0f;

		if (nTutorialStage == 0 && keys[KN_SPACE].bPressed)
		{
			nTutorialStage++; 
			fTutorialTimers[0] = 0.0f;
		}

		if (keys[KN_PAUSE].bPressed || wnd.hWnd != GetActiveWindow())
		{
			ShowCursorX(true);
			OnEvent(EVENT::PAUSE);
			return false;
		}
		if (keys[KN_RESET].bPressed)
		{
			Reset();
		}
		if (keys[KN_SHOW_GHOST].bPressed && !GetAsyncKeyState(VK_MENU))
		{
			bShowGhost = !bShowGhost;
		}

		if (nTutorialStage >= 4)
		{
			if (keys[KN_DOWN].bPressed || keys[KN_DOWN].bAutoRepeat)
			{
				auto pos = tetromino.pos;
				pos.y--;
				if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
				{
					tetromino.pos.y--;
				}
				else
				{
					pos.y++;
					if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::OVER_ROOF))
					{
						int nPlanes = play_field.PutTetromino(tetromino.id, tetromino.GetShape(), tetromino.pos);
						cur_game.nScore += nPlanes * nPlanes * play_field.dim.x * play_field.dim.z;

						auto str = std::to_wstring(cur_game.nScore);
						if (str.size() < 6)
							str = std::wstring(6 - str.size(), ' ') + str;
						lb_score->SetText(str).Reshuffle();


						cur_game.nTetrominos++;
						if (cur_game.IsLevelUp())
						{
							fGravitySpeed = std::min(5.0f, (float)cur_game.nTetrominos * 4.0f / 300.0f + 1.0f);
						}
						progress_bar->Update(cur_game.nTetrominos / 10 + 1, (float)(cur_game.nTetrominos % 10) * 0.1f);
						NewTetromino();
					}
					else
					{
						ShowCursorX(true);
						OnEvent(EVENT::GAME_OVER);
						return false;
					}
				}
			}
		}

		//play field rotation
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(wnd.hWnd, &pt);
			vec2d<int> delta = { pt.x - center.x * wnd.GetScale(),pt.y - center.y * wnd.GetScale() };
			if (delta.x != 0 || delta.y != 0)
			{
				pt.x -= delta.x;
				pt.y -= delta.y;
				ClientToScreen(wnd.hWnd, &pt);
				SetCursorPos(pt.x, pt.y);
				if (nTutorialStage >= 3)
				{
					play_field.angle.y -= 0.01f * delta.x;
					if (fabs(play_field.angle.y -= 0.01f * delta.x) >= 2.0f * pi)
					{
						play_field.angle.y = 0.0f;
					}
					if (fabs(play_field.angle.x -= 0.01f * delta.y) >= 2.0f * pi)
					{
						play_field.angle.x = 0.0f;
					}
					if (nTutorialStage == 3)
						nTutorialInts[0] = true;
				}
			}
			if (nTutorialStage == 3 && nTutorialInts[0] && (fTutorialTimers[1] += fElapsedTime) > 4.5f)
			{
				if ((fTutorialTimers[2] += fElapsedTime) > 4.5f)
				{
					nTutorialInts[0] = false;
					fTutorialTimers[2] = 0.0f;
					fTutorialTimers[1] = 0.0f;
					fTutorialTimers[0] = 0.0f;
					nTutorialStage++;
				}
			}
		}

		//tetromino rotation
		if (nTutorialStage >= 2)
		{
			if (keys[KN_ROTATE_CW].bPressed)
			{
				auto shape = tetromino.GetShape();
				auto vec_y = play_field.UnVecY();
				shape.RotateZ(-1 * vec_y.x);
				shape.RotateX(+1 * vec_y.y);
				if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
				{
					tetromino.RotateZ(-1 * vec_y.x);
					tetromino.RotateX(+1 * vec_y.y);
					nTutorialInts[0] = nTutorialStage == 2;
				}
				else if (nTutorialStage == 2)
				{
					nTutorialInts[1] = 1;
					fTutorialTimers[1] = 4.5f;
				}
			}
			else if (keys[KN_ROTATE_CCW].bPressed)
			{
				auto shape = tetromino.GetShape();
				auto vec_y = play_field.UnVecY();
				shape.RotateZ(+1 * vec_y.x);
				shape.RotateX(-1 * vec_y.y);
				if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
				{
					tetromino.RotateZ(+1 * vec_y.x);
					tetromino.RotateX(-1 * vec_y.y);
					nTutorialInts[0] = nTutorialStage == 2;
				}
				else if (nTutorialStage == 2)
				{
					nTutorialInts[1] = 2;
					fTutorialTimers[1] = 4.5f;
				}
			}
			else if (keys[KN_ROTATE_YCW].bPressed)
			{
				auto shape = tetromino.GetShape();
				shape.RotateY(1);
				if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
				{
					tetromino.RotateY(1);
					nTutorialInts[0] = nTutorialStage == 2;
				}
				else if (nTutorialStage == 2)
				{
					nTutorialInts[1] = 3;
					fTutorialTimers[1] = 4.5f;
				}
			}
			else if (keys[KN_ROTATE_YCCW].bPressed)
			{
				auto shape = tetromino.GetShape();
				shape.RotateY(-1);
				if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
				{
					tetromino.RotateY(-1);
					nTutorialInts[0] = nTutorialStage == 2;
				}
				else if (nTutorialStage == 2)
				{
					nTutorialInts[1] = 4;
					fTutorialTimers[1] = 4.5f;
				}
			}
			if (nTutorialStage == 2)
			{
				if (keys[KN_SPACE].bPressed)
				{
					keys[KN_SPACE].bPressed = false;
					nTutorialInts[0] = 0;
					nTutorialInts[1] = 0;
					fTutorialTimers[2] = 0.0f;
					fTutorialTimers[1] = 0.0f;
					fTutorialTimers[0] = 0.0f;
					nTutorialStage++;
				}
				fTutorialTimers[1] -= fElapsedTime;
				if (fTutorialTimers[1] < 0.0f)
					fTutorialTimers[1] = 0.0f;
			}
		}

		//tetromino translation
		if (nTutorialStage >= 1)
		{
			if (keys[KN_PUSH].bPressed || keys[KN_PUSH].bAutoRepeat)
			{
				auto pos = tetromino.pos;
				auto vec = play_field.UnVecY();
				pos.z += vec.x;
				pos.x -= vec.y;
				if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
				{
					tetromino.pos = pos;
				}
				if (nTutorialStage == 1)
				{
					nTutorialInts[0] = true;
				}
			}
			else if (keys[KN_PULL].bPressed || keys[KN_PULL].bAutoRepeat)
			{
				auto pos = tetromino.pos;
				auto vec = play_field.UnVecY();
				pos.z -= vec.x;
				pos.x += vec.y;
				if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
				{
					tetromino.pos = pos;
				}
				if (nTutorialStage == 1)
				{
					nTutorialInts[0] = true;
				}
			}
			if (keys[KN_RIGHT].bPressed || keys[KN_RIGHT].bAutoRepeat)
			{
				auto pos = tetromino.pos;
				auto vec = play_field.UnVecY();
				pos.z += vec.y;
				pos.x += vec.x;
				if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
				{
					tetromino.pos = pos;
				}
				if (nTutorialStage == 1)
				{
					nTutorialInts[0] = true;
				}
			}
			else if (keys[KN_LEFT].bPressed || keys[KN_LEFT].bAutoRepeat)
			{
				auto pos = tetromino.pos;
				auto vec = play_field.UnVecY();
				pos.z -= vec.y;
				pos.x -= vec.x;
				if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
				{
					tetromino.pos = pos;
				}
				if (nTutorialStage == 1)
				{
					nTutorialInts[0] = true;
				}
			}
			if (nTutorialStage == 1 && keys[KN_SPACE].bPressed)
			{
				nTutorialInts[0] = false;
				fTutorialTimers[0] = 0.0f;
				fTutorialTimers[1] = 0.0f;
				nTutorialStage++;
			}
		}
	}
	else
	{
		if (keys[KN_PAUSE].bPressed || wnd.hWnd != GetActiveWindow())
		{
			ShowCursorX(true);
			OnEvent(EVENT::PAUSE);
			return false;
		}
		if (keys[KN_RESET].bPressed)
		{
			Reset();
		}
		if (keys[KN_SHOW_GHOST].bPressed && !GetAsyncKeyState(VK_MENU))
		{
			bShowGhost = !bShowGhost;
		}

		if ((fGravityTimer += fElapsedTime) > 1.0f / (fGravitySpeed + keys[KN_DOWN].bHeld * 7.0f))
		{
			fGravityTimer = 0.0f;
			keys[KN_DOWN].bPressed = true;
		}
		if (keys[KN_DOWN].bPressed)
		{
			auto pos = tetromino.pos;
			pos.y--;
			if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
			{
				tetromino.pos.y--;
			}
			else
			{
				pos.y++;
				if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::OVER_ROOF))
				{
					int nPlanes = play_field.PutTetromino(tetromino.id, tetromino.GetShape(), tetromino.pos);
					cur_game.nScore += nPlanes * nPlanes * play_field.dim.x * play_field.dim.z;

					auto str = std::to_wstring(cur_game.nScore);
					if (str.size() < 6)
						str = std::wstring(6 - str.size(), ' ') + str;
					lb_score->SetText(str).Reshuffle();


					cur_game.nTetrominos++;
					if (cur_game.IsLevelUp())
					{
						fGravitySpeed = std::min(5.0f, (float)cur_game.nTetrominos * 4.0f / 300.0f + 1.0f);
					}
					progress_bar->Update(cur_game.nTetrominos / 10 + 1, (float)(cur_game.nTetrominos % 10) * 0.1f);
					NewTetromino();
				}
				else
				{
					ShowCursorX(true);
					OnEvent(EVENT::GAME_OVER);
					return false;
				}
			}
		}

		//play field rotation
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(wnd.hWnd, &pt);
			vec2d<int> delta = { pt.x - center.x * wnd.GetScale(),pt.y - center.y * wnd.GetScale() };
			if (delta.x != 0 || delta.y != 0)
			{
				pt.x -= delta.x;
				pt.y -= delta.y;
				ClientToScreen(wnd.hWnd, &pt);
				SetCursorPos(pt.x, pt.y);
				play_field.angle.y -= 0.01f * delta.x;
				if (fabs(play_field.angle.y -= 0.01f * delta.x) >= 2.0f * pi)
				{
					play_field.angle.y = 0.0f;
				}
				if (fabs(play_field.angle.x -= 0.01f * delta.y) >= 2.0f * pi)
				{
					play_field.angle.x = 0.0f;
				}
			}
		}

		//tetromino rotation
		if (keys[KN_ROTATE_CW].bPressed)
		{
			auto shape = tetromino.GetShape();
			auto vec_y = play_field.UnVecY();
			shape.RotateZ(-1 * vec_y.x);
			shape.RotateX(+1 * vec_y.y);
			if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
			{
				tetromino.RotateZ(-1 * vec_y.x);
				tetromino.RotateX(+1 * vec_y.y);
			}
		}
		else if (keys[KN_ROTATE_CCW].bPressed)
		{
			auto shape = tetromino.GetShape();
			auto vec_y = play_field.UnVecY();
			shape.RotateZ(+1 * vec_y.x);
			shape.RotateX(-1 * vec_y.y);
			if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
			{
				tetromino.RotateZ(+1 * vec_y.x);
				tetromino.RotateX(-1 * vec_y.y);
			}
		}
		else if (keys[KN_ROTATE_YCW].bPressed)
		{
			auto shape = tetromino.GetShape();
			shape.RotateY(1);
			if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
			{
				tetromino.RotateY(1);
			}
		}
		else if (keys[KN_ROTATE_YCCW].bPressed)
		{
			auto shape = tetromino.GetShape();
			shape.RotateY(-1);
			if (!(play_field.TestTetromino(shape, tetromino.pos) & PlayField::COLLISION))
			{
				tetromino.RotateY(-1);
			}
		}

		//tetromino translation
		if (keys[KN_PUSH].bPressed || keys[KN_PUSH].bAutoRepeat)
		{
			auto pos = tetromino.pos;
			auto vec = play_field.UnVecY();
			pos.z += vec.x;
			pos.x -= vec.y;
			if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
			{
				tetromino.pos = pos;
			}
		}
		else if (keys[KN_PULL].bPressed || keys[KN_PULL].bAutoRepeat)
		{
			auto pos = tetromino.pos;
			auto vec = play_field.UnVecY();
			pos.z -= vec.x;
			pos.x += vec.y;
			if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
			{
				tetromino.pos = pos;
			}
		}
		if (keys[KN_RIGHT].bPressed || keys[KN_RIGHT].bAutoRepeat)
		{
			auto pos = tetromino.pos;
			auto vec = play_field.UnVecY();
			pos.z += vec.y;
			pos.x += vec.x;
			if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
			{
				tetromino.pos = pos;
			}
		}
		else if (keys[KN_LEFT].bPressed || keys[KN_LEFT].bAutoRepeat)
		{
			auto pos = tetromino.pos;
			auto vec = play_field.UnVecY();
			pos.z -= vec.y;
			pos.x -= vec.x;
			if (!(play_field.TestTetromino(tetromino.GetShape(), pos) & PlayField::COLLISION))
			{
				tetromino.pos = pos;
			}
		}
	}

	next_display->Update(fElapsedTime);

	wnd.RequestRedraw();

	return true;
}
void Tetris3D::OnSetSize()
{
	fScale =
		std::min(
			GetSize().x / float(std::max(play_field.dim.x, play_field.dim.z)),
			GetSize().y / (float)(play_field.dim.y + 3))
		* play_field.pos.z;
}
void Tetris3D::OnSetPos()
{
	center = GetPos() + GetSize() * 0.5f;
}
vec2d<float> Tetris3D::OnMinSizeUpdate()
{
	return { 350.0f,550.0f };
}
void Tetris3D::NewTetromino()
{
	tetromino.pos = play_field.dim / 2;
	tetromino.pos.y = play_field.dim.y + 2;
	tetromino.id = next_display->Get();
	tetromino.Reset();
}


bool Tetris3D::Geometry::Cull(const std::vector<vec3d<float>>& vx_pool) const
{
	return
		vx_ids.size() > 2 &&
		((vx_pool[vx_ids[1]] - vx_pool[vx_ids[0]]).cross
		(vx_pool[vx_ids[2]] - vx_pool[vx_ids[0]])).dot
		(vx_pool[vx_ids[0]]) < 0.0f;
}
Tetris3D::Plane* Tetris3D::Plane::NewCopy() const
{
	return new Plane(*this);
}
void Tetris3D::Plane::Draw(D2DGraphics& gfx, const std::vector<vec3d<float>>& vx_pool) const
{
	CComPtr<ID2D1PathGeometry> path;
	d2dFactory()->CreatePathGeometry(&path);
	CComPtr<ID2D1GeometrySink> sink;
	path->Open(&sink);
	sink->BeginFigure({ vx_pool[vx_ids.back()].x,vx_pool[vx_ids.back()].y }, D2D1_FIGURE_BEGIN_FILLED);
	for (int id : vx_ids)
	{
		sink->AddLine({ vx_pool[id].x,vx_pool[id].y });
	}
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	if (fill_color.a > 0.0f)
	{
		auto col = fill_color;
		col.r *= fLightIntensity;
		col.g *= fLightIntensity;
		col.b *= fLightIntensity;
		gfx.pSolidBrush->SetColor(col);
		gfx.pRenderTarget->FillGeometry(path, gfx.pSolidBrush);
	}
	if (outline_color.a > 0.0f && outline_thickness > 0.0f)
	{
		auto col = outline_color;
		col.r *= fLightIntensity;
		col.g *= fLightIntensity;
		col.b *= fLightIntensity;
		gfx.pSolidBrush->SetColor(col);
		gfx.pRenderTarget->DrawGeometry(path, gfx.pSolidBrush, outline_thickness);
	}
}
Tetris3D::Lines* Tetris3D::Lines::NewCopy() const
{
	return new Lines(*this);
}
void Tetris3D::Lines::Draw(D2DGraphics& gfx, const std::vector<vec3d<float>>& vx_pool) const
{
	if (thickness > 0.0f)
	{
		gfx.pSolidBrush->SetColor(color);
		for (int i = 0, j = vx_ids.size(); i < j - 1; i++)
		{
			gfx.pRenderTarget->DrawLine(
				{ vx_pool[vx_ids[i]].x,vx_pool[vx_ids[i]].y },
				{ vx_pool[vx_ids[i + 1]].x,vx_pool[vx_ids[i + 1]].y },
				gfx.pSolidBrush,
				thickness
			);
		}
	}
}
Tetris3D::Mesh Tetris3D::Mesh::operator+(Mesh rhs) const
{
	Mesh output;
	int offset = verticies.size();
	output.verticies.insert(output.verticies.end(), verticies.begin(), verticies.end());
	output.verticies.insert(output.verticies.end(), rhs.verticies.begin(), rhs.verticies.end());
	for (auto geo : geometries)
	{
		output.geometries.push_back(std::shared_ptr<Geometry>(geo->NewCopy()));
	}
	for (auto geo : rhs.geometries)
	{
		output.geometries.push_back(std::shared_ptr<Geometry>(geo->NewCopy()));
		for (auto& id : output.geometries.back()->vx_ids)
		{
			id += offset;
		}
	}
	return output;
}
void Tetris3D::Mesh::SortGeometriesByZ()
{
	std::sort(geometries.begin(), geometries.end(),
		[&](const std::shared_ptr<Geometry>& a, const std::shared_ptr<Geometry>& b) -> bool 
		{
			float az = 0.0f;
			for (int id : a->vx_ids)
			{
				az += verticies[id].z;
			}
			az /= (float)a->vx_ids.size();
			float bz = 0.0f;
			for (int id : b->vx_ids)
			{
				bz += verticies[id].z;
			}
			bz /= (float)b->vx_ids.size();
			return az > bz;
		}
	);
}


void Tetris3D::Tetromino::Shape::RotateX(int quads)
{
	if (quads % 4)
	{
		auto mat = Mat4x4_RotateX(pi * 0.5f * (float)quads);
		for (auto& v : voxels)
		{
			auto t = mat * (v.to<float>() + 0.5f);
			v.x = floorf(t.x);
			v.y = floorf(t.y);
			v.z = floorf(t.z);
		}
	}
}
void Tetris3D::Tetromino::Shape::RotateY(int quads)
{
	if (quads % 4)
	{
		auto mat = Mat4x4_RotateY(pi * 0.5f * (float)quads);
		for (auto& v : voxels)
		{
			auto t = mat * (v.to<float>() + 0.5f);
			v.x = floorf(t.x);
			v.y = floorf(t.y);
			v.z = floorf(t.z);
		}
	}
}
void Tetris3D::Tetromino::Shape::RotateZ(int quads)
{
	if (quads % 4)
	{
		auto mat = Mat4x4_RotateZ(pi * 0.5f * (float)quads);
		for (auto& v : voxels)
		{
			auto t = mat * (v.to<float>() + 0.5f);
			v.x = floorf(t.x);
			v.y = floorf(t.y);
			v.z = floorf(t.z);
		}
	}
}
vec3d<char>& Tetris3D::Tetromino::Shape::operator[](int n)
{
	return voxels[n];
}
const vec3d<char>& Tetris3D::Tetromino::Shape::operator[](int n) const
{
	return voxels[n];
}
Tetris3D::Tetromino::Shape& Tetris3D::Tetromino::Shape::operator=(const Shape& rhs)
{
	for (int i = 0; i < 4; i++)
		voxels[i] = rhs[i];
	return *this;
}

void Tetris3D::Tetromino::Reset()
{
	shape = Tetromino::shapes[id];
	mesh = Tetromino::meshes[id];
}
void Tetris3D::Tetromino::RotateX(int quads)
{
	if (quads % 4)
	{
		ort_angle.x = (ort_angle.x + (quads > 0 ? quads : 4 - quads % 4)) % 4;
		shape.RotateX(quads);
		auto mat = Mat4x4_RotateX(pi * 0.5f * (float)quads);
		for (auto& v : mesh.verticies)
		{
			v = mat * v.to<float>();
			v.x = std::round(v.x);
			v.y = std::round(v.y);
			v.z = std::round(v.z);
		}
	}
}
void Tetris3D::Tetromino::RotateY(int quads)
{
	if (quads % 4)
	{
		ort_angle.y = (ort_angle.y + (quads > 0 ? quads : 4 - quads % 4)) % 4;
		shape.RotateY(quads);
		auto mat = Mat4x4_RotateY(pi * 0.5f * (float)quads);
		for (auto& v : mesh.verticies)
		{
			v = mat * v.to<float>();
			v.x = std::round(v.x);
			v.y = std::round(v.y);
			v.z = std::round(v.z);
		}
	}
}
void Tetris3D::Tetromino::RotateZ(int quads)
{
	if (quads % 4)
	{
		ort_angle.z = (ort_angle.z + (quads > 0 ? quads : 4 - quads % 4)) % 4;
		shape.RotateZ(quads);
		auto mat = Mat4x4_RotateZ(pi * 0.5f * (float)quads);
		for (auto& v : mesh.verticies)
		{
			v = mat * v.to<float>();
			v.x = std::round(v.x);
			v.y = std::round(v.y);
			v.z = std::round(v.z);
		}
	}
}
Tetris3D::Mesh Tetris3D::Tetromino::GetMesh() const
{
	Mesh tetromino = mesh;
	for (auto& vx : tetromino.verticies) vx += pos;

	return tetromino;
}
Tetris3D::Mesh Tetris3D::Tetromino::GetGhostMesh(const PlayField& play_field) const
{
	Mesh ghost = mesh;
	int y = pos.y;
	for (; !(play_field.TestTetromino(shape, { pos.x,y - 1,pos.z }) & PlayField::COLLISION); y--);
	for (auto& vx : ghost.verticies) vx += vec3d<int>{pos.x, y, pos.z};
	for (auto& geo : ghost.geometries)
	{
		geo.reset(geo->NewCopy());
		dynamic_cast<Plane*>(geo.get())->fill_color.a = 0.5f;
		dynamic_cast<Plane*>(geo.get())->outline_thickness = 0.0f;
	}
	return ghost;
}
Tetris3D::Mesh Tetris3D::Tetromino::GetShadowMesh(const PlayField& play_field) const
{
	const auto& pf_voxels = play_field.GetVoxels();
	const auto& pf_dim = play_field.dim;
	auto vfdim = pf_dim + 1; //vertex field dimension
	auto key_encoder = [&vfdim](const vec3d<int>& pos) -> int
	{
		return pos.x + pos.z * vfdim.x + pos.y * vfdim.x * vfdim.z;
	};
	auto key_decoder = [&vfdim](int key) -> vec3d<int>
	{
		return
		{
			key % (vfdim.x),
			key / (vfdim.x * vfdim.z),
			(key % (vfdim.x * vfdim.z)) / vfdim.x
		};
	};

	Mesh shadows;
	std::vector<vec2d<char>> vec;
	Plane plane;
	plane.vx_ids.resize(4);
	plane.fill_color = D2D1::ColorF(colors[id][0], 0.4f);

	//z axis shadows
	for (const auto& a : shape.voxels)
	{
		auto c = pos + a;
		if (c.y >= 0 && c.y < pf_dim.y && std::find(vec.begin(), vec.end(), vec2d<char>{ a.x,a.y }) == vec.end())
		{
			vec.push_back({ a.x,a.y });
			int z;
			//back wall
			for (
				z = c.z + 1;
				z < pf_dim.z && pf_voxels[c.x + z * pf_dim.x + c.y * pf_dim.x * pf_dim.z] == 0;
				z++
				);

			vec3d<int> vx = { c.x, c.y, z };
			plane.vx_ids[0] = key_encoder(vx);
			vx.x++;
			plane.vx_ids[1] = key_encoder(vx);
			vx.y++;
			plane.vx_ids[2] = key_encoder(vx);
			vx.x--;
			plane.vx_ids[3] = key_encoder(vx);
			shadows.geometries.push_back(std::shared_ptr<Geometry>{ new Plane(plane) });

			//front wall
			for (
				z = c.z;
				z > 0 && pf_voxels[c.x + z * pf_dim.x + c.y * pf_dim.x * pf_dim.z] == 0;
				z--
				);
			vx.z = z;
			plane.vx_ids[0] = key_encoder(vx);
			vx.x++;
			plane.vx_ids[1] = key_encoder(vx);
			vx.y--;
			plane.vx_ids[2] = key_encoder(vx);
			vx.x--;
			plane.vx_ids[3] = key_encoder(vx);
			shadows.geometries.push_back(std::shared_ptr<Geometry>{ new Plane(plane) });
		}
	}

	//x axis shadows
	vec.clear();
	for (const auto& a : shape.voxels)
	{
		auto c = pos + a;
		if (c.y >= 0 && c.y < pf_dim.y && std::find(vec.begin(), vec.end(), vec2d<char>{ a.z, a.y }) == vec.end())
		{
			vec.push_back({ a.z,a.y });
			int x;

			//right wall
			for (
				x = c.x + 1;
				x < pf_dim.x && pf_voxels[x + c.z * pf_dim.x + c.y * pf_dim.x * pf_dim.z] == 0;
				x++
				);
			vec3d<int> vx = { x, c.y, c.z };
			plane.vx_ids[0] = key_encoder(vx);
			vx.y++;
			plane.vx_ids[1] = key_encoder(vx);
			vx.z++;
			plane.vx_ids[2] = key_encoder(vx);
			vx.y--;
			plane.vx_ids[3] = key_encoder(vx);
			shadows.geometries.push_back(std::shared_ptr<Geometry>{ new Plane(plane) });

			//left wall
			for (
				x = c.x;
				x >= 0 && pf_voxels[x + c.z * pf_dim.x + c.y * pf_dim.x * pf_dim.z] == 0;
				x--
				);
			vx.x = x + 1;
			plane.vx_ids[0] = key_encoder(vx);
			vx.y++;
			plane.vx_ids[1] = key_encoder(vx);
			vx.z--;
			plane.vx_ids[2] = key_encoder(vx);
			vx.y--;
			plane.vx_ids[3] = key_encoder(vx);
			shadows.geometries.push_back(std::shared_ptr<Geometry>{ new Plane(plane) });
		}
	}

	//y axis shadows
	vec.clear();
	for (const auto& a : shape.voxels)
	{
		if (std::find(vec.begin(), vec.end(), vec2d<char>{ a.x, a.z }) == vec.end())
		{
			vec.push_back({ a.x,a.z });
			auto c = pos + a;
			int y;
			//floor
			for (
				y = std::min(std::max(0, c.y), pf_dim.y - 1);
				y >= 0 && pf_voxels[c.x + c.z * pf_dim.x + y * pf_dim.x * pf_dim.z] == 0;
				y--
				);
			vec3d<int> vx = { a.x + pos.x, y + 1, a.z + pos.z };
			plane.vx_ids[0] = key_encoder(vx);
			vx.x++;
			plane.vx_ids[1] = key_encoder(vx);
			vx.z++;
			plane.vx_ids[2] = key_encoder(vx);
			vx.x--;
			plane.vx_ids[3] = key_encoder(vx);
			shadows.geometries.push_back(std::shared_ptr<Geometry>{ new Plane(plane) });
		}
	}

	std::unordered_map<int, int> vx_id_map;
	//create verticies and remap keys
	for (auto& plane : shadows.geometries)
	{
		for (auto& key : plane->vx_ids)
		{
			if (!vx_id_map.contains(key))
			{
				vx_id_map[key] = shadows.verticies.size();
				shadows.verticies.push_back(key_decoder(key).to<float>() + vec3d<float>{0.0f, 0.00001f, 0.0f});
			}
			key = vx_id_map[key];
		}
	}

	return shadows;
}
const Tetris3D::Tetromino::Shape& Tetris3D::Tetromino::GetShape() const
{
	return shape;
}


Tetris3D::PlayField::PlayField(vec3d<int> dim)
	:dim(dim)
{
	Resize(dim);
}
char Tetris3D::PlayField::TestTetromino(const Tetromino::Shape& shape, const vec3d<int>& pos) const
{
	char flags = 0;
	for (const auto& vox : shape.voxels)
	{
		auto p = pos + vox;
		bool bCollision = 
			p.x < 0 || p.x >= dim.x || p.z < 0 || p.z >= dim.z || p.y < 0 ||
			(p.y < dim.y && voxels[p.x + p.z * dim.x + p.y * dim.x * dim.z]);

		flags |= COLLISION * bCollision;

		flags |= OVER_ROOF * (p.y >= dim.y);
	}
	return flags;
}
int Tetris3D::PlayField::PutTetromino(int tetromino_id, const Tetromino::Shape& shape, const vec3d<int>& pos)
{
	//add voxels
	for (const auto& vox : shape.voxels)
	{
		auto p = pos + vox;
		voxels[p.x + p.z * dim.x + p.y * dim.x * dim.z] = tetromino_id + 1;
	}
	//check for full planes
	int nPlanes = 0;
	for (int y = 0; y < dim.y; y++)
	{
		bool bFullPlane = true;
		for (int z = 0; z < dim.z; z++)
		{
			for (int x = 0; x < dim.x; x++)
			{
				if (voxels[x + z * dim.x + y * dim.x * dim.z] == 0)
				{
					bFullPlane = false;
					break;
				}
			}
		}
		if (bFullPlane)
		{
			nPlanes++;
			for (vec3d<int> i = { 0,y,0 }; i.y < dim.y; i.y++)
			{
				for (i.z = 0; i.z < dim.z; i.z++)
				{
					for (i.x = 0; i.x < dim.x; i.x++)
					{
						voxels[i.x + i.z * dim.x + i.y * dim.x * dim.z] = 
							i.y != dim.y - 1 ? 
							voxels[i.x + i.z * dim.x + (i.y + 1) * dim.x * dim.z] : 0;
					}
				}
			}
			y--;
		}
	}

	RecreateVoxelsMesh();

	return nPlanes;
}
void Tetris3D::PlayField::Clear()
{
	for (int i = 0; i < dim.x * dim.z * dim.y; i++)
	{
		voxels[i] = 0;
	}
	mesh_voxels.verticies.clear();
	mesh_voxels.geometries.clear();
}
void Tetris3D::PlayField::Resize(vec3d<int> dim)
{
	voxels.resize(dim.x * dim.y * dim.z, 0);
	Clear();

	//create grid mesh
	auto vfdim = dim + 1; //vertex field dimension
	auto key_encoder = [&vfdim](const vec3d<int>& pos) -> int
	{
		return pos.x + pos.z * vfdim.x + pos.y * vfdim.x * vfdim.z;
	};
	auto key_decoder = [&vfdim](int key) -> vec3d<int>
	{
		return
		{
			key % (vfdim.x),
			key / (vfdim.x * vfdim.z),
			(key % (vfdim.x * vfdim.z)) / vfdim.x
		};
	};

	//create geometries
	Lines lines;
	lines.color = D2D1::ColorF(0xa0a0a0);
	lines.thickness = 1.2f;
	//left and right
	{
		//center
		lines.vx_ids.resize(3);
		for (vec3d<int> i = { 0,0,0 }; i.y < dim.y - 1; i.y++)
		{
			for (i.z = 0; i.z < dim.z - 1; i.z++)
			{
				auto a = i;
				//left
				a.y++;
				lines.vx_ids[0] = key_encoder(a);
				a.y--;
				lines.vx_ids[1] = key_encoder(a);
				a.z++;
				lines.vx_ids[2] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
				//right
				a.x += dim.x;
				lines.vx_ids[0] = key_encoder(a);
				a.z--;
				lines.vx_ids[1] = key_encoder(a);
				a.y++;
				lines.vx_ids[2] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
			}
		}

		//last row
		lines.vx_ids.resize(4);
		for (vec3d<int> i = { 0,dim.y - 1,0 }; i.y < dim.y; i.y++)
		{
			for (i.z = 0; i.z < dim.z - 1; i.z++)
			{
				auto a = i;
				//left
				a.y++; a.z++;
				lines.vx_ids[0] = key_encoder(a);
				a.z--;
				lines.vx_ids[1] = key_encoder(a);
				a.y--;
				lines.vx_ids[2] = key_encoder(a);
				a.z++;
				lines.vx_ids[3] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});

				//right
				a.x += dim.x;
				lines.vx_ids[0] = key_encoder(a);
				a.z--;
				lines.vx_ids[1] = key_encoder(a);
				a.y++;
				lines.vx_ids[2] = key_encoder(a);
				a.z++;
				lines.vx_ids[3] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
			}
		}
		//last column
		for (vec3d<int> i = { 0,0,0 }; i.y < dim.y - 1; i.y++)
		{
			for (i.z = dim.z - 1; i.z < dim.z; i.z++)
			{
				auto a = i;
				//left
				a.y++;
				lines.vx_ids[0] = key_encoder(a);
				a.y--;
				lines.vx_ids[1] = key_encoder(a);
				a.z++;
				lines.vx_ids[2] = key_encoder(a);
				a.y++;
				lines.vx_ids[3] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});

				//right
				a.x += dim.x;
				lines.vx_ids[0] = key_encoder(a);
				a.y--;
				lines.vx_ids[1] = key_encoder(a);
				a.z--;
				lines.vx_ids[2] = key_encoder(a);
				a.y++;
				lines.vx_ids[3] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
			}
		}

		//last corner
		lines.vx_ids.resize(5);
		//left
		lines.vx_ids[0] = key_encoder({ 0, dim.y, dim.z });
		lines.vx_ids[1] = key_encoder({ 0, dim.y, dim.z - 1 });
		lines.vx_ids[2] = key_encoder({ 0, dim.y - 1, dim.z - 1 });
		lines.vx_ids[3] = key_encoder({ 0, dim.y - 1, dim.z });
		lines.vx_ids[4] = lines.vx_ids[0];
		mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});

		//right
		lines.vx_ids[0] = key_encoder({ dim.x, dim.y - 1, dim.z });
		lines.vx_ids[1] = key_encoder({ dim.x, dim.y - 1, dim.z - 1 });
		lines.vx_ids[2] = key_encoder({ dim.x, dim.y, dim.z - 1 });
		lines.vx_ids[3] = key_encoder({ dim.x, dim.y, dim.z });
		lines.vx_ids[4] = lines.vx_ids[0];
		mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
	};

	//back and front
	{
		//center
		lines.vx_ids.resize(3);
		for (vec3d<int> i = { 0,0,dim.z }; i.y < dim.y - 1; i.y++)
		{
			for (i.x = 0; i.x < dim.x - 1; i.x++)
			{
				auto a = i;
				//back
				a.y++;
				lines.vx_ids[0] = key_encoder(a);
				a.y--;
				lines.vx_ids[1] = key_encoder(a);
				a.x++;
				lines.vx_ids[2] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});

				//front
				a.z = 0;
				lines.vx_ids[0] = key_encoder(a);
				a.x--;
				lines.vx_ids[1] = key_encoder(a);
				a.y++;
				lines.vx_ids[2] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
			}
		}

		//last row
		lines.vx_ids.resize(4);
		for (vec3d<int> i = { 0,dim.y,dim.z }; i.x < dim.x - 1; i.x++)
		{
			auto a = i;
			//back
			a.x++;
			lines.vx_ids[0] = key_encoder(a);
			a.x--;
			lines.vx_ids[1] = key_encoder(a);
			a.y--;
			lines.vx_ids[2] = key_encoder(a);
			a.x++;
			lines.vx_ids[3] = key_encoder(a);
			mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});

			//front
			a.z = 0;
			lines.vx_ids[0] = key_encoder(a);
			a.x--;
			lines.vx_ids[1] = key_encoder(a);
			a.y++;
			lines.vx_ids[2] = key_encoder(a);
			a.x++;
			lines.vx_ids[3] = key_encoder(a);
			mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
		}
		//last column
		for (vec3d<int> i = { dim.x - 1,0,dim.z }; i.y < dim.y - 1; i.y++)
		{
			auto a = i;
			//back
			a.y++;
			lines.vx_ids[0] = key_encoder(a);
			a.y--;
			lines.vx_ids[1] = key_encoder(a);
			a.x++;
			lines.vx_ids[2] = key_encoder(a);
			a.y++;
			lines.vx_ids[3] = key_encoder(a);
			mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});

			//front
			a.z = 0;
			lines.vx_ids[0] = key_encoder(a);
			a.y--;
			lines.vx_ids[1] = key_encoder(a);
			a.x--;
			lines.vx_ids[2] = key_encoder(a);
			a.y++;
			lines.vx_ids[3] = key_encoder(a);
			mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
		}

		//last corner
		lines.vx_ids.resize(5);
		//back
		lines.vx_ids[0] = key_encoder({ dim.x,dim.y,dim.z });
		lines.vx_ids[1] = key_encoder({ dim.x - 1,dim.y,dim.z });
		lines.vx_ids[2] = key_encoder({ dim.x - 1,dim.y - 1,dim.z });
		lines.vx_ids[3] = key_encoder({ dim.x,dim.y - 1,dim.z });
		lines.vx_ids[4] = lines.vx_ids[0];
		mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
		//front
		lines.vx_ids[0] = key_encoder({ dim.x,dim.y - 1,0 });
		lines.vx_ids[1] = key_encoder({ dim.x - 1,dim.y - 1,0 });
		lines.vx_ids[2] = key_encoder({ dim.x - 1,dim.y,0 });
		lines.vx_ids[3] = key_encoder({ dim.x,dim.y,0 });
		lines.vx_ids[4] = lines.vx_ids[0];
		mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
	};

	//bottom
	{
		//center
		lines.vx_ids.resize(3);
		for (vec3d<int> i = { 0,0,0 }; i.z < dim.z - 1; i.z++)
		{
			for (i.x = 0; i.x < dim.x - 1; i.x++)
			{
				auto a = i;
				a.z++;
				lines.vx_ids[0] = key_encoder(a);
				a.z--;
				lines.vx_ids[1] = key_encoder(a);
				a.x++;
				lines.vx_ids[2] = key_encoder(a);
				mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
			}
		}

		//last row
		lines.vx_ids.resize(4);
		for (vec3d<int> i = { 0,0,dim.z }; i.x < dim.x - 1; i.x++)
		{
			auto a = i;
			a.x++;
			lines.vx_ids[0] = key_encoder(a);
			a.x--;
			lines.vx_ids[1] = key_encoder(a);
			a.z--;
			lines.vx_ids[2] = key_encoder(a);
			a.x++;
			lines.vx_ids[3] = key_encoder(a);
			mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
		}
		//last column
		for (vec3d<int> i = { dim.x - 1,0,0 }; i.z < dim.z - 1; i.z++)
		{
			auto a = i;
			a.z++;
			lines.vx_ids[0] = key_encoder(a);
			a.z--;
			lines.vx_ids[1] = key_encoder(a);
			a.x++;
			lines.vx_ids[2] = key_encoder(a);
			a.z++;
			lines.vx_ids[3] = key_encoder(a);
			mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
		}

		//last corner
		lines.vx_ids.resize(5);
		lines.vx_ids[0] = key_encoder({ dim.x,0,dim.z });
		lines.vx_ids[1] = key_encoder({ dim.x - 1,0,dim.z });
		lines.vx_ids[2] = key_encoder({ dim.x - 1,0,dim.z - 1 });
		lines.vx_ids[3] = key_encoder({ dim.x,0,dim.z - 1 });
		lines.vx_ids[4] = lines.vx_ids[0];
		mesh_grid.geometries.push_back(std::shared_ptr<Geometry>{new Lines(lines)});
	};

	//create verticies and remap keys
	std::unordered_map<int, int> vx_id_map;
	for (auto geo : mesh_grid.geometries)
	{
		for (int& vx_key : geo->vx_ids)
		{
			if (!vx_id_map.contains(vx_key))
			{
				vx_id_map[vx_key] = mesh_grid.verticies.size();
				mesh_grid.verticies.push_back(key_decoder(vx_key).to<float>());
			}
			//remap key
			vx_key = vx_id_map[vx_key];
		}
	}
}
void Tetris3D::PlayField::RecreateVoxelsMesh()
{
	mesh_voxels.geometries.clear();
	mesh_voxels.verticies.clear();

	auto vfdim = dim + 1; //vertex field dimension
	auto key_encoder = [&vfdim](const vec3d<int>& pos) -> int
	{
		return pos.x + pos.z * vfdim.x + pos.y * vfdim.x * vfdim.z;
	};
	auto key_decoder = [&vfdim](int key) -> vec3d<int>
	{
		return
		{
			key % (vfdim.x),
			key / (vfdim.x * vfdim.z),
			(key % (vfdim.x * vfdim.z)) / vfdim.x
		};
	};

	//create planes
	Plane plane;
	plane.vx_ids.resize(4);
	plane.outline_thickness = 1.8f;
	plane.fill_color = D2D1::ColorF(0xd4d4d4);
	plane.outline_color = D2D1::ColorF(0x969696);
	for (vec3d<int> i = { 0,0,0 }; i.y < dim.y; i.y++)
	{
		for (i.z = 0; i.z < dim.z; i.z++)
		{
			for (i.x = 0; i.x < dim.x; i.x++)
			{
				int j = i.x + i.z * dim.x + i.y * dim.x * dim.z;
				if (voxels[j])
				{
					//plane.fill_color = D2D1::ColorF(Tetromino::colors[voxels[j] - 1][0]);
					//plane.outline_color = D2D1::ColorF(Tetromino::colors[voxels[j] - 1][1]);
					//create planes
					//temporarely plane.vertex_id is actually a key to the to-be-created vertex id
					if (i.x == 0 || voxels[j - 1] == 0) //if there is no neighbour to the left then add plane
					{
						plane.vx_ids[0] = key_encoder(i + vec3d<int>{0, 0, 0});
						plane.vx_ids[1] = key_encoder(i + vec3d<int>{0, 1, 0});
						plane.vx_ids[2] = key_encoder(i + vec3d<int>{0, 1, 1});
						plane.vx_ids[3] = key_encoder(i + vec3d<int>{0, 0, 1});
						mesh_voxels.geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});
					}
					if (i.x == dim.x - 1 || voxels[j + 1] == 0) //if there is no neighbour to the right then add plane
					{
						plane.vx_ids[0] = key_encoder(i + vec3d<int>{1, 0, 0});
						plane.vx_ids[1] = key_encoder(i + vec3d<int>{1, 0, 1});
						plane.vx_ids[2] = key_encoder(i + vec3d<int>{1, 1, 1});
						plane.vx_ids[3] = key_encoder(i + vec3d<int>{1, 1, 0});
						mesh_voxels.geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});
					}
					if (i.z == 0 || voxels[j - dim.x] == 0) //if there is no neighbour in front then add plane
					{
						plane.vx_ids[0] = key_encoder(i + vec3d<int>{0, 0, 0});
						plane.vx_ids[1] = key_encoder(i + vec3d<int>{1, 0, 0});
						plane.vx_ids[2] = key_encoder(i + vec3d<int>{1, 1, 0});
						plane.vx_ids[3] = key_encoder(i + vec3d<int>{0, 1, 0});
						mesh_voxels.geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});
					}
					if (i.z == dim.z - 1 || voxels[j + dim.x] == 0) //if there is no neighbour behind then add plane
					{
						plane.vx_ids[0] = key_encoder(i + vec3d<int>{0, 0, 1});
						plane.vx_ids[1] = key_encoder(i + vec3d<int>{0, 1, 1});
						plane.vx_ids[2] = key_encoder(i + vec3d<int>{1, 1, 1});
						plane.vx_ids[3] = key_encoder(i + vec3d<int>{1, 0, 1});
						mesh_voxels.geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});
					}
					if (i.y == 0 || voxels[j - dim.x * dim.z] == 0) //if there is no neighbour bellow then add plane
					{
						plane.vx_ids[0] = key_encoder(i + vec3d<int>{0, 0, 0});
						plane.vx_ids[1] = key_encoder(i + vec3d<int>{0, 0, 1});
						plane.vx_ids[2] = key_encoder(i + vec3d<int>{1, 0, 1});
						plane.vx_ids[3] = key_encoder(i + vec3d<int>{1, 0, 0});
						mesh_voxels.geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});
					}
					if (i.y == dim.y - 1 || voxels[j + dim.x * dim.z] == 0) //if there is no neighbour on top then add plane
					{
						plane.vx_ids[0] = key_encoder(i + vec3d<int>{0, 1, 0});
						plane.vx_ids[1] = key_encoder(i + vec3d<int>{1, 1, 0});
						plane.vx_ids[2] = key_encoder(i + vec3d<int>{1, 1, 1});
						plane.vx_ids[3] = key_encoder(i + vec3d<int>{0, 1, 1});
						mesh_voxels.geometries.push_back(std::shared_ptr<Geometry>{new Plane(plane)});
					}
				}
			}
		}
	}
	//create verticies and remap keys
	std::unordered_map<int, int> vx_id_map;
	for (auto geo : mesh_voxels.geometries)
	{
		for (int& vx_key : geo->vx_ids)
		{
			if (!vx_id_map.contains(vx_key))
			{
				vx_id_map[vx_key] = mesh_voxels.verticies.size();
				mesh_voxels.verticies.push_back(key_decoder(vx_key).to<float>());
			}
			vx_key = vx_id_map[vx_key];
		}
	}
}
Matrix<4,4> Tetris3D::PlayField::Transform() const
{
	return
		Mat4x4_Translate(pos + 0.5f * vec3d<float>{(float)dim.x, 3.5f, (float)dim.z})*
		Mat4x4_RotateZ(angle.z) * 
		Mat4x4_RotateX(angle.x) *
		Mat4x4_RotateY(angle.y) * 
		Mat4x4_Translate(-0.5f * vec3d<float>{(float)dim.x, 4.0f, (float)dim.z});
}
const Tetris3D::Mesh& Tetris3D::PlayField::GetMeshVoxels() const
{
	return mesh_voxels;
}
const Tetris3D::Mesh& Tetris3D::PlayField::GetMeshGrid() const
{
	return mesh_grid;
}
const std::vector<char>& Tetris3D::PlayField::GetVoxels() const
{
	return voxels;
}
vec2d<char> Tetris3D::PlayField::UnVecY() const
{
	float angle_y = std::roundf(angle.y * 2.0f / pi) * pi / 2.0f;
	return { (char)cosf(angle_y),(char)sinf(angle_y) };
}