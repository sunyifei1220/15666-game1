#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

#include <set>

#include "data_path.hpp"

#include "load_save_png.hpp"
#include <glm/gtx/string_cast.hpp>
#include <chrono>
#include <thread>
PPU466::Palette parse_color(PPU466::Tile* tile, std::vector< glm::u8vec4 > data) {
	std::vector < glm::u8vec4>vec;
	for (glm::u8vec4 color : data) {
		int dup = 0;
		for (uint8_t i = 0; i < vec.size(); ++i) {
			if (glm::all(glm::equal(color, vec[i]))) dup = 1;
		}
		if (!dup) vec.push_back(color);
	}
	std::array<glm::u8vec4, 4>arr;
	arr.fill({ 0x0, 0x0, 0x0, 0x0 });
	std::copy(vec.begin(), vec.end(), arr.begin());
	PPU466::Palette palette = arr;
	uint8_t j = 0,k = 0;
	
	for (glm::u8vec4 pixel : data) {
		if (j >= 8){	
			j = 0;
			k++;
		}
		for (uint8_t i = 0; i < 4; ++i) {
			if (glm::all(glm::equal(pixel, arr[i]))) {
				tile->bit0[k] = tile->bit0[k] << 1 | (i & 0x1);
				tile->bit1[k] = tile->bit1[k] << 1 | (i >> 1);
				break;
			}
			
		}
		j++; 
		
	}
	return palette;
}
PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:


	{
		glm::uvec2 size = glm::uvec2(0x8, 0x8);
		
		std::vector< glm::u8vec4 > poop = std::vector< glm::u8vec4>(8 * 8);
		std::vector< glm::u8vec4 > snake= std::vector< glm::u8vec4>(8 * 8);
		
		load_png(data_path("poop.png"), &size, &poop, LowerLeftOrigin);
		load_png(data_path("snake.png"), &size, &snake, LowerLeftOrigin);
		
		ppu.tile_table[32].bit0 = { 0,0,0,0,0,0,0,0 };
		ppu.tile_table[32].bit1 = { 0,0,0,0,0,0,0,0 };

		ppu.tile_table[31].bit0 = { 0,0,0,0,0,0,0,0 };
		ppu.tile_table[31].bit1 = { 0,0,0,0,0,0,0,0 };
		
		ppu.tile_table[0].bit0 = { 0,0,0,0,0,0,0,0 };
		ppu.tile_table[0].bit1 = { 0,0,0,0,0,0,0,0 };

		PPU466::Palette palette_poop = parse_color(&ppu.tile_table[32], poop);
		PPU466::Palette palette_snake = parse_color(&ppu.tile_table[31], snake);

		//used for the player:
		ppu.palette_table[7] = palette_poop;
		ppu.palette_table[6] = palette_snake;
		ppu.palette_table[0] = PPU466::Palette();
	}

	//some other misc sprites:
	for (uint32_t i = 1; i < 63; ++i) {
		ppu.sprites[i].y = 255;
		ppu.sprites[i].index = 32;
		ppu.sprites[i].attributes = 7;
	}

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			dir = LEFT;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			dir = RIGHT;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			dir = UP;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			dir = DOWN;
			return true;
		}else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			space.lock = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (poop_cnt > 20) {
		std::cout << "You Win!!" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		exit(0);
	}
	static uint8_t sprite_cnt = 1;
	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;
	if (space.pressed && !space.lock) {
		if (sprite_cnt < 63) {
			ppu.sprites[sprite_cnt].x = (uint8_t)player_at.x;
			ppu.sprites[sprite_cnt].y = (uint8_t)player_at.y;
			++sprite_cnt;
			++poop_cnt;
			space.lock = true;
		}
	}
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---
	constexpr float PlayerSpeed = 0.5f;
	switch (dir)
	{
	case LEFT:
		player_at.x -= PlayerSpeed;
		break;
	case RIGHT:
		player_at.x += PlayerSpeed;
		break;
	case UP:
		player_at.y += PlayerSpeed;
		break;
	case DOWN:
		player_at.y -= PlayerSpeed;
		break;
	default:
		break;
	}

	//tilemap gets recomputed every frame as some weird plasma thing:
	//NOTE: don't do this in your game! actually make a map or something :-)
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			//TODO: make weird plasma thing
			ppu.background[x+PPU466::BackgroundWidth*y] = 0;
		}
	}

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

	//player sprite:
	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y);
	ppu.sprites[0].index = 31;
	ppu.sprites[0].attributes = 6;



	//--- actually draw ---
	ppu.draw(drawable_size);
}
