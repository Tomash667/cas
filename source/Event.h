#pragma once

#include "cas/Settings.h"

void Event(cas::EventType event_type, cstring msg);

inline void Info(cstring msg)
{
	Event(cas::EventType::Info, msg);
}

template<typename... Args>
inline void Info(cstring msg, const Args&... args)
{
	Info(Format(msg, args...));
}

inline void Warn(cstring msg)
{
	Event(cas::EventType::Warning, msg);
}

template<typename... Args>
inline void Warn(cstring msg, const Args&... args)
{
	Warn(Format(msg, args...));
}

inline void Error(cstring msg)
{
	Event(cas::EventType::Error, msg);
}

template<typename... Args>
inline void Error(cstring msg, const Args&... args)
{
	Error(Format(msg, args...));
}
