#include "Hourglass.h"

Hourglass::Hourglass(std::string filename, int screenWidth, int screenHeight)
	: m_screenWidth(screenWidth)
	, m_screenHeight(screenHeight)
{
	//Generate LUT
	GenerateTables();

	//Init RNG
	m_rng.x = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	m_rng.y = 0xDEADBEEF;
	m_rng.z = std::chrono::high_resolution_clock::now().time_since_epoch().count() << 16;

	//Load array from file
	LoadFromImage(filename);
	
	float scale = std::min(static_cast<float>(m_screenWidth) / static_cast<float>(m_width), static_cast<float>(m_screenHeight) / static_cast<float>(m_height));

	hourglassSprite.setScale(scale, scale);
	
	//Init OpenCL
	InitOCL("cpu", 0, 0);
}

Hourglass::~Hourglass()
{
}

void Hourglass::GenerateTables()
{
	for (unsigned int i = 0; i < 256; ++i)
	{
		m_kernelLUT[i] = i;
	}

	//Real cases - Sand only
	m_kernelLUT[0b01000000] = 0b00000100;
	m_kernelLUT[0b00010000] = 0b00000001;
	m_kernelLUT[0b01000100] = 0b00000101;
	m_kernelLUT[0b00010001] = 0b00000101;
	m_kernelLUT[0b01000001] = 0b00000101;
	m_kernelLUT[0b00010100] = 0b00000101;
	m_kernelLUT[0b01010001] = 0b00010101;
	m_kernelLUT[0b01010100] = 0b01000101;
	m_kernelLUT[0b01010000] = 0b00000101;

	////Real cases - Sand + Wall
	m_kernelLUT[0b01001000] = 0b00001001;
	m_kernelLUT[0b00010010] = 0b00000110;
	m_kernelLUT[0b10010000] = 0b10000001;
	m_kernelLUT[0b01100000] = 0b00100100;
	m_kernelLUT[0b10011000] = 0b10001001;
	m_kernelLUT[0b01100010] = 0b00100110;
	m_kernelLUT[0b01010010] = 0b00010110;
	m_kernelLUT[0b01011000] = 0b01001001;
	m_kernelLUT[0b10010100] = 0b10000101;
	m_kernelLUT[0b01100001] = 0b00100101;

	//Real Cases - Sand under wall
	m_kernelLUT[0b10010001] = 0b10000101;
	m_kernelLUT[0b01100100] = 0b00100101;

	m_emptyColor = _byteswap_ulong(m_emptyColor);
	m_sandColor = _byteswap_ulong(m_sandColor);
	m_wallColor = _byteswap_ulong(m_wallColor);

	m_colorLUT[m_emptyCode] = m_emptyColor;
	m_colorLUT[m_sandCode] = m_sandColor;
	m_colorLUT[m_wallCode] = m_wallColor;
}

void Hourglass::RotateImage(int degrees)
{
	sf::RenderTexture tex;
	tex.create(m_width, m_height);

	sf::Sprite sprite;
	sprite.setTexture(hourglassTexture);
	sprite.setOrigin(sprite.getLocalBounds().width / 2.0f, sprite.getLocalBounds().height / 2.0f);
	sprite.setPosition(m_width / 2.0f, m_height / 2.0f);
	sprite.rotate(degrees);

	tex.clear(sf::Color::Blue);
	tex.draw(sprite);
	tex.display();

	hourglassImage = tex.getTexture().copyToImage();

	memcpy(hourglassArray, hourglassImage.getPixelsPtr(), m_width * m_height * sizeof(sf::Uint32));
	queue_hourglass.enqueueWriteBuffer(hourglassArrayBuffer, CL_FALSE, 0, sizeof(sf::Uint32) * m_width * m_height, hourglassArray);
}

