// LevelEdit.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "SFML/Graphics.hpp"
#include <iostream>
#include <list>
#include <chrono>
#include <thread>
#include <cmath>
#include "TextureAtlas.h"
#include "SpriteBatch.h"

#define MAX_FILE_SIZE 100000
#define DEFAULT_MAP_SIZE_X 40
#define DEFAULT_MAP_SIZE_Y 20

#define PUSH(buffer, value){auto temp = value;\
							memcpy(buffer , &temp, sizeof(temp));\
							buffer+=sizeof(temp);}

#define POP(buffer, value)  memcpy(&value, buffer, sizeof(value));\
							buffer+=sizeof(value)	

#define F_READ(filestream,obj) filestream.read((char*)(&obj), sizeof(obj))
#define F_WRITE(filestream,obj) filestream.write((char*)(&obj), sizeof(obj)) 

std::list<std::string> tile_names = {"cell_skin_1.png","cell_skin_2.png" ,"cell_skin_3.png" ,"cell_skin_4.png" ,"cell_skin_5.png", "furniture_1.png"};
std::list<std::string> entity_names = { "minion_1.png" };
moony::TextureAtlas cell_atlas;

unsigned char buffer[MAX_FILE_SIZE];

enum CLSIDS
{
	CLS_Sprite = 1,
	CLS_Texture,
	CLS_Player,
	CLS_Entity,
	CLS_Map
};

const unsigned char occupied    = 1;
const unsigned char impassable  = 1<<1;
const unsigned char implaceable = 1<<2;

sf::Vector2i globalToGrid(sf::Vector2f coords, float btwin_length) {
	return sf::Vector2i(std::floor(coords.x / btwin_length), std::floor(coords.y / btwin_length));
	//floor rounds decimal value to whole values downward (2.8 to 2, -1.3 to -2, 10.9 to 10, etc)
	//casting from float to int here directly will yield unwanted results
	//for negative numbers (-1.3 will be rounded to -1, while we need it to be -2)
}

sf::Vector2f gridToGlobal(sf::Vector2i coords, float btwin_length) {
	return sf::Vector2f(coords.x * btwin_length, coords.y * btwin_length);
}

class Drawable {
public:

	sf::Sprite sprite;
	sf::Vector2f initPos;

public:

	Drawable(sf::Sprite sprite, sf::Vector2f initPos) {
		this->sprite = sprite;
		this->initPos = initPos;
		this->sprite.setPosition(initPos);
	}
};

class Entity {
public:

	int entity_id;
	sf::Vector2f pos;
	Drawable entity_drawable;

public:
	Entity(int entity_id, sf::Vector2f pos, Drawable entity_drawable) : entity_id(entity_id), pos(pos), entity_drawable(entity_drawable) {}

	int getSize() {
		return sizeof(entity_id) + sizeof(pos.x) + sizeof(pos.y);
	}
};

class SaveTile {
public:

	unsigned char occupation = 0;
	unsigned char no_tiles_occupying_this = 0;
	std::vector<int> tile_id_vector;

	//NOT SERIALIZABLE // ONLY IN EDITOR //
	std::list<Drawable> tiles;

public:

	int getSize() {
		int size = 0;
		size += sizeof(no_tiles_occupying_this) + sizeof(occupation) + sizeof(int) * no_tiles_occupying_this;
		return size;
	}
};

class Grid {
public:

	int size_x;
	int size_y;
	SaveTile** tile_grid;

public:

	int getSize() {
		int size = 0;
		size += sizeof(size_x) + sizeof(size_y);
		for (int j = 0; j < size_y; j++) {
			for (int i = 0; i < size_x; i++) {
				size += tile_grid[i][j].getSize();
			}
		}
		return size;
	}

	Grid(int size_x, int size_y) : size_x(size_x), size_y(size_y){

		tile_grid = new SaveTile*[size_x];
		for (int i = 0; i < size_x; i++) {
			tile_grid[i] = new SaveTile[size_y];
		}
	}

	~Grid(){

		for (int i = 0; i < size_x; i++) {
			delete[] tile_grid[i];
		}
		delete[] tile_grid;
	}
};

class Script {

};

class Save {
public:

	Grid map;
	std::vector<Entity> entities;
	//TO DO//
	std::vector<Script> scripts;

public:

	int getSize() {
		int size = 0;
		size += map.getSize();
		for (auto e : entities) {
			e.getSize();
		}
		return size;
	}

