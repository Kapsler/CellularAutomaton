#include <SFML/Graphics.hpp>
#include "Hourglass.h"
#include <Windows.h>
#include "TimerClass.h"
#include "CommandLineParser.h"

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

namespace config
{
	static const int screenWidth = 900;
	static const int screenHeight = 900;
	static const bool VSYNC = false;	
}

namespace commandLine
{
	string mode, devicetype;
	bool measure;
}

bool handleParameters(int argc, char* argv[])
{
	CommandLineParser input(argc, argv);
	

	if (input.cmdOptionExists("--mode"))
	{
		commandLine::mode = input.getCmdOption("--mode");
	}
	else
	{
		cerr << "Parameter --mode not found!" << endl;
		return false;
	}

	if (commandLine::mode == "ocl")
	{
		if (input.cmdOptionExists("--device"))
		{
			commandLine::devicetype = input.getCmdOption("--device");
		}
	}

	if (input.cmdOptionExists("--measure"))
	{
		commandLine::measure = true;
	} else
	{
		commandLine::measure = false;
	}

	return true;
}

int main(int argc, char* argv[])
{
	sf::RenderWindow window(sf::VideoMode(config::screenWidth, config::screenHeight), "Hourglass Automaton");
	window.setVerticalSyncEnabled(config::VSYNC);

	handleParameters(argc, argv);

	Hourglass hourglass("./Assets/hourglass300x1000.png", config::screenWidth, config::screenHeight, commandLine::mode.c_str(), commandLine::devicetype.c_str());
	TimerClass timer;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			} else if(event.type == sf::Event::KeyPressed)
			{
				if(event.key.code == sf::Keyboard::Escape)
				{
					window.close();
				}

				hourglass.handleInput(event.key.code);
			} else if (event.type == sf::Event::MouseButtonPressed)
			{
				hourglass.handleInput(event.mouseButton);
			} else if (event.type == sf::Event::MouseMoved)
			{
				hourglass.handleInput(&window);
			}
		}

		double kernelTime = 0.0;

		if(commandLine::mode == "seq")
		{
			timer.StartTimer();
			hourglass.RunSingleThreadCPU();
			kernelTime = timer.GetTime();
		} else if(commandLine::mode == "omp")
		{
			timer.StartTimer();
			hourglass.RunOMPCPU();
			kernelTime = timer.GetTime();
		} else if (commandLine::mode == "ocl")
		{
			timer.StartTimer();
			hourglass.RunOCL();
			kernelTime = timer.GetTime();
		}
		if(commandLine::measure)
		{
			printf("Kernel Time: %f\r\n", kernelTime);
		}

		window.clear(sf::Color::Blue);
		hourglass.Render(&window);
		window.display();
	}

	return 0;
}