void Hourglass::ScaleImage(float factor)
{
	hourglassSprite.scale(factor, factor);
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


	// create buffers on the device
	hourglassArrayBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, m_width * m_height * sizeof(sf::Uint32));
	widthBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int));
	heightBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int));
	kernelLUTBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(uint8_t) * 256);
	colorLUTBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(uint32_t) * 3);
	kernelOffsetBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int));
	randomsBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(unsigned long) * 2048);

	//create queue to which we will push commands for the device.
	queue_hourglass = cl::CommandQueue(context, default_device);

	// write arrays A and B to the device
	queue_hourglass.enqueueWriteBuffer(hourglassArrayBuffer, CL_TRUE, 0, m_width * m_height * sizeof(sf::Uint32), hourglassArray);
	queue_hourglass.enqueueWriteBuffer(widthBuffer, CL_TRUE, 0, sizeof(int), &m_width);
	queue_hourglass.enqueueWriteBuffer(heightBuffer, CL_TRUE, 0, sizeof(int), &m_height);
	queue_hourglass.enqueueWriteBuffer(kernelLUTBuffer, CL_TRUE, 0, sizeof(uint8_t) * 256, m_kernelLUT);
	queue_hourglass.enqueueWriteBuffer(colorLUTBuffer, CL_TRUE, 0, sizeof(uint32_t) * 3, m_colorLUT);
	queue_hourglass.enqueueWriteBuffer(kernelOffsetBuffer, CL_TRUE, 0, sizeof(int), &m_kernelStart);
	queue_hourglass.enqueueWriteBuffer(randomsBuffer, CL_TRUE, 0, sizeof(unsigned long) * 2048, m_randoms);

	kernel_hourglass = cl::Kernel(program, "iterate");
	kernel_hourglass.setArg(0, hourglassArrayBuffer);
	kernel_hourglass.setArg(1, widthBuffer);
	kernel_hourglass.setArg(2, heightBuffer);
	kernel_hourglass.setArg(3, kernelLUTBuffer);
	kernel_hourglass.setArg(4, colorLUTBuffer);
	kernel_hourglass.setArg(5, kernelOffsetBuffer);
	kernel_hourglass.setArg(6, randomsBuffer);

	//Find ranges
	int pow2widthRange = mathHelp::pow2roundup(m_width);
	int pow2heightRange = mathHelp::pow2roundup(m_height);

	global = cl::NDRange(pow2heightRange, pow2widthRange);

	local = cl::NDRange(maxgroupsize, 1);
	offset = cl::NullRange;
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

	for(int y = m_kernelStart % 2 + 1; y < m_height; y += 2)
	{
		for (int x = m_kernelStart % 2 + 1; x < m_width; x += 2)
		{
			uint8_t kernel = 0b0;

			//BUILDING KERNEL
			//Top Left
			kernel = kernel | ((hourglassArray[(y - 1) * m_width + (x - 1)] == m_wallColor) << 7);
			kernel = kernel | ((hourglassArray[(y - 1) * m_width + (x - 1)] == m_sandColor) << 6);
			
			//Top Right
			kernel = kernel | ((hourglassArray[(y - 1) * m_width + x] == m_wallColor) << 5);
			kernel = kernel | ((hourglassArray[(y - 1) * m_width + x] == m_sandColor) << 4);

			//Bottom Left
			kernel = kernel | ((hourglassArray[y * m_width + (x - 1)] == m_wallColor) << 3);
			kernel = kernel | ((hourglassArray[y * m_width + (x - 1)] == m_sandColor) << 2);

			//Bottom Right
			kernel = kernel | ((hourglassArray[y * m_width + x] == m_wallColor) << 1);
			kernel = kernel | ((hourglassArray[y * m_width + x] == m_sandColor));

			//Looking up new case of built kernel
			if(kernel == 0b01010000)
			{
				float randVal = m_rng.GetNumber() / (ULONG_MAX + 1.0f);
				if (randVal < 0.5f) continue;
			}
			kernel = m_kernelLUT[kernel];

			//Getting Cell Values
			cell1 = kernel >> 6;
			cell2 = (kernel & 0b00110000) >> 4;
			cell3 = (kernel & 0b00001100) >> 2;
			cell4 = (kernel & 0b00000011);

			//Rewriting Cells
			hourglassArray[(y - 1) * m_width + (x - 1)] = m_colorLUT[cell1];
			hourglassArray[(y - 1) * m_width + x] = m_colorLUT[cell2];
			hourglassArray[y * m_width + (x - 1)] = m_colorLUT[cell3];
			hourglassArray[y * m_width + x] = m_colorLUT[cell4];
		}
	}

	m_kernelStart == 1 ? m_kernelStart = 2 :  m_kernelStart = 1;
}

