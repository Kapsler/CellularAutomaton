#include "Hourglass.h"

Hourglass::Hourglass(std::string filename)
{
	GenerateTable();

	twister.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());

	LoadFromImage(filename);
	hourglassSprite.setScale(0.1f, 0.1f);
}

Hourglass::~Hourglass()
{
}

void Hourglass::GenerateTable()
{
	for (unsigned int i = 0; i < 256; ++i)
	{
		m_lookupTable[i] = i;
	}

	//Real cases - Sand only
	m_lookupTable[0b01000000] = 0b00000100;
	m_lookupTable[0b00010000] = 0b00000001;
	m_lookupTable[0b01000100] = 0b00000101;
	m_lookupTable[0b00010001] = 0b00000101;
	m_lookupTable[0b01000001] = 0b00000101;
	m_lookupTable[0b00010100] = 0b00000101;
	m_lookupTable[0b01010001] = 0b00010101;
	m_lookupTable[0b01010100] = 0b01000101;
	m_lookupTable[0b01010000] = 0b00000101;

	////Real cases - Sand + Wall
	m_lookupTable[0b01001100] = 0b00001101;
	m_lookupTable[0b00010011] = 0b00000111;
	m_lookupTable[0b11010000] = 0b11000001;
	m_lookupTable[0b01110000] = 0b00110100;
	m_lookupTable[0b11011100] = 0b11001101;
	m_lookupTable[0b01110011] = 0b00110111;
}

void Hourglass::InitOCL(std::string devicetype, int devicenum, int platformnum)
{
	//get all platforms (drivers)
	std::vector<cl::Platform> all_platforms;
	std::vector<cl::Device> all_devices;

	cl::Platform::get(&all_platforms);
	if (all_platforms.size() == 0) {
		std::cout << " No platforms found. Check OpenCL installation!\n";
		exit(1);
	}


	cl::Platform default_platform;
	cl::Device default_device;
	unsigned int getdevicetype = CL_DEVICE_TYPE_ALL;;
	if (devicetype == "cpu")
	{
		getdevicetype = CL_DEVICE_TYPE_CPU;

	}
	else if (devicetype == "gpu")
	{
		getdevicetype = CL_DEVICE_TYPE_GPU;

	}

	if (getdevicetype != CL_DEVICE_TYPE_ALL)
	{
		for (int i = 0; i < all_platforms.size(); ++i)
		{
			default_platform = all_platforms[i];

			default_platform.getDevices(getdevicetype, &all_devices);
			if (all_devices.size() == 0) {
				continue;
			}

			default_device = all_devices[0];
		}
	}
	else
	{
		default_platform = all_platforms[platformnum];

		default_platform.getDevices(getdevicetype, &all_devices);
		if (all_devices.size() == 0) {
			std::cout << " No devices found. Check OpenCL installation!\n";
			exit(1);
		}

		default_device = all_devices[devicenum];


	}
	std::cout << "Using device: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";
	std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";

	int maxgroupsize = default_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();

	cl::Context context({ default_device });

	cl::Program::Sources sources;

	std::string kernel_code = GetKernelCode("./Kernel.txt");

	sources.push_back({ kernel_code.c_str(),kernel_code.length() });

	cl::Program program(context, sources);
	if (program.build({ default_device }) != CL_SUCCESS) {
		std::cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
		exit(1);
	}
}

std::string Hourglass::GetKernelCode(std::string inputFilename)
{
	std::string kernelcode;
	std::stringstream contentStream;
	std::ifstream fileStream(inputFilename);

	if (fileStream)
	{
		contentStream << fileStream.rdbuf();
		fileStream.close();

		kernelcode = contentStream.str();
	}
	else
	{
		std::cerr << "Kernel " << inputFilename << " not Found!" << std::endl;
	}

	return kernelcode;
}