	Save(int mapx, int mapy)
		: map(mapx,mapy) {
	}
};

class IMenuItem {
public:
	sf::FloatRect rect;
	moony::Sprite sprite;
	int id;
	virtual bool isMouseOver(sf::RenderWindow& app) {
		std::cout << "SOMETHING WENT WRONG";
		return true;
	}
	virtual bool place(sf::Vector2f pos, sf::Vector2f offset, Save& scene) {
		std::cout << "SOMETHING WENT WRONG";
		return true;
	}
};

class MenuEntity : public IMenuItem {
public:
	MenuEntity(moony::Sprite spr) {
		sprite = spr;
	}

	bool isMouseOver(sf::RenderWindow& app) override {
		return rect.contains(static_cast<sf::Vector2f>(sf::Mouse::getPosition(app)));
	}

	bool place(sf::Vector2f mouse_pos, sf::Vector2f nav_offset, Save& scene) override {
		if (sf::FloatRect(0, 0, scene.map.size_x * 85.0f, scene.map.size_y*85.0f).contains(mouse_pos - nav_offset)) {
			Drawable drawable = Drawable(sf::Sprite(*sprite.m_subtexture.m_texture, sprite.m_subtexture.m_rect), mouse_pos - nav_offset);
			Entity entity = Entity(id, mouse_pos - nav_offset, drawable);
			scene.entities.push_back(entity);
		}
		return true;
	}
};

class MenuTile : public IMenuItem
{
public:

	MenuTile(moony::Sprite spr) {
		sprite = spr;
	}


	bool isMouseOver(sf::RenderWindow& app) override {
		return rect.contains(static_cast<sf::Vector2f>(sf::Mouse::getPosition(app)));
	}


	bool place(sf::Vector2f mouse_pos, sf::Vector2f nav_offset, Save& scene) override {
		sf::Vector2i coords = globalToGrid(mouse_pos - nav_offset, 85.0f);
		if (sf::IntRect(0, 0, scene.map.size_x, scene.map.size_y).contains(coords)) {
			if (!(scene.map.tile_grid[coords.x][coords.y].occupation & implaceable)) {
				bool alreadyExists = false;
				for (int index = 0; index < scene.map.tile_grid[coords.x][coords.y].no_tiles_occupying_this; index++) {
					if (scene.map.tile_grid[coords.x][coords.y].tile_id_vector[index] == id) {
						alreadyExists = true;
					}
				}
				if (!alreadyExists) {
					scene.map.tile_grid[coords.x][coords.y].tiles.push_back(Drawable(sf::Sprite(*sprite.m_subtexture.m_texture, sprite.m_subtexture.m_rect), gridToGlobal(coords, 85.0f)));
					scene.map.tile_grid[coords.x][coords.y].occupation = scene.map.tile_grid[coords.x][coords.y].occupation | occupied;
					if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) {
						scene.map.tile_grid[coords.x][coords.y].occupation = scene.map.tile_grid[coords.x][coords.y].occupation | implaceable | impassable;
					}

					scene.map.tile_grid[coords.x][coords.y].tile_id_vector.push_back(id);
					scene.map.tile_grid[coords.x][coords.y].no_tiles_occupying_this++;
				}
			}
		}
		return true;
	}

};


void drawLine(sf::Vector2f p1, sf::Vector2f p2, sf::RenderWindow& app) {
	sf::VertexArray lines(sf::LinesStrip, 2);
	lines[0].position = p1;
	lines[0].color = sf::Color(0, 0, 0, 255);
	lines[1].position = p2;
	lines[1].color = sf::Color(0, 0, 0, 255);
	app.draw(lines);
}

void drawGrid(int cnt, float btwin_length, sf::Vector2f offset, sf::RenderWindow& app) {
	
	for (int x = -cnt / 2; x < cnt / 2; x++) {
		drawLine(sf::Vector2f(x*btwin_length, -cnt / 2 * btwin_length) + offset, sf::Vector2f(x*btwin_length, cnt / 2 * btwin_length) + offset, app);
	}
	for (int y = -cnt / 2; y < cnt / 2; y++) {
		drawLine(sf::Vector2f(-cnt / 2 * btwin_length, y*btwin_length) + offset, sf::Vector2f( cnt / 2 * btwin_length, y*btwin_length) + offset, app);
	}
}

