Cutscenes do not work like the rest of the game module, IT is designed to be be easily integrated into the other game modules with msot functionality.
The cutscenes are scripted in the local files and the files can have any name, as long as they end with .cfg.

Important Notes
* all elements have a successors field, this is executed when the element dies
* successors inherent the container and interpolation function of the parent
* new actions are always added to the end of the parent list
* containers can be used to defer things, specifically if you're doing slides and want test/subtitles on top
* cutscenes are not loaded into memory until they are executed
* cutscenes have an optional post field which can be used to execute scripts when the cutscene ends
* all cutscenes can be skipped, without exception
* the player is subject to the whims of the AI during cutscenes; see r_action commands

see the wiki for more detailed information

variables (note: top and left are 0)
hud_right - scaled coordinate for the right side of the screen
hud_bottom - scaled coordinate for the bottom of the screen

generic commands
cs_start file[s]
cs_interrupt
cs_post body[e]

interpolation functions
cs_interp_linear
cs_interp_dlinear
cs_interp_sin
cs_interp_sin2
cs_interp_cos
cs_interp_cos2
cs_interp_one

actions
cs_action_generic	delay[i] post[e]
cs_action_moveldeta	x[f] y[f] z[f] delay[i] post[e]
cs_action_moveset	x[f] y[f] z[f] delay[i] post[e]
cs_action_movecamera	tag[i] delay[i] post[e]
cs_action_moveent	ref[r] delay[i] post[e]
cs_action_viewdelta	y[f] p[f] r[f] delay[i] post[e]
cs_action_viewset	y[f] p[f] r[f] delay[i] post[e]
cs_action_viewcamera	tag[i] delay[i] post[e]
cs_action_viewent	ref[r] delay[i] post[e]
cs_action_focus		ref[r] interp[f] lead[f] delay[i] post[e]
cs_action_follow	ref[r] interp[f] tail[f] height[f] delay[i] post[e]
cs_action_viewport	ref[r] delay[i] post[e]

containers
cs_container_generic	children[e] delay[i] post[e]
cs_container_translate	x[f] y[f] children[e] delay[i] post[e]
cs_container_scale	x[f] y[f] children[e] delay[i] post[e]
cs_container_rotate	angle[f] children[e] delay[i] post[e]

elements
cs_element_text		x[f] y[f] width[f] colour[i] string[s] delay[i] post[e]
cs_element_image	x[f] y[f] dx[f] dy[f] colour[i] path[s] delay[i] post[e]
cs_element_solid	x[f] y[f] dx[f] dy[f] colour[i] modulate[i] delay[i] post[e]

AI directives
r_action_clear		ent[r]
r_action_attack		ent[r] victim[r] priority[i]
r_action_move		ent[r] location[i] priority[i]
r_action_wander		ent[s] location[i] radius]i] priority[i]
