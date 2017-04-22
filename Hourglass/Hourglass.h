#pragma once
#include <SFML/Graphics.hpp>
#include <random>

class Hourglass
{
public:
	Hourglass(std::string filename);
	~Hourglass();

	void RunSingleThreadCPU();
	void RunOMP();
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
	uint8_t sandCode = 0b01;
	uint8_t wallCode = 0b11;
	uint8_t emptyCode = 0b00;

	uint8_t m_lookupTable[256];

	std::mt19937 twister;

	int kernelStart = 1;
};
