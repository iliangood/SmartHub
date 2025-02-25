#if !defined EVENTS_H
#define EVENTS_H

#include <vector>
#include <string>

class event
{
public:
    std::string type;
    void* data;
    event(std::string type, void* data)
    {
        this->type = type;
        this->data = data;
    }
};

class eventHandler
{
public:
    std::string type;
    void(*handler)(void*);
    eventHandler(std::string type, void(*function)(void*))
    {
        this->type = type;
        handler = function;
    }
};

class eventDispatcher
{
public:
    std::vector<eventHandler> handlers;
    long findHandler(std::string eventType);
    long findHandler(event Event);
public:
    void registerHandler(eventHandler handler);
    void registerHandler(std::string type, void(*function)(void*));
    void dispatchEvent(event Event);
};

#endif
