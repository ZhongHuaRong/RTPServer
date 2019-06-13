#include <iostream>
#include "serverengine.h"

int main()
{
	rtpserver::ServerEngine engine(rtpserver::VIDEO_PORTBASE);
	rtpserver::RegisterEngine::Register_Engine(rtpserver::ENGINENAME,&engine);
	return engine.exec();
}
