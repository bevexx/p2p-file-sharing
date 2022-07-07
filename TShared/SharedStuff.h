#pragma once
#include <fstream>

class File
{
public:
	bool IsOpen() const;
	const std::streamsize& Size() const;

	void Init(int aTotalFileSize);
	void Open(const std::string& aPath, std::ios_base::openmode mode = 3, int prot = 64);
	void Read(char* aMemory, int aLength);
	void Close();
	int IncrementIndex();

	int GetPacketIndex() const { return myPacketIndex; }
	int GetTotalFileSize() const { return myTotalFileSize; }

protected:
	std::fstream myStream;
	int myPacketIndex = 0;
	int myTotalFileSize = 0;
};

class SendFile : public File
{
public:

protected:
};

class RecieveFile : public File
{
public:

	void Flush();
	void Write(char* aMemory, int aLength);
	void ResetIndex();
	
	std::filebuf *GetFileBuf() { return myStream.rdbuf(); }

	std::string myFileName = "";
	std::pair<std::string, char> myFileIsFrom;

protected:
	std::filebuf *myFileBuf;
};
