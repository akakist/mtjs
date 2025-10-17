#include "uplinkconnectionstate.h"
#include "mutexInspector.h"

std::string dfsReferrer::_uplinkConnectionState::wdump()
{
    MUTEX_INSPECTOR;
    return "m_isTopServer=" + std::to_string(m_isTopServer);

}
