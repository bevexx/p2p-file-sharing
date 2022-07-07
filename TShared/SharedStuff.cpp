#include "SharedStuff.h"

bool File::IsOpen() const
{
	return myStream.is_open();
}

const std::streamsize &File::Size() const
{
	return myStream.gcount();
}

void File::Init(int aTotalFileSize)
{
	myTotalFileSize = aTotalFileSize;
	myStream.rdbuf()->pubseekoff(0, myStream.beg);
}

void File::Open(const std::string &aPath, std::ios_base::openmode mode, int prot)
{
	myStream.open(aPath, mode, prot);
	Init(myStream.rdbuf()->pubseekoff(0, myStream.end));
}

void File::Read(char *aMemory, int aLength)
{
	myStream.read(aMemory, aLength);
}

void File::Close()
{
	myPacketIndex = 0;
	myStream.close();
}

int File::IncrementIndex()
{
	return myPacketIndex++;
}

void RecieveFile::Flush()
{
	myStream.flush();
}

void RecieveFile::Write(char *aMemory, int aLength)
{
	myStream.write(aMemory, aLength);
}

void RecieveFile::ResetIndex()
{
	myPacketIndex = 0;
}