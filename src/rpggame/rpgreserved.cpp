#include "rpggame.h"

using namespace game;

namespace reserved
{
	const char *amm_mana = NULL,
		*amm_health = NULL,
		*amm_experience = NULL;

	void loadammo()
	{
		amm_mana = queryhashpool("mana");
		amm_health = queryhashpool("health");
		amm_experience = queryhashpool("experience");

		ammotype *am = NULL;

		am = &ammotypes[amm_mana];
		if (!am->key)
		{
			WARNINGF("mana ammotype declared implicitly!");
			am->key = amm_mana;
			am->name = newstring("mana");
		}

		am = &ammotypes[amm_health];
		if (!am->key)
		{
			WARNINGF("health ammotype declared implicitly!");
			am->key = amm_health;
			am->name = newstring("health");
		}

		am = &ammotypes[amm_experience];
		if (!am->key)
		{
			WARNINGF("experience ammotype declared implicitly!");
			am->key = amm_experience;
			am->name = newstring("experience");
		}
	}

	void load()
	{
		loadammo();
	}

}