void Hourglass::RunSingleThreadCPU()
{
	uint8_t cell1 = 0b0;
	uint8_t cell2 = 0b0;
	uint8_t cell3 = 0b0;
	uint8_t cell4 = 0b0;

	for(int y = kernelStart % 2 + 1; y < m_height; y += 2)
	{
		for (int x = kernelStart % 2 + 1; x < m_width; x += 2)
		{
			uint8_t kernel = 0b0;

			//BUILDING KERNEL
			//Top Left
			if (hourglassArray[(y-1) * m_width + (x-1)] == sandColor)
			{
				kernel = kernel | 0b01000000;
			} else if(hourglassArray[(y - 1) * m_width + (x - 1)] == wallColor)
			{
				kernel = kernel | 0b11000000;
			}

			//Top Right
			if (hourglassArray[(y-1) * m_width + x] == sandColor)
			{
				kernel = kernel | 0b00010000;
			} else if (hourglassArray[(y - 1) * m_width + x] == wallColor)
			{
				kernel = kernel | 0b00110000;
			}

			//Bottom Left
			if (hourglassArray[y * m_width + (x-1)] == sandColor)
			{
				kernel = kernel | 0b00000100;
			} else if (hourglassArray[y * m_width + (x - 1)] == wallColor)
			{
				kernel = kernel | 0b00001100;
			}

			//Bottom Right
			if (hourglassArray[y * m_width + x] == sandColor)
			{
				kernel = kernel | 0b00000001;
			} else if (hourglassArray[y * m_width + x] == wallColor)
			{
				kernel = kernel | 0b00000011;
			}

			//Looking up new case of built kernel
			kernel = m_lookupTable[kernel];

			//Getting Cell Values
			cell1 = kernel >> 6;
			cell2 = (kernel & 0b00110000) >> 4;
			cell3 = (kernel & 0b00001100) >> 2;
			cell4 = (kernel & 0b00000011);

			//Rewriting Cell 1
			if (cell1 == sandCode)
			{
				hourglassArray[(y - 1) * m_width + (x - 1)] = sandColor;
			}
			else if (cell1 == emptyCode)
			{
				hourglassArray[(y - 1) * m_width + (x - 1)] = emptyColor;
			}
			else if (cell1 == wallCode)
			{
				hourglassArray[(y - 1) * m_width + (x - 1)] = wallColor;
			}

			//Rewriting Cell 2
			if (cell2 == sandCode)
			{
				hourglassArray[(y - 1) * m_width + x] = sandColor;
			}
			else if (cell2 == emptyCode)
			{
				hourglassArray[(y - 1) * m_width + x] = emptyColor;
			}
			else if (cell2 == wallCode)
			{
				hourglassArray[(y - 1) * m_width + x] = wallColor;
			}

			//Rewriting Cell 3
			if (cell3 == sandCode)
			{
				hourglassArray[y * m_width + (x - 1)] = sandColor;
			}
			else if (cell3 == emptyCode)
			{
				hourglassArray[y * m_width + (x - 1)] = emptyColor;
			}
			else if (cell3 == wallCode)
			{
				hourglassArray[y * m_width + (x - 1)] = wallColor;
			}

			//Rewriting Cell 4
			if (cell4 == sandCode)
			{
				hourglassArray[y * m_width + x] = sandColor;
			}
			else if (cell4 == emptyCode)
			{
				hourglassArray[y * m_width + x] = emptyColor;
			}
			else if (cell4 == wallCode)
			{
				hourglassArray[y * m_width + x] = wallColor;
			}
		}
	}

	++kernelStart;
	if (kernelStart > 0b1111111111111111)
	{
		kernelStart = 1;
	}
}

void Hourglass::Render(sf::RenderWindow* window)
{
	WriteArrayToImage();

	hourglassTexture.loadFromImage(hourglassImage);
	hourglassSprite.setTexture(hourglassTexture);
	window->draw(hourglassSprite);
}

void Hourglass::RenderWithoutRewrite(sf::RenderWindow* window)
{
	hourglassTexture.loadFromImage(hourglassImage);
	hourglassSprite.setTexture(hourglassTexture);
	window->draw(hourglassSprite);
}

void Hourglass::LoadFromImage(std::string filename)
{
	hourglassImage.loadFromFile(filename);

	m_width = hourglassImage.getSize().x;
	m_height = hourglassImage.getSize().y;

	hourglassArray = new sf::Uint32[m_width * m_height];


	for(int y = 0; y < m_height; ++y)
	{
		for(int x = 0; x < m_width; ++x)
		{
			hourglassArray[y * m_width + x] = hourglassImage.getPixel(x, y).toInteger();
		}
	}
}

void Hourglass::WriteArrayToImage()
{
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			hourglassImage.setPixel(x, y, sf::Color(hourglassArray[y * m_width + x]));
		}
	}
}

