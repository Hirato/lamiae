#include "rpggame.h"

namespace game
{
	void g3d_gamemenus()
	{

	}

	VARP(projallowlight, 0, 1, 1);
	VARP(editdrawgame, 0, 0, 1);
	void adddynlights()
	{
		if(!curmap || (editmode && !editdrawgame)) return;

		loopv(curmap->objs)
		{
			vec4 &light = curmap->objs[i]->temp.light;
			float radius = light.w;
			vec col = vec(light.x, light.y, light.z);

			if(light.magnitude() > 4)
			{
				rpgent *d = curmap->objs[i];

				vec pos = vec(d->yaw * RAD, d->pitch * RAD).mul(d->radius * 1.75);
				pos.z = 0;
				pos.add(d->o);

				adddynlight(pos, radius, col);
			}
		}

		if(!projallowlight)
			return;

		loopv(curmap->projs)
			curmap->projs[i]->dynlight();
		loopv(curmap->aeffects)
			curmap->aeffects[i]->dynlight();
	}

	void rendergame()
	{
		if(!curmap) return;

		if(editmode)
		{
			entities::renderentities();
			if(isthirdperson())
				player1->render();
		}

		if(!editmode || editdrawgame)
		{
			loopv(curmap->objs)
			{
				if(camera::cutscene ? (curmap->objs[i] != camera::attached ||
					fabs(camera::camera.o.x - curmap->objs[i]->o.x) > curmap->objs[i]->radius ||
					fabs(camera::camera.o.y - curmap->objs[i]->o.y) > curmap->objs[i]->radius
				) : (curmap->objs[i] != player1 || isthirdperson()))
				{
					curmap->objs[i]->temp.alpha = clamp(curmap->objs[i]->temp.alpha, 0, 1);
					curmap->objs[i]->render();

					if(DEBUG_ENT)
					{
						defformatstring(ds)("%p", curmap->objs[i]);
						particle_textcopy(curmap->objs[i]->abovehead(), ds, PART_TEXT, 1, 0xFFFFFF, 4);
					}
				}
			}

			loopv(curmap->projs)
			{
				curmap->projs[i]->render();

				if(DEBUG_PROJ)
				{
					defformatstring(ds)("%p", curmap->projs[i]);
					vec pos = vec(0, 0, 6).add(curmap->projs[i]->o);
					particle_textcopy(pos, ds, PART_TEXT, 1, 0xFFFFFF, 4);
				}
			}

			loopv(curmap->aeffects)
				curmap->aeffects[i]->render();
		}

		if(ai::dropwaypoints || DEBUG_AI)
		{
			ai::renderwaypoints();
		}
	}

	void renderavatar()
	{
		if(editmode) return;
		use_armour *left = NULL, *right = NULL;
		loopv(player1->equipped)
		{
			use_armour *cur = (use_armour *) player1->equipped[i]->it->uses[player1->equipped[i]->use];
			if(cur->slots & SLOT_LHAND)
				left = cur;
			if(cur->slots & SLOT_RHAND)
				right = cur;
		}

		//TODO attach hands
		//TODO place models at correct positions
		//TODO animations
		//TODO tags for particle emissions
		//for now just get it working
		if(left == right)
		{
			if(!left || !left->hudmdl) return;

			rendermodel(left->hudmdl, ANIM_HIDLE|ANIM_LOOP, player1->o, player1->yaw, player1->pitch, player1->roll, MDL_NOBATCH, NULL, NULL, player1->lastaction, 500);
		}
		else
		{
			if(left && left->hudmdl)
			{
				rendermodel(left->hudmdl, ANIM_HIDLE|ANIM_LOOP, player1->o, player1->yaw, player1->pitch, player1->roll, MDL_NOBATCH, NULL, NULL, player1->lastaction, 500);
			}
			if(right && right->hudmdl)
			{
				rendermodel(left->hudmdl, ANIM_HIDLE|ANIM_LOOP, player1->o, player1->yaw, player1->pitch, player1->roll, MDL_NOBATCH, NULL, NULL, player1->lastaction, 500);
			}
		}
	}

	const char *animname(int i)
	{
		i &= ANIM_ALL;
		if(i >= NUMANIMS) return "";
		return animnames[i];
	}

	const int numanims() {return NUMANIMS;}
}