void loadAtlas(std::string atl_name, moony::TextureAtlas &texture_atlas) {
	texture_atlas.loadFromFile("atlases/" + atl_name + ".mtpf");
}

void menuItemGrid(sf::Vector2f top_left, sf::Vector2f bottom_right, float gap_len, std::list<IMenuItem*>& list, moony::SpriteBatch* batch) {
	sf::Vector2f i = top_left;
	int id_cnt = 0;
	for (auto tile_name : tile_names) {
		moony::Texture texture = cell_atlas.findSubTexture(tile_name);
		moony::Sprite sprite(texture);
		sprite.setPosition(i);
		MenuTile* tile = new MenuTile(sprite);
		tile->rect = sf::FloatRect(i.x, i.y, i.x + 85.0f, i.y+85.0f);
		tile->id = id_cnt++;
		list.push_back(tile);
		batch->draw(list.back()->sprite);

		i.x += sprite.m_subtexture.m_rect.width + gap_len;
		if (i.x + sprite.m_subtexture.m_rect.width > bottom_right.x) {
			i.x = top_left.x;
			i.y += sprite.m_subtexture.m_rect.height + gap_len;
		}
	}
	for(auto entity_name : entity_names){
		moony::Texture texture = cell_atlas.findSubTexture(entity_name);
		moony::Sprite sprite(texture);
		sprite.setPosition(i);
		MenuEntity* entity = new MenuEntity(sprite);
		entity->id = id_cnt++;
		entity->rect = sf::FloatRect(i.x, i.y, i.x + 85.0f, i.y + 85.0f);
		list.push_back(entity);
		batch->draw(list.back()->sprite);
		i.x += sprite.m_subtexture.m_rect.width + gap_len;
		if (i.x + sprite.m_subtexture.m_rect.width > bottom_right.x) {
			i.x = top_left.x;
			i.y += sprite.m_subtexture.m_rect.height + gap_len;
		}
	}
}

unsigned char isOccupied(SaveTile** grid, sf::Vector2i coords) {
	return grid[coords.x][coords.y].occupation;
}

int clearBuffer(unsigned char buffer[]) {
	memset(buffer, 0, MAX_FILE_SIZE);
	return 0;
}

int saveLevelFileFromBuffer(std::string level_name, unsigned char buffer[], Save& scene) {
	std::ofstream file(level_name, std::ios::out | std::ios::binary);
	file.write((char*)buffer, scene.getSize());
	return 0;
}


int loadLevelFileToBuffer(std::string level_name, unsigned char buffer[]) {
	std::ifstream file(level_name, std::ios::in | std::ios::binary);
	file.read((char*)buffer, MAX_FILE_SIZE);
	return 0;
}

bool saveWorldToBuffer(unsigned char buffer[], Save& s) {
	int id, size = s.map.getSize();
	id = CLS_Map;
	PUSH(buffer, id);
	PUSH(buffer, size);	
	PUSH(buffer, s.map.size_x);
	PUSH(buffer, s.map.size_y);
	for (int j = 0; j < s.map.size_y; j++) {
		for (int i = 0; i < s.map.size_x; i++) {
			PUSH(buffer, s.map.tile_grid[i][j].occupation);
			PUSH(buffer, s.map.tile_grid[i][j].no_tiles_occupying_this);
			for (int nav = 0; nav < s.map.tile_grid[i][j].no_tiles_occupying_this; nav++) {
				PUSH(buffer, s.map.tile_grid[i][j].tile_id_vector[nav]);
			}
		}
	}
	id = CLS_Entity;
	for (auto e : s.entities) {
		PUSH(buffer, id);
		PUSH(buffer, e.getSize());
		PUSH(buffer, e.entity_id);
		PUSH(buffer, e.pos.x);
		PUSH(buffer, e.pos.y);
	}

	return true;
}

