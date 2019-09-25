// LevelEdit.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "SFML/Graphics.hpp"
#include <iostream>
#include <list>
#include <chrono>
#include <thread>
#include <cmath>

struct btn {
	sf::Sprite& sprite;
	int btn_id;
	btn(sf::Sprite& _spritem, int _btn_id) 
		: sprite(_spritem), btn_id(_btn_id) {}
};

struct tile {
	sf::Sprite sprite;
	sf::Vector2f initPos;
	tile(sf::Sprite sprite, sf::Vector2f initPos) {
		this->sprite = sprite;
		this->initPos = initPos;
		this->sprite.setPosition(initPos);
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

sf::Vector2i globalToGrid(sf::Vector2f coords, float btwin_length) {
	return sf::Vector2i(std::floor(coords.x/btwin_length),std::floor(coords.y / btwin_length)); 
	//floor rounds decimal value to whole values downward (2.8 to 2, -1.3 to -2, 10.9 to 10, etc)
	//casting from float to int here directly will yield unwanted results
	//for negative numbers (-1.3 will be rounded to -1, while we need it to be -2)
}

sf::Vector2f gridToGlobal(sf::Vector2i coords, float btwin_length) {
	return sf::Vector2f(coords.x * btwin_length, coords.y * btwin_length);
}

void drawGrid(int cnt, float btwin_length, sf::Vector2f offset, sf::RenderWindow& app) {
	
	for (int x = -cnt / 2; x < cnt / 2; x++) {
		drawLine(sf::Vector2f(x*btwin_length, -cnt / 2 * btwin_length) + offset, sf::Vector2f(x*btwin_length, cnt / 2 * btwin_length) + offset, app);
	}
	for (int y = -cnt / 2; y < cnt / 2; y++) {
		drawLine(sf::Vector2f(-cnt / 2 * btwin_length, y*btwin_length) + offset, sf::Vector2f( cnt / 2 * btwin_length, y*btwin_length) + offset, app);
	}
}

int main()
{
	bool is_dragging = false;
	sf::Vector2f nav_offset_b_drag;
	sf::Vector2f nav_offset(0,0);
	sf::Vector2i ms_drag_point;
	sf::Vector2f before_drag_pos;

	std::list<btn> btns;

	std::list<tile> tiles;

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

	sf::Texture tile1_txture;
	tile1_txture.loadFromFile("cell_skin_1.png");
	sf::Sprite tile1(tile1_txture);
	btns.push_back(btn(tile1, 1));


	men_bkgr.setPosition(bkgr.getPosition() + sf::Vector2f(1000, 0));
	tile1.setPosition(men_bkgr.getPosition() + sf::Vector2f(12, 60));

	while (app.isOpen()) {
		while (app.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				app.close();
				break;
			case sf::Event::MouseButtonPressed:
				for (btn b : btns) {
					if(b.sprite.getGlobalBounds().contains(sf::Mouse::getPosition(app).x, sf::Mouse::getPosition(app).y)){
						std::cout << men_bkgr.getPosition().x << " " << men_bkgr.getPosition().y << std::endl;
					}
				}

				//if click on the grid
				if(event.mouseButton.button == sf::Mouse::Button::Left)
					tiles.push_back(tile(sf::Sprite(tile1_txture), gridToGlobal(globalToGrid(static_cast<sf::Vector2f>(sf::Mouse::getPosition(app))-nav_offset, 85.0f), 85.0f)));
				//if the click was a left click
				//if menu isn't clicked
				if (true && event.mouseButton.button == sf::Mouse::Button::Right && is_dragging == false) {
					is_dragging = true; 
					ms_drag_point = sf::Mouse::getPosition(app);
					nav_offset_b_drag = nav_offset;
				}
				break;

			case sf::Event::MouseButtonReleased:
				if (is_dragging && event.mouseButton.button == sf::Mouse::Button::Right) is_dragging = false;
				break;
			}
		}
		if (is_dragging) {
			nav_offset = nav_offset_b_drag  + static_cast<sf::Vector2f>(sf::Mouse::getPosition(app)) - static_cast<sf::Vector2f>(ms_drag_point);
			bkgr.setPosition(nav_offset);
		}

		app.setView(view);
		app.clear(sf::Color(0, 0, 255, 255));

		app.draw(bkgr);
		drawGrid(200, 85.0f,nav_offset, app);
		for (tile t : tiles) {
			t.sprite.setPosition(t.initPos + nav_offset);
			app.draw(t.sprite);
		}
		app.draw(men_bkgr);
		app.draw(tile1);
		app.display();
		

		std::this_thread::sleep_for(std::chrono::milliseconds(9));
	}
}