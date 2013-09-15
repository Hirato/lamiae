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
		am->reserved = true;
		if (!am->key)
		{
			WARNINGF("mana ammotype declared implicitly!");
			am->key = amm_mana;
			am->name = newstring("mana");
		}

		am = &ammotypes[amm_health];
		am->reserved = true;
		if (!am->key)
		{
			WARNINGF("health ammotype declared implicitly!");
			am->key = amm_health;
			am->name = newstring("health");
		}
	}

	void loadscripts()
	{
		script *scr = &scripts["null"];
		if(!scr->key)
		{
			WARNINGF("null script declared implicitly");
			scr->key = queryhashpool("null");
		}

		mapscript *mscr = &mapscripts["null"];
		if(!mscr->key)
		{
			WARNINGF("null mapscript declared implicitly");
			mscr->key = queryhashpool("null");
		}
	}

	void loadfactions()
	{
		faction *fac = &factions["player"];
		if(!fac->key)
		{
			WARNINGF("player faction declared implicitly");
			fac->key = queryhashpool("player");
		}
	}

	void load()
	{
		loadammo();
		loadscripts();
		loadfactions();
	}

}
