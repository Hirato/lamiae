#include "rpggame.h"

using namespace rpgscript;
using namespace game;

namespace rpggui
{
	struct tradestack
	{
		rpgent *owner;
		vector<item *> items;

		item *add(item *it, int q)
		{
// 			conoutf("attempting to add item %p", it);
			vector<item *> *stack = NULL;
			if(owner->type() == ENT_CHAR)
				stack = &((rpgchar *) owner)->inventory.access(it->base, vector<item *>());
			else
				stack = &((rpgcontainer *) owner)->inventory.access(it->base, vector<item *>());

			if(stack->find(it) == -1) return NULL;
// 			conoutf("found item in owner inventory");

			item *cmp = NULL;
			loopv(items) if(items[i]->compare(it))
				cmp = items[i];
			if(!cmp)
			{
				cmp = items.add(new item());
				it->transfer(*cmp);
				cmp->quantity = 0;
			}

			int diff = clamp(q, -cmp->quantity, it->quantity);

			cmp->quantity += diff;
			it->quantity -= diff;

// 			conoutf("successfully added item %p as %p", it, cmp);
			return cmp;
		}

		void remove(item *it, int q)
		{
			if(items.find(it) == -1) return;

			q = clamp(q, 0, it->quantity);
			int amnt = it->quantity;

			it->quantity = q;
			owner->additem(it);
			it->quantity = amnt - q;
		}

		float value(const merchant &mer, bool buy) const
		{
			float val = 0;
			loopv(items)
			{
				item *it = items[i];
				if(mer.currency == it->base)
					val += it->quantity * it->value;
				else
				{
					merchant::rate exch = mer.getrate(*player1, items[i]->category);
					val += items[i]->value * items[i]->quantity * (buy ? exch.buy : exch.sell);
				}
			}
			return ceil(val);
		}

		void give(rpgent *t)
		{
			loopv(items)
			{
				t->additem(items[i]);
				rpgscript::removeminorrefs(items[i]);
			}

			items.deletecontents();
		}
		inline void refund() { give(owner); }

		tradestack(rpgent *owner) : owner(owner) {}
		~tradestack() { refund(); }
	};

	tradestack *sellstack = NULL, *buystack = NULL;

	ICOMMAND(r_buystack_get, "s", (const char *ref),
		if(!*ref) return;
		reference *stack = searchstack(ref, true);
		stack->setnull();
		if(buystack) stack->pushref(&buystack->items);
	)

	ICOMMAND(r_buystack_value, "", (),
		if(!sellstack || !buystack) { intret(0); return; }
		merchant *mer = NULL;
		if(buystack->owner->type() == ENT_CHAR)
			mer = ((rpgchar *) buystack->owner)->merchant;
		else
			mer = ((rpgcontainer *) buystack->owner)->merchant;

		intret(buystack->value(*mer, true));
	)

	ICOMMAND(r_buystack_add, "si", (const char *ref, int *q),
		if(!buystack) return;

		reference *items = searchstack(ref, false);
		if(!items) return;

		const char *sep = strchr(ref, ':');
		int idx = sep ? parseint(++sep) : 0;

		if(sep)
		{
			if(items->getinv(idx)) buystack->add(items->getinv(idx), *q);
		}
		else loopv(items->list)
		{
			if(items->getinv(i)) buystack->add(items->getinv(idx), *q);
		}
	)

	ICOMMAND(r_buystack_remove, "si", (const char *ref, int *q),
		if(!buystack) return;

		reference *items = searchstack(ref, false);
		if(!items) return;

		const char *sep = strchr(ref, ':');
		int idx = sep ? parseint(++sep) : 0;

		if(sep)
		{
			if(items->getinv(idx)) buystack->remove(items->getinv(idx), *q);
		}
		else loopv(items->list)
		{
			if(items->getinv(i)) buystack->remove(items->getinv(idx), *q);
		}
	)

	ICOMMAND(r_sellstack_get, "s", (const char *ref),
		if(!*ref) return;
		reference *stack = searchstack(ref, true);
		stack->setnull();
		if(sellstack) stack->pushref(&sellstack->items);
	)

	ICOMMAND(r_sellstack_value, "", (),
		if(!sellstack || !buystack) { intret(0); return; }
		merchant *mer = NULL;
		if(buystack->owner->type() == ENT_CHAR)
			mer = ((rpgchar *) buystack->owner)->merchant;
		else
			mer = ((rpgcontainer *) buystack->owner)->merchant;

		intret(sellstack->value(*mer, false));
	)

	ICOMMAND(r_sellstack_add, "si", (const char *ref, int *q),
		if(!sellstack) return;

		reference *items = searchstack(ref, false);
		if(!items) return;

		const char *sep = strchr(ref, ':');
		int idx = sep ? parseint(++sep) : 0;

		if(sep)
		{
			if(items->getinv(idx)) sellstack->add(items->getinv(idx), *q);
		}
		else loopv(items->list)
		{
			if(items->getinv(i)) sellstack->add(items->getinv(idx), *q);
		}
	)

