#ifndef SESSION_HPP
#define SESSION_HPP

#include "Globals.hpp"
#include "PhysicalWrite.hpp"
#include "PhysicalRead.hpp"
#include "Datalink.hpp"
#include "Application.hpp"

VOID idle();
int run();
VOID writeBuffer();

extern std::mutex mtx;
extern std::queue<char> processingBuffer;
extern std::atomic<bool> connection;

#endif // !SESSION_HPP
