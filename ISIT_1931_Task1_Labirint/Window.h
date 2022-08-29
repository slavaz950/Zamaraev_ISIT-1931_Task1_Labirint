/*
 * Window.h
 *
 *  Created on: 28 авг. 2022 г.
 *      Author: Zver
 */

#ifndef WINDOW_H_
#define WINDOW_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_image.h>
#include <utility>

#include "Map.h"
#include "Player.h"

class Window final
{
	private:
		static constexpr int CELL_SIZE = 8;
		static constexpr int MAP_OFFSET = 20;

		static constexpr int PLAYER_SIZE = 5;
		static constexpr int PLAYER_ARROW = CELL_SIZE / 3;
		static constexpr double PLAYER_WALK_DIST = 0.01;
		static constexpr double PLAYER_TURN_ANGLE = 0.01;

		static constexpr double WALL_HEIGHT = 0.5;
		static constexpr double FOV = 90.0;

		int _width, _height;
		std::shared_ptr<SDL_Window> _window;
		std::shared_ptr<SDL_Renderer> _renderer;
		std::shared_ptr<Map> _map;
		Player _player;

		std::shared_ptr<SDL_Texture> _wall_tex;
		int _wall_tex_width, _wall_tex_height;

		std::shared_ptr<SDL_Texture> _door_tex;
		int _door_tex_width, _door_tex_height;

		struct
		{
			bool walk_forward 	{ false };
			bool walk_back 		{ false };
			bool shift_right 	{ false };
			bool shift_left 	{ false };
			bool turn_right 	{ false };
			bool turn_left 		{ false };

		} _controls;

		void create_window();
		void create_renderer();
		void draw_minimap(SDL_Renderer *r);

		void draw_col(SDL_Renderer *r, SDL_Texture *t,
				int col, int h, int tw, int th, double tx);

		void cast_ray(double &rx, double &ry, double dx, double dy);
		std::shared_ptr<SDL_Texture> load_texture(const char *texname);

	public:
		Window(int w, int h);

		void main_loop();
		void render(SDL_Renderer * r);
		void event(const SDL_Event & e);
		void update();
};

#endif /* WINDOW_H_ */