bool loadWorldFromBuffer(unsigned char buffer[], Save& s, std::list<IMenuItem*>& menu_items) {
	unsigned char* end = buffer + s.getSize() + sizeof(int)*2 + s.entities.size()*sizeof(int)*2;
	int clsid, size;
	int size_x, size_y;
	POP(buffer, clsid);
	POP(buffer, size);
	POP(buffer, size_x);
	POP(buffer, size_y);
	//s = save(size_x, size_y);
	for (int j = 0; j < size_y; j++) {
		for (int i = 0; i < size_x; i++) {
			POP(buffer, s.map.tile_grid[i][j].occupation);
			POP(buffer, s.map.tile_grid[i][j].no_tiles_occupying_this);
			s.map.tile_grid[i][j].tiles.clear();
			for (int nav = 0; nav < s.map.tile_grid[i][j].no_tiles_occupying_this; nav++) {
				int id;
				POP(buffer, id);
				s.map.tile_grid[i][j].tile_id_vector.push_back(id);
				for (auto item : menu_items) {
					if (item->id == s.map.tile_grid[i][j].tile_id_vector[nav]) {
						Drawable drawable = Drawable(sf::Sprite(*item->sprite.m_subtexture.m_texture, item->sprite.m_subtexture.m_rect), gridToGlobal(sf::Vector2i(i, j), 85.0f));
						//drawable.id = item->id;
						s.map.tile_grid[i][j].tiles.push_back(drawable);
						
					}
				}
			}


		}
	}
  	s.entities.clear();
	while (buffer < end) {
		int ent_id, size;
		float posx, posy;
		POP(buffer, clsid);
		POP(buffer, size);
		POP(buffer, ent_id);
		POP(buffer, posx);
		POP(buffer, posy);
		for (auto item : menu_items) {
			if (item->id == ent_id) {
				Drawable drawable = Drawable(sf::Sprite(*item->sprite.m_subtexture.m_texture, item->sprite.m_subtexture.m_rect), sf::Vector2f(posx, posy));
				Entity entity = Entity(ent_id, sf::Vector2f(posx, posy), drawable);
				s.entities.push_back(entity);

			}
		}
	}
	
	return true;
}

sf::Vector2i clamp(sf::Vector2i v, sf::Vector2i min, sf::Vector2i max) {
	if (v.x < min.x) v.x = min.x;
	if (v.y < min.y) v.y = min.y;	
	if (v.x > max.x) v.x = max.x;
	if (v.y > max.y) v.y = max.y;
	return v;
}

sf::Vector2f clamp(sf::Vector2f v, sf::Vector2f min, sf::Vector2f max) {
	if (v.x < min.x) v.x = min.x;
	if (v.y < min.y) v.y = min.y;
	if (v.x > max.x) v.x = max.x;
	if (v.y > max.y) v.y = max.y;
	return v;
}

bool clickedOnMenu(bool is_menu_visible, sf::Sprite& menu, sf::RenderWindow& app) {
	return menu.getGlobalBounds().contains(static_cast<sf::Vector2f>(sf::Mouse::getPosition(app))) && is_menu_visible;
}

