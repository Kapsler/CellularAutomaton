#include <SFML/Graphics.hpp>
#include "Hourglass.h"

namespace config
{
	static const bool VSYNC = false;	
}

int main()
{
	sf::RenderWindow window(sf::VideoMode(1000, 1000), "Hourglass Automaton");
	window.setVerticalSyncEnabled(config::VSYNC);

	Hourglass hourglass("./Assets/hourglass1500x5000.png");

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
			}
		}
		hourglass.RunSingleThreadCPU();

		window.clear(sf::Color::Blue);
		hourglass.Render(&window);
		window.display();
	}

	return 0;
}
