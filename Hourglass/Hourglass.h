#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <CL/cl.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>

class Hourglass
{
public:
	Hourglass(std::string filename);
	~Hourglass();

	void RunSingleThreadCPU();
	void Render(sf::RenderWindow* window);
	void RenderWithoutRewrite(sf::RenderWindow* window);

private:

	void LoadFromImage(std::string filename);
	void WriteArrayToImage();
	void GenerateTable();

	void InitOCL(std::string devicetype, int devicenum, int platformnum);
	std::string GetKernelCode(std::string inputFilename);

	int m_width, m_height;
	uint32_t* hourglassArray;
	sf::Image *hourglassImage;
	sf::Texture hourglassTexture;
	sf::Sprite hourglassSprite;

	uint32_t sandColor = 0xffff00ff;
	uint32_t wallColor = 0x000000ff;
	uint32_t emptyColor = 0xffffffff;
	uint8_t sandCode = 0b01;
	uint8_t wallCode = 0b10;
	uint8_t emptyCode = 0b00;

	uint8_t m_lookupTable[256];

	std::mt19937 twister;

	int kernelStart = 1;
};