void Hourglass::RunOMPCPU()
{
	#pragma omp parallel num_threads(8)
	{
		uint8_t cell1 = 0b0;
		uint8_t cell2 = 0b0;
		uint8_t cell3 = 0b0;
		uint8_t cell4 = 0b0;

		#pragma omp for
		for (int y = m_kernelStart % 2 + 1; y < m_height; y += 2)
		{
			for (int x = m_kernelStart % 2 + 1; x < m_width; x += 2)
			{
				uint8_t kernel = 0b0;

				//BUILDING KERNEL
				//Top Left
				kernel = kernel | ((hourglassArray[(y - 1) * m_width + (x - 1)] == m_wallColor) << 7);
				kernel = kernel | ((hourglassArray[(y - 1) * m_width + (x - 1)] == m_sandColor) << 6);

				//Top Right
				kernel = kernel | ((hourglassArray[(y - 1) * m_width + x] == m_wallColor) << 5);
				kernel = kernel | ((hourglassArray[(y - 1) * m_width + x] == m_sandColor) << 4);

				//Bottom Left
				kernel = kernel | ((hourglassArray[y * m_width + (x - 1)] == m_wallColor) << 3);
				kernel = kernel | ((hourglassArray[y * m_width + (x - 1)] == m_sandColor) << 2);

				//Bottom Right
				kernel = kernel | ((hourglassArray[y * m_width + x] == m_wallColor) << 1);
				kernel = kernel | ((hourglassArray[y * m_width + x] == m_sandColor));

				//Looking up new case of built kernel
				if (kernel == 0b01010000)
				{
					float randVal = m_rng.GetNumber() / (ULONG_MAX + 1.0f);
					if (randVal < 0.5f) continue;
				}
				kernel = m_kernelLUT[kernel];

				//Getting Cell Values
				cell1 = kernel >> 6;
				cell2 = (kernel & 0b00110000) >> 4;
				cell3 = (kernel & 0b00001100) >> 2;
				cell4 = (kernel & 0b00000011);

				//Rewriting Cells
				hourglassArray[(y - 1) * m_width + (x - 1)] = m_colorLUT[cell1];
				hourglassArray[(y - 1) * m_width + x] = m_colorLUT[cell2];
				hourglassArray[y * m_width + (x - 1)] = m_colorLUT[cell3];
				hourglassArray[y * m_width + x] = m_colorLUT[cell4];
			}
		}
	}
	
	m_kernelStart == 1 ? m_kernelStart = 2 : m_kernelStart = 1;
}

void Hourglass::RunOCL()
{
	m_kernelStart == 1 ? m_kernelStart = 2 : m_kernelStart = 1;

	for(size_t i = 0u; i < 2048; ++i)
	{
		m_randoms[i] = m_rng.GetNumber();
	}

	queue_hourglass.enqueueWriteBuffer(randomsBuffer, CL_FALSE, 0, sizeof(unsigned long) * 2048, m_randoms);
	queue_hourglass.enqueueWriteBuffer(kernelOffsetBuffer, CL_FALSE, 0, sizeof(int), &m_kernelStart);
	queue_hourglass.enqueueNDRangeKernel(kernel_hourglass, offset, global, local);
	queue_hourglass.enqueueReadBuffer(hourglassArrayBuffer, CL_BLOCKING, 0, m_width * m_height * sizeof(sf::Uint32), hourglassArray);
	queue_hourglass.finish();
}

void Hourglass::Render(sf::RenderWindow* window)
{
	//Write array to image
	memcpy(const_cast<sf::Uint8*>(hourglassImage.getPixelsPtr()), hourglassArray, m_width * m_height * sizeof(sf::Uint32));

	//Format output sprite
	hourglassTexture.loadFromImage(hourglassImage);
	hourglassSprite.setTexture(hourglassTexture, true);
	hourglassSprite.setOrigin(hourglassSprite.getLocalBounds().width / 2.0f, hourglassSprite.getLocalBounds().height / 2.0f);
	hourglassSprite.setPosition(m_screenWidth / 2.0f, m_screenHeight / 2.0f);
	window->draw(hourglassSprite);
}

void Hourglass::handleInput(sf::Keyboard::Key key)
{
	if(key == sf::Keyboard::Up)
	{
		ScaleImage(1.5f);
	}
	if (key == sf::Keyboard::Down)
	{
		ScaleImage(0.75f);
	}
	if (key == sf::Keyboard::Left)
	{
		RotateImage(-45);
	}
	if (key == sf::Keyboard::Right)
	{
		RotateImage(45);
	}
}

void Hourglass::LoadFromImage(std::string filename)
{
	hourglassImage.loadFromFile(filename);

	m_width = hourglassImage.getSize().x;
	m_height = hourglassImage.getSize().y;

	m_width = std::max(m_width, m_height);
	m_height = m_width;

	hourglassArray = new sf::Uint32[m_width * m_height];
	
	memcpy(hourglassArray, hourglassImage.getPixelsPtr(), m_width * m_height * sizeof(sf::Uint32));
}
