#pragma once
#include "ClassTypeID.h"

class CNetMessage
{
public:
	ClassTypeID myClassTypeID = ClassTypeID::NoMessage;
	char myID;
	int mySize = 0;
	int myPosition = -1;
};

