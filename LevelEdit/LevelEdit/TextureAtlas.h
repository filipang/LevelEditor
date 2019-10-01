


#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <SFML/Graphics.hpp>

#include "Texture.h"
#include "Log.h"

#ifdef USE_ZLIB
	#include <zlib.h>
#endif


namespace moony
{
	// The TextureAtlas class holds all the Moony Texture objects inside Atlas objects which can be 
	// referenced by the name of the original file they were loaded as. Only one of these objects should be necessary.
	class TextureAtlas
	{
	public:
		bool loadFromFile(const std::string& file_path);
		const Texture findSubTexture(std::string name);
		std::vector<std::string> getSubTextureNames();

	private:
		struct Atlas
		{
			Atlas() : m_texture(new sf::Texture) {};
			std::unique_ptr<sf::Texture> m_texture;
			std::unordered_map<std::string, sf::IntRect> m_texture_table;
		};

		std::vector<Atlas> m_atlas_list;
	};
}
