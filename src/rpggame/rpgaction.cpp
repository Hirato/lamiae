#include "rpggame.h"

//contains definitions for the various loadaction definitions

void action_teleport::exec()
{
	if(DEBUG_SCRIPT)
		DEBUGF("executing delayed teleport to dest %i", dest);
	entities::teleport(ent, dest, etype);
}

void action_spawn::exec()
{
	vector<int> matches;
	loopv(entities::ents)
	{
		if(entities::ents[i]->type == SPAWN && entities::ents[i]->attr[2] == tag)
			matches.add(i);
	}
	if(!matches.length())
	{
		WARNINGF("spawn action spawned nothing: %i %i %s %i %i", tag, ent, id, amount, qty);
		return;
	}

	if(!amount) loopv(matches)
		entities::spawn(*entities::ents[matches[i]], id, ent, qty);
	else loopi(amount)
		entities::spawn(*entities::ents[matches[rnd(matches.length())]], id, ent, qty);
}
