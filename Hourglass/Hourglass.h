#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <CL/cl.hpp>
#include "MathHelp.h"
#include "RNGesus.h"

class Hourglass
{
public:
	Hourglass(std::string filename, int screenWidth, int screenHeight);
	~Hourglass();

	void RunSingleThreadCPU();
	void RunOMPCPU();
	void RunOCL();
	void Render(sf::RenderWindow* window);
	void handleInput(sf::Keyboard::Key key);

private:

	void LoadFromImage(std::string filename);
	void GenerateTables();
	void RotateImage(int degrees);
	void ScaleImage(float factor);

	void InitOCL(std::string devicetype, int devicenum, int platformnum);
	std::string GetKernelCode(std::string inputFilename);

	int m_width, m_height;
	int m_screenWidth, m_screenHeight;
	sf::Uint32* hourglassArray;
	sf::Image hourglassImage;
	sf::Texture hourglassTexture;
	sf::Sprite hourglassSprite;

	//RGBA
	//Will be Endian converted
	sf::Uint32 m_emptyColor = 0x000000ff;
	sf::Uint32 m_sandColor = 0xffff00ff;
	sf::Uint32 m_wallColor = 0x0000ffff;

	uint8_t m_emptyCode = 0b00;
	uint8_t m_sandCode = 0b01;
	uint8_t m_wallCode = 0b10;

	uint8_t m_kernelLUT[256];
	sf::Uint32 m_colorLUT[3];

	RNGesus m_rng;
	unsigned long m_randoms[2048];

	int m_kernelStart = 1;

	//OpenCL
	cl::Buffer hourglassArrayBuffer, widthBuffer, heightBuffer, kernelOffsetBuffer, colorLUTBuffer, kernelLUTBuffer, randomsBuffer;
	cl::Kernel kernel_hourglass;
	cl::CommandQueue queue_hourglass;
	cl::NDRange global, local, offset;
};
