#include "event_mt.h"
#include "IUtils.h"
#include "commonError.h"


void Event::NoPacked::pack(outBuffer& ) const
{

    throw CommonError("NoPacked::pack: invalid usage for class %s",iUtils->genum_name(id));
}
void Event::NoPacked::unpack(inBuffer& )
{
    throw CommonError("NoPacked::unpack: invalid usage for class %s",iUtils->genum_name(id));
}
