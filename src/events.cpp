#include "events.h"
#include <iostream>

long eventDispatcher::findHandler(std::string eventType)
{
    for(size_t i = 0; i < handlers.size(); i++)
    {
        if(handlers[i].type == eventType)
        {
            return static_cast<long>(i);
        }
    }
    return -1;
}

long eventDispatcher::findHandler(event Event)
{
    return findHandler(Event.type);
}

void eventDispatcher::registerHandler(eventHandler handler)
{
    handlers.push_back(handler);
}

void eventDispatcher::registerHandler(std::string type, void(*function)(void*))
{
    handlers.push_back(eventHandler(type, function));
}

void eventDispatcher::dispatchEvent(event Event)
{
    long pos = findHandler(Event);
    if (pos != -1)
    {
        handlers[pos].handler(Event.data);
    }
    else
    {
        std::cerr << "No handler for event type: " << Event.type << std::endl;
    }
}