	ICOMMAND(r_sellstack_remove, "si", (const char *ref, int *q),
		if(!sellstack) return;

		reference *items = searchstack(ref, false);
		if(!items) return;

		const char *sep = strchr(ref, ':');
		int idx = sep ? parseint(++sep) : 0;

		if(sep)
		{
			if(items->getinv(idx)) sellstack->remove(items->getinv(idx), *q);
		}
		else loopv(items->list)
		{
			if(items->getinv(i)) sellstack->remove(items->getinv(idx), *q);
		}
	)

	ICOMMAND(r_dotrade, "", (),
		if(!sellstack || !buystack) { intret(0); return; }
		merchant *merch = NULL;
		if(buystack->owner->type() == ENT_CHAR)
			merch = ((rpgchar *) buystack->owner)->merchant;
		else
			merch = ((rpgcontainer *) buystack->owner)->merchant;

		int buy = buystack->value(*merch, true);
		int sell = sellstack->value(*merch, false);

		//make me a better offer
		if(buy > sell + merch->credit) { intret(0); return; }

		merch->credit += sell - buy;
		buystack->give(sellstack->owner);
		sellstack->give(buystack->owner);

		intret(1);
	)

	bool open()
	{
		return !editmode && connected && curmap && UI::numui();
	}

	void forcegui()
	{
		if(editmode || !connected)
		{
			UI::hideui("chat");
			UI::hideui("trade");
			return;
		}

		if(UI::numui() > 1 && (talker->getent(0) || trader->getent(0)))
			UI::hideallui();

		if((!buystack && trader->getent(0)) || (buystack && buystack->owner != trader->getent(0)))
		{
			DELETEP(sellstack);
			DELETEP(buystack); //set to NULL

			if(trader->getchar(0) || trader->getcontainer(0))
			{
				buystack = new tradestack(trader->getent(0));
				sellstack = new tradestack(player1);
				rpgexecute("showtrade");
				rpgexecute("refreshtrade");
			}
		}

		if(talker->getent(0) && !UI::uivisible("chat"))
			rpgexecute("showchat");
		if(trader->getent(0) && !UI::uivisible("trade"))
			trader->setnull(true);
	}

	void refreshgui()
	{
		if(UI::uivisible("chat"))
			rpgexecute("refreshchat");
		else if(UI::uivisible("trade"))
			rpgexecute("refreshtrade");
	}

	ICOMMAND(r_get_dialogue, "", (),
		if(!talker || !talker->getent(0)) return;
		script *scr = talker->getent(0)->getscript();
		if(scr->curnode) result(scr->curnode->str);
	)

	ICOMMAND(r_num_response, "", (),
		if(!talker || !talker->getent(0)) {intret(0); return;}
		script *scr = talker->getent(0)->getscript();
		if(scr->curnode) intret(scr->curnode->choices.length());
		else intret(0);
	)

	ICOMMAND(r_get_response, "i", (int *n),
		if(!talker || !talker->getent(0)) {result(""); return;}
		script *scr = talker->getent(0)->getscript();
		if(scr->curnode && scr->curnode->choices.inrange(*n))
			result(scr->curnode->choices[*n]->talk);
		else
			result("");
	)

	void chattrigger(int n)
	{
		if(!talker || !talker->getent(0)) return;
		script *scr = talker->getent(0)->getscript();

		if(scr->curnode && scr->curnode->choices.inrange(n))
		{
			dialogue *cur = scr->curnode;
			scr->curnode = NULL;

			if(cur->choices[n]->dest[0])
			{
				scr->curnode = scr->chat.access(cur->choices[n]->dest);
				if(!scr->curnode)
					ERRORF("no such dialogue node: %s", cur->choices[n]->dest);
			}

			uint *code = cur->choices[n]->script;
			keepcode(code);
			rpgexecute(code);
			freecode(code);

			cur->close();

			if(!talker || !talker->getent(0))
				scr->curnode = NULL;
			else
				scr = talker->getent(0)->getscript();

			if(scr->curnode)
			{
				//flush it, just in case...
				//this is probably taken care of in r_response...
				scr->curnode->close();
				scr->curnode->open();

				if(!scr->curnode->choices.length())
				{
					//if(DEBUG print something
					//there are no destinations so just print the text and close...
					game::hudline("%s: %s", talker->getent(0)->getname(), scr->curnode->str);
					scr->curnode->close();
					scr->curnode = NULL;
				}
			}
			if(!scr->curnode)
			{
				talker->setnull(true);
				UI::hideui("chat");
			}
			else
				refreshgui();
		}
	}

	ICOMMAND(r_trigger_response, "i", (int *n),
		chattrigger(*n);
	)

	bool hotkey(int n)
	{
		if(talker->getent(0))
		{
			chattrigger(n);
			return true;
		}
		return false;
	}
}
