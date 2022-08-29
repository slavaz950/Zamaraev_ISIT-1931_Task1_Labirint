/*
 * Window.cpp
 *
 *  Created on: 28 авг. 2022 г.
 *      Author: Zver
 */

#include <stdexcept>
#include <thread>
#include <chrono>

#include "Window.h"

static constexpr double Pi = acos(-1.0);

void Window::create_window() {
	_window = std::shared_ptr<SDL_Window>(
			SDL_CreateWindow(
					"Main_Window",
					SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED,
					_width, _height, 0),
			SDL_DestroyWindow);
	if (_window == nullptr)
		throw std::runtime_error(
				std::string("Не могу создать окно: ") +
				std::string(SDL_GetError()));
}

void Window::create_renderer() {
	_renderer = std::shared_ptr<SDL_Renderer>(
			SDL_CreateRenderer(_window.get(), -1,
					SDL_RENDERER_ACCELERATED |
					SDL_RENDERER_PRESENTVSYNC),
			SDL_DestroyRenderer);
	if (_renderer == nullptr)
		throw std::runtime_error(
				std::string("Не могу создать рендерер: ") +
				std::string(SDL_GetError()));
}

std::shared_ptr<SDL_Texture> Window::load_texture(const char *texname)
{
	std::shared_ptr<SDL_Texture> result;

	result = std::shared_ptr<SDL_Texture>(
			IMG_LoadTexture(_renderer.get(), texname),
					SDL_DestroyTexture);

	if (result == nullptr)
		throw std::runtime_error(
				std::string("Не могу загрузить картинку: ") +
				std::string(SDL_GetError()));

	return result;
}

Window::Window(int w, int h)
: _width(w), _height(h)
{
	create_window();
	create_renderer();

	_wall_tex = load_texture("wall.jpg");
	_door_tex= load_texture("door.jpg");

	SDL_QueryTexture(
			_wall_tex.get(), nullptr, nullptr,
			&_wall_tex_width, &_wall_tex_height);
	SDL_QueryTexture(
			_door_tex.get(), nullptr, nullptr,
			&_door_tex_width, &_door_tex_height);

	_map = std::make_shared<Map>("map1.txt");
	_player.spawn(_map);
}

void Window::main_loop()
{
	bool want_quit { false };
	SDL_Event e;

	auto update_thread = std::thread([&]() {

		using clk = std::chrono::high_resolution_clock;
		auto delay = std::chrono::microseconds(1'000'000 / 60);
		auto next_time = clk::now() + delay;

		while(not want_quit)
		{
			std::this_thread::sleep_until(next_time);
			next_time += delay;
			update();
		}
		});

	for(;;)
	{
		while (SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT)
			{
				want_quit = true;
				update_thread.join();
				return;
			}
			event(e);
		}

		update();

		render(_renderer.get());
		SDL_RenderPresent(_renderer.get());
	}
}

