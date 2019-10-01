// Texture class constructor
#include "Texture.h"
using namespace moony;
Texture::Texture(const sf::Texture* texture, sf::IntRect rect)
{
	m_texture = texture;
	m_rect = rect;
}

