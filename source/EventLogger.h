#pragma once

#include "Event.h"

struct EventLogger : Logger
{
	void Log(cstring text, LOG_LEVEL level) override
	{
		cas::EventType type;
		switch(level)
		{
		case L_INFO:
			type = cas::EventType::Info;
			break;
		case L_WARN:
			type = cas::EventType::Warning;
			break;
		case L_ERROR:
		default:
			type = cas::EventType::Error;
			break;
		}
		Event(type, text);
	}

	void Log(cstring text, LOG_LEVEL level, const tm& time) override
	{
		Log(text, level);
	}

	void Flush() override
	{

	}
};
