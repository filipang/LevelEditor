#ifndef SPRITEBATCH_H
#define SPRITEBATCH_H

// Number of frames a batch can be empty for before it is destroyed

#include <vector>
#include <array>
#include <random>
#include <iostream>
#include <SFML/Graphics.hpp>

#include "Sprite.h"
#include "Texture.h"

namespace moony
{
	class SpriteBatch : public sf::Drawable
	{
	public:
		void clear();
		void draw(const Sprite& sprite);
		void order();

	private:
		struct Batch
		{
			void clear();

			void draw(const Sprite& sprite);

			const sf::Texture* m_texture;
			int m_layer;
			unsigned int m_inactive;
			unsigned int m_count;
			//unsigned int m_reserved;
			std::vector<sf::Vertex> m_vertices;
		};

		

		Batch* findBatch(const Sprite& sprite);
		void draw(sf::RenderTarget& target, sf::RenderStates state) const;

		std::vector<Batch*> m_batches;
	};
}



#endif