#pragma once

class RNGesus
{
public:
	RNGesus();
	RNGesus(const unsigned long& xseed, const unsigned long& yseed, const unsigned long& zseed);

	unsigned long GetNumber();

	unsigned long x = 0, y = 0, z = 0;
	unsigned long t = 0;
private:
};