void Window::draw_minimap(SDL_Renderer *r)
{
	SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
	for (int y = 0; y < _map->height(); ++y) {
		for (int x = 0; x < _map->width(); ++x) {
			SDL_Rect cr { MAP_OFFSET + x * CELL_SIZE, MAP_OFFSET
					+ y * CELL_SIZE, CELL_SIZE, CELL_SIZE };
			if (_map->is_wall(x, y) && _map->get(x, y) == '@')
			{
				SDL_SetRenderDrawColor(r, 250, 250, 250, 255);
				SDL_RenderFillRect(r, &cr);
				SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
			}
			else if(_map->is_wall(x, y))
				SDL_RenderFillRect(r, &cr);
		}
	}
	int x1, y1, x2, y2;
	x1 = MAP_OFFSET + CELL_SIZE * _player.x();
	y1 = MAP_OFFSET + CELL_SIZE * _player.y();
	x2 = x1 + PLAYER_ARROW * cos(_player.dir());
	y2 = y1 + PLAYER_ARROW * sin(_player.dir());
	SDL_Rect pr { x1 - PLAYER_SIZE / 2, y1 - PLAYER_SIZE / 2, PLAYER_SIZE,
			PLAYER_SIZE };
	SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
	SDL_RenderFillRect(r, &pr);
	SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
	SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

void Window::render(SDL_Renderer *r)
{
	//Потолок
	SDL_Rect cr { 0, 0, _width, _height / 2 };
	//Пол
	SDL_Rect fr { 0, _height / 2, _width, _height / 2 };

	SDL_SetRenderDrawColor(r, 62, 155, 220, 99);
	SDL_RenderFillRect(r, &cr);
	SDL_SetRenderDrawColor(r, 112, 83, 53, 1);
	SDL_RenderFillRect(r, &fr);

	//Стены
	SDL_SetRenderDrawColor(r, 60, 60, 240, 255);

	double fov = FOV * Pi / 180.0;
	double ds = _width / 2.0 / tan(fov / 2.0); //расстояние до экрана
	double px = _player.x();
	double py = _player.y();
	double alpha = _player.dir();
	double eps = 0.00001;

	for (int col = 0; col < _width; ++col)
	{
		double gamma = atan2(col - _width / 2, ds);
		double beta = alpha + gamma;

		double rx, ry, dx, dy;
		double d, dv, dh; //d - расстояние до стены
		double tv, th, tx;
		char cv, ch, c;

		if(sin(beta) > eps)
		{
			dy = 1.0;
			dx = 1.0 / tan(beta);
			ry = floor(py) + eps;
			rx = px - (py - ry) * dx;
			cast_ray(rx, ry, dx, dy);
			dh = hypot(rx-px, ry-py);
			th = 1.0 - (rx-floor(rx));
			ch = _map->get(rx, ry);
		} else if (sin(beta) < -eps)
		{
			dy = -1.0;
			dx = 1.0 / tan(-beta);
			ry = ceil(py) - eps;
			rx = px - (ry - py) * dx;
			cast_ray(rx, ry, dx, dy);
			dh = hypot(rx-px, ry-py);
			th = rx-floor(rx);
			ch = _map->get(rx, ry);
		} else
		{
			dh = INFINITY;
		}

		if(cos(beta) > eps)
		{
			dx = 1.0;
			dy = tan(beta);
			rx = floor(px) + eps;
			ry = py - (px - rx) * dy;
			cast_ray(rx, ry, dx, dy);
			dv = hypot(rx-px, ry-py);
			tv = ry-floor(ry);
			cv = _map->get(rx, ry);
		} else if (cos(beta) < -eps)
		{
			dx = -1.0;
			dy = tan(-beta);
			rx = ceil(px) - eps;
			ry = py - (rx - px) * dy;
			cast_ray(rx, ry, dx, dy);
			dv = hypot(rx-px, ry-py);
			tv = 1.0 - (ry-floor(ry));
			cv = _map->get(rx, ry);
		} else
		{
			dv = INFINITY;
		}

		if(dv < dh) { d = dv; tx = tv; c = cv;}
		else 		{ d = dh; tx = th; c = ch;}

		int h = WALL_HEIGHT * ds / d / cos(gamma); //видимая высота стены

		if(c == '@')
		{
			draw_col(r, _door_tex.get(), col, h,
							_door_tex_width, _door_tex_height, tx);
		}
		else
		{
			draw_col(r, _wall_tex.get(), col, h,
					_wall_tex_width, _wall_tex_height, tx);
		}
	}

	draw_minimap(r);
}

void Window::event(const SDL_Event &e)
{
	switch(e.type)
	{
	case SDL_KEYDOWN:
		switch(e.key.keysym.scancode)
		{
		case SDL_SCANCODE_W: _controls.walk_forward = true; break;
		case SDL_SCANCODE_S: _controls.walk_back 	= true; break;
		case SDL_SCANCODE_A: _controls.shift_left 	= true; break;
		case SDL_SCANCODE_D: _controls.shift_right 	= true; break;

		case SDL_SCANCODE_E: _controls.turn_right 	= true; break;
		case SDL_SCANCODE_Q: _controls.turn_left 	= true; break;

		default:;
		}
		break;
	case SDL_KEYUP:
		switch(e.key.keysym.scancode)
		{
		case SDL_SCANCODE_W: _controls.walk_forward = false; break;
		case SDL_SCANCODE_S: _controls.walk_back 	= false; break;
		case SDL_SCANCODE_A: _controls.shift_left 	= false; break;
		case SDL_SCANCODE_D: _controls.shift_right 	= false; break;

		case SDL_SCANCODE_E: _controls.turn_right 	= false; break;
		case SDL_SCANCODE_Q: _controls.turn_left 	= false; break;

		default:;
		}
		break;
	default:;
	}
}

void Window::cast_ray(double &rx, double &ry, double dx, double dy)
{
	int limit = std::max(_map->width(), _map->height()) * 3;
	double mx, my;
	do {
		rx += dx;
		ry += dy;
		mx = rx;
		my = ry;
		while(mx >= _map->width()) mx -= _map->width();
		while(mx < 0) mx += _map->width();
		while(my >= _map->height()) my -= _map->height();
		while(my < 0) my += _map->height();
		--limit;
	} while (not _map->is_wall(mx, my) && limit);
}

void Window::draw_col(SDL_Renderer *r, SDL_Texture *t, int col, int h, int tw, int th, double tx)
{
	SDL_Rect src {
		int (floor(tx*tw)), 0,
		1, th};

	SDL_Rect dst {col, _height/2 - h/2, 1, h};

	SDL_RenderCopy(r, t, &src, &dst);
}

void Window::update()
{
	if(_controls.walk_forward) 	_player.walk_forward(PLAYER_WALK_DIST);
	if(_controls.walk_back) 	_player.walk_back(PLAYER_WALK_DIST);
	if(_controls.shift_left) 	_player.shift_left(PLAYER_WALK_DIST);
	if(_controls.shift_right) 	_player.shift_right(PLAYER_WALK_DIST);

	if(_controls.turn_right) 	_player.turn_right(PLAYER_TURN_ANGLE);
	if(_controls.turn_left) 	_player.turn_left(PLAYER_TURN_ANGLE);
}


