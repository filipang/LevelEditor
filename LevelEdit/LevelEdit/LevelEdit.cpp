// LevelEdit.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "SFML/Graphics.hpp"
#include <iostream>

int main()
{
    std::cout << "Hello World!\n"; 
	sf::Event event;
	sf::RenderWindow app(sf::VideoMode(1600, 900), "SFML STARTUP", sf::Style::Close | sf::Style::Resize);
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

	while (app.isOpen()) {
		while (app.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				app.close();
				break;
			}
		}
		app.setView(view);
		app.clear(sf::Color(0, 0, 255, 255));
		men_bkgr.setPosition(bkgr.getPosition() + sf::Vector2f(1000, 0));
		tile1.setPosition(men_bkgr.getPosition() + sf::Vector2f(12,60));

		app.draw(bkgr);
		app.draw(men_bkgr);
		app.draw(tile1);
		app.display();
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
