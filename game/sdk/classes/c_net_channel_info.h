#pragma once
#include "../hooks/detour_hook/vfunc.h"
#define FLOW_OUTGOING 0
#define FLOW_INCOMING 1
#define MAX_FLOWS     2

class c_net_channel_info
{
public:
	VIRTUAL_METHOD( float, getLatency, 9, ( int flow ), ( this, flow ) )

	std::byte pad[ 24 ];
	int OutSequenceNr;
	int InSequenceNr;
	int OutSequenceNrAck;
	int OutReliableState;
	int InReliableState;
	int chokedPackets;
	
};