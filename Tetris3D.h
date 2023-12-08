#pragma once
#include <ext_matrix.h>
#include <ext_vec3d.h>
#include <guipp.h>
#include <guipp_label.h>
#include <guipp_matrix.h>
#include <d2d1.h>
#include <functional>
#include <unordered_map>
#include <memory>


class Tetris3D : public guipp::Object, private guipp::Updatable
{
	static constexpr float pi = 3.14159f;
public:
	enum class EVENT
	{
		PAUSE,
		GAME_OVER
	};
	Tetris3D(const std::wstring& font_name, ext::vec3d<int> dim, std::function<void(EVENT)> OnEvent);
	~Tetris3D();

	void Resize(ext::vec3d<int> dim);
	void Reset();
	void Resume(guipp::Window& wnd);
	void Tutorial(guipp::Window& wnd);
	std::function<void(EVENT)> OnEvent;

private:
	class NextDisplay : public guipp::Object
	{
	public:
		NextDisplay() { Get(); };
		int Get();
		void Update(float fElapsedTime);
	private:
		void OnDraw(ext::D2DGraphics& gfx) override;
		static ext::vec3d<float> tetro_pivot[8];
		ext::vec3d<float> angle = { -pi / 5.0f, 0.0f, 0.0f };
		int next = 0;
	};
	std::shared_ptr<NextDisplay> next_display;

	class ProgressBar : public guipp::Object
	{
	public:
		ProgressBar(const std::wstring& font_name);
		void Update(int nLevel, float fProgress);
	private:
		void OnDraw(ext::D2DGraphics& gfx) override;
		ext::vec2d<float> OnMinSizeUpdate() override;
		void OnSetPos() override;
		std::shared_ptr<guipp::Label> lb1, lb2;
		std::shared_ptr<guipp::Matrix> mat;
		float fProgress = 0.0f;
	};
	std::shared_ptr<ProgressBar> progress_bar;

	std::shared_ptr<guipp::Label> lb_score, lb_best;
	std::shared_ptr<guipp::Matrix> mat_stats, mat_next;

public:
	std::shared_ptr<guipp::Object> GetNextDisplay() { return mat_next; }
	std::shared_ptr<guipp::Object> GetProgressBar() { return progress_bar; }
	std::shared_ptr<guipp::Object> GetStats() { return mat_stats; }

private:
	void OnDraw(ext::D2DGraphics& gfx) override;
	bool OnUpdate(guipp::Window& wnd, float fElapsedTime) override;
	guipp::Object* OnKbdFocus(guipp::Window& wnd, bool bFirst) override { return this; }
	bool OnKbdMessage(guipp::Window& wnd, UINT msg, unsigned key_code, LPARAM lParam) { return true; }
	void OnSetSize() override;
	void OnSetPos() override;
	ext::vec2d<float> OnMinSizeUpdate() override;

	ext::vec2d<float> center;
	float fScale = 1.0f;

	ext::TextFormat font, font_small;

	struct InputKey
	{
		static constexpr float fAutoRepeatBeginThreshold = 0.3f;
		static constexpr float fAutoRepeatThreshold = 0.1f;
		float fTimer1 = 0.0f, fTimer2 = 0.0f;
		bool bPressed = false, bHeld = false, bAutoRepeat = false, bReleased = false;
	};
	enum KEY_NAME { 
		KN_RESET,
		KN_PAUSE,
		KN_SHOW_GHOST,
		KN_ROTATE_CW, 
		KN_ROTATE_CCW,
		KN_ROTATE_YCW,
		KN_ROTATE_YCCW,
		KN_PUSH,
		KN_PULL,
		KN_RIGHT, 
		KN_LEFT, 
		KN_DOWN,
		KN_SPACE,
		KN_END
	};
	std::unordered_map<KEY_NAME, int> key_codes =
	{
		{ KN_RESET         , 'R'    },
		{ KN_PAUSE         , 'P'    },
		{ KN_SHOW_GHOST    , VK_TAB },
		{ KN_ROTATE_CW     , 'E'    },
		{ KN_ROTATE_CCW    , 'Q'    },
		{ KN_ROTATE_YCW    , 'C'    },
		{ KN_ROTATE_YCCW   , 'Z'    },
		{ KN_PUSH          , 'W'    },
		{ KN_PULL          , 'S'    },
		{ KN_RIGHT         , 'D'    },
		{ KN_LEFT          , 'A'    },
		{ KN_DOWN          , 'X'    },
		{ KN_SPACE         , ' '    }
	};
	std::unordered_map<KEY_NAME, InputKey> keys;