int main()
{
	Save scene(DEFAULT_MAP_SIZE_X, DEFAULT_MAP_SIZE_Y);
	bool is_menu_visible = true;
	bool is_dragging = false;
	sf::Vector2f nav_offset_b_drag;
	sf::Vector2f nav_offset(0,0);
	sf::Vector2i ms_drag_point;
	sf::Vector2f before_drag_pos;

	loadAtlas("cell_skins", cell_atlas);

	std::list<IMenuItem*> menu_items;
	moony::SpriteBatch menu_tiles_batch;

    std::cout << "Hello World!\n"; 
	sf::Event event;
	sf::RenderWindow app(sf::VideoMode(1600, 900), "SFML STARTUP", sf::Style::Close);
	sf::View view(sf::Vector2f(800,450), sf::Vector2f(1600.0f, 900.0f));

	sf::Texture men_bkgr_txture;
	men_bkgr_txture.loadFromFile("menu_bkgr.png");
	sf::Sprite men_bkgr(men_bkgr_txture);

	sf::Texture bkgr_txture;
	bkgr_txture.loadFromFile("bkgr.png");
	sf::Sprite bkgr(bkgr_txture);

	/*sf::Texture tile1_txture;
	tile1_txture.loadFromFile("cell_skin_1.png");
	sf::Sprite tile1(tile1_txture);
	btns.push_back(btn(tile1, 1));
	*/

	men_bkgr.setPosition(bkgr.getPosition() + sf::Vector2f(1000, 0));
	//tile1.setPosition(men_bkgr.getPosition() + sf::Vector2f(12, 60));

	menuItemGrid(men_bkgr.getPosition() + sf::Vector2f(12, 60), sf::Vector2f(1600.0f, 900.0f), 20.0f, menu_items, &menu_tiles_batch);
	auto current_selected = menu_items.front();
	while (app.isOpen()) {
		while (app.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				app.close();
				break;
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Button::Left && is_menu_visible) {
					//checks if mouse clicked any of the menu elements
					for (auto t : menu_items) {
 						if (t->isMouseOver(app)) {
							current_selected = t;
						}
					}
				}

				//if click on the grid
				//if the click was a right click
				//if wasn t already dragging
				if (event.mouseButton.button == sf::Mouse::Button::Right &&
					is_dragging == false &&
					!clickedOnMenu(is_menu_visible, men_bkgr, app))
				{
					is_dragging = true; 
					ms_drag_point = sf::Mouse::getPosition(app);
					nav_offset_b_drag = nav_offset;
				}
				break;

			case sf::Event::MouseButtonReleased:
				if (is_dragging && event.mouseButton.button == sf::Mouse::Button::Right) is_dragging = false;
				break;

			case sf::Event::KeyReleased:
				switch (event.key.code)
				{
				case sf::Keyboard::Escape:
					is_menu_visible = !is_menu_visible;
					break;
				case sf::Keyboard::S:
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
						clearBuffer(buffer);
						saveWorldToBuffer(buffer, scene);
						//saveLevelFileFromBuffer("save1.save", buffer, scene);
					}
					break;
				case sf::Keyboard::L:
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
						//clearBuffer(buffer);
						//loadLevelFileToBuffer("save1.save", buffer);
						loadWorldFromBuffer(buffer, scene, menu_items);
					}
					break;
				default:
					break;
				}

				break;
			}
		}
		if (is_dragging) {
			nav_offset = clamp(nav_offset_b_drag  + static_cast<sf::Vector2f>(sf::Mouse::getPosition(app)) - static_cast<sf::Vector2f>(ms_drag_point), -sf::Vector2f(DEFAULT_MAP_SIZE_X*85.0f - view.getViewport().width*view.getSize().x, DEFAULT_MAP_SIZE_Y*85.0f - view.getViewport().height*view.getSize().y), sf::Vector2f(0,0));
			bkgr.setPosition(nav_offset);
			ms_drag_point = sf::Mouse::getPosition(app);
			nav_offset_b_drag = nav_offset;
		}
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
			if (!clickedOnMenu(is_menu_visible, men_bkgr, app)) {
				current_selected->place(static_cast<sf::Vector2f>(sf::Mouse::getPosition(app)), nav_offset, scene);
			}
		}
		if (sf::Mouse::isButtonPressed(sf::Mouse::Middle)) {
			if (!clickedOnMenu(is_menu_visible, men_bkgr, app)) {
				sf::Vector2i coords = globalToGrid(static_cast<sf::Vector2f>(sf::Mouse::getPosition(app)) - nav_offset, 85.0f);
				if (sf::IntRect(0, 0, scene.map.size_x, scene.map.size_y).contains(coords)) {
					if (scene.map.tile_grid[coords.x][coords.y].occupation != 0) {
						scene.map.tile_grid[coords.x][coords.y].tiles.clear();
						scene.map.tile_grid[coords.x][coords.y].occupation = 0;
						scene.map.tile_grid[coords.x][coords.y].no_tiles_occupying_this = 0;
					}
				}
			}
		}

		app.setView(view);
		app.clear(sf::Color(0, 0, 255, 255));

		app.draw(bkgr);
		drawGrid(200, 85.0f,nav_offset, app);
		/*
		for (tile t : scene.tiles) {
			t.sprite.setPosition(t.initPos + nav_offset);
			app.draw(t.sprite);
		}*/
		
		for (int j = 0; j < scene.map.size_y; j++) {
			for (int i = 0; i < scene.map.size_x; i++) {
				if (scene.map.tile_grid[i][j].occupation & occupied) {
					for (auto t : scene.map.tile_grid[i][j].tiles) {
						t.sprite.setPosition(t.initPos + nav_offset);
						app.draw(t.sprite);
					}
				}
			}
		}

		for (auto e : scene.entities) {
			e.entity_drawable.sprite.setPosition(e.entity_drawable.initPos + nav_offset);
			app.draw(e.entity_drawable.sprite);
		}

		if (is_menu_visible) {
			app.draw(men_bkgr);
			app.draw(menu_tiles_batch);
		}
		app.display();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}