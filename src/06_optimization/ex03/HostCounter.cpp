#include "HostCounter.h"

#include <algorithm> // for std::find

HostCounter::HostCounter()
{
}

bool HostCounter::isNewHost(const std::string& hostname)
{
    return myHosts.find(hostname) == myHosts.end();
}

void HostCounter::notifyHost(const std::string& hostname)
{
    // add the host in the list if not already in
    if(isNewHost(hostname))
    {
        myHosts.insert(hostname); 
    }
}

int HostCounter::getNbOfHosts()
{
    return myHosts.size();
}
