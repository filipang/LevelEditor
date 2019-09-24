#pragma once
#include <vector>
#include "SFML/Graphics.hpp"
class SelectMenu
{
public:
	int selectedItem;
	std::vector<sf::Texture> texture_vec;
	std::vector<sf::Sprite> sprite_vec;

public:
	SelectMenu();
	~SelectMenu();
};

