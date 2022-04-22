#pragma once
#include "..\SHARED_DEFINES.h"
#include "CNetMessage.h"
#include <string>
class CNetMessageFile : public CNetMessage
{
public:
	char myMessage[FILE_BUFLEN];
	int mySize = 0;
};

