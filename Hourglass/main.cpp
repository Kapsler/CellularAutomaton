#include <SFML/Graphics.hpp>
#include "Hourglass.h"

namespace config
{
	static const int screenWidth = 900;
	static const int screenHeight = 900;
	static const bool VSYNC = false;	
}

int main()
{
	sf::RenderWindow window(sf::VideoMode(config::screenWidth, config::screenHeight), "Hourglass Automaton");
	window.setVerticalSyncEnabled(config::VSYNC);

	Hourglass hourglass("./Assets/hourglass150x500.png", config::screenWidth, config::screenHeight );

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
			}
		}
		//hourglass.RunSingleThreadCPU();
		//hourglass.RunOMPCPU();
		hourglass.RunOCL();

		window.clear(sf::Color::Blue);
		hourglass.Render(&window);
		window.display();
	}

	return 0;
}
