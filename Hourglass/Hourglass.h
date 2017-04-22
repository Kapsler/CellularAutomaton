#pragma once
#include <SFML/Graphics.hpp>
#include <random>

class Hourglass
{
public:
	Hourglass(std::string filename);
	~Hourglass();

	void Run();
	void Render(sf::RenderWindow* window);

private:

	void LoadFromImage(std::string filename);
	void WriteArrayToImage();
	void GenerateTable();

	int m_width, m_height;
	sf::Uint32* hourglassArray;
	sf::Image hourglassImage;
	sf::Texture hourglassTexture;
	sf::Sprite hourglassSprite;

	sf::Uint32 sandColor = 0xffff00ff;
	sf::Uint32 wallColor = 0x000000ff;
	sf::Uint32 emptyColor = 0xffffffff;

	uint8_t m_lookupTable[256];

	std::mt19937 twister;

	int kernelStart = 1;
};
