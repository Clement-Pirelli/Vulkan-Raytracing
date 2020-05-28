#ifndef FILES_H_DEFINED
#define FILES_H_DEFINED
#include <fstream>
#include <string>
#include <iostream>

class FileReader {
public:
	FileReader(std::string givenPath) : path(givenPath), stream(path, std::ios::binary) 
	{
		assert(!stream.fail() && stream.is_open());
	}

	~FileReader()
	{
		stream.close();
	}

	size_t length()
	{
		stream.seekg(0, stream.end);
		size_t length = stream.tellg();
		stream.seekg(0, stream.beg);
		return length;
	}

	void read(char *dataToFill, size_t size)
	{
		stream.read(dataToFill, size);
	}

private:
	std::string path;
	std::ifstream stream;
};

class FileWriter {
public:
	FileWriter(std::string givenPath) : path(givenPath), stream(path, std::ios::binary) {}

	~FileWriter()
	{
		stream.close();
	}

	bool write(char *data, size_t size)
	{
		stream.write(data, size);
#ifndef NDEBUG
		if (stream.fail()) return false;
#endif
		return true;
	}



private:
	std::string path;
	std::ofstream stream;
};


#endif