	bool bTutorial = false;
	int nTutorialStage = 0;
	int nTutorialInts[10] = { 0 };
	float fTutorialTimers[4] = { 0 };

	bool bShowGhost = false;
	float fGravityTimer = 0.0f;
	float fGravitySpeed = 1.0f;
	struct GameStats
	{
		int nScore = 0, nTetrominos = 0;
		int GetLevel() { return nTetrominos / 10 + 1; }
		bool IsLevelUp() { return !(nTetrominos % 10); }
	}cur_game, best_game;

private:
	static ext::vec3d<float> light_source;

	struct Geometry
	{
		virtual Geometry* NewCopy() const = 0;
		virtual void Draw(ext::D2DGraphics& gfx, const std::vector<ext::vec3d<float>>& vx_pool) const = 0;
		bool Cull(const std::vector<ext::vec3d<float>>& vx_pool) const;
		float fLightIntensity = 1.0f;
		std::vector<int> vx_ids;
	};
	struct Plane : public Geometry
	{
		Plane* NewCopy() const override;
		void Draw(ext::D2DGraphics& gfx, const std::vector<ext::vec3d<float>>& vx_pool) const override;
		D2D1_COLOR_F fill_color, outline_color;
		float outline_thickness;
	};
	struct Lines : public Geometry
	{
		Lines* NewCopy() const override;
		void Draw(ext::D2DGraphics& gfx, const std::vector<ext::vec3d<float>>& vx_pool) const override;
		D2D1_COLOR_F color;
		float thickness;
	};
	struct Mesh
	{
		Mesh operator+(Mesh rhs) const;
		void SortGeometriesByZ();
		std::vector<std::shared_ptr<Geometry>> geometries;
		std::vector<ext::vec3d<float>> verticies;
	};

	class PlayField;
	struct Tetromino
	{
		struct Shape
		{
			void RotateX(int quads);
			void RotateY(int quads);
			void RotateZ(int quads);
			ext::vec3d<char>& operator[](int n);
			const ext::vec3d<char>& operator[](int n) const;
			Shape& operator=(const Shape& rhs);
			ext::vec3d<char> voxels[4];
		};
		const static unsigned colors[8][2];
		static const Shape shapes[8];
		static Mesh meshes[8];

		void Reset();

		void RotateX(int quads);
		void RotateY(int quads);
		void RotateZ(int quads);

		//PlayField& to draw the tetromino's final position
		Mesh GetMesh() const;
		Mesh GetGhostMesh(const PlayField& play_field) const;
		Mesh GetShadowMesh(const PlayField& play_field) const;
		const Shape& GetShape() const;

		int id = 0;
		ext::vec3d<int> pos = { 0 };

	private:
		Shape shape;
		Mesh mesh;
		ext::vec3d<int> ort_angle = { 0 };
	}tetromino;

	class PlayField
	{
	public:
		PlayField(ext::vec3d<int> dim);

		enum { COLLISION = 0b1, OVER_ROOF = 0b10 };
		char TestTetromino(const Tetromino::Shape& shape, const ext::vec3d<int>& pos) const;
		int PutTetromino(int tetromino_id, const Tetromino::Shape& shape, const ext::vec3d<int>& pos);
		void Clear();
		void Resize(ext::vec3d<int> dim);

		ext::Matrix<4, 4> Transform() const;
		const Mesh& GetMeshVoxels() const;
		const Mesh& GetMeshGrid() const;
		const std::vector<char>& GetVoxels() const;

		ext::vec2d<char> UnVecY() const;

		ext::vec3d<float> pos = { 0 }, angle = { 0 };
		const ext::vec3d<int> dim;
	private:
		Mesh mesh_voxels, mesh_grid;

		std::vector<char> voxels;

		void RecreateVoxelsMesh();
	}play_field;

	void NewTetromino();
};