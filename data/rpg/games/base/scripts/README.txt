//slots
// - none
//commands
// - r_script_say text[s] body[e]
//	Add a dialogue item in which an entity communicates the text to the player at some point
//	convention wise, place non-dialogue inside [] and use ** for emphasis
//	eg [The shaman wears a large and colourful mask, ornated with beads and symbols. You catch his attention and his piercing eyes fixate upon you] Why have *you* come to me?
//	- r_response text[s] dst[i] script[e]
//		Adds a response for the player
// - r_script_signal signal[s] body[e]
//	Registers a signal slot onto a script and overwrites the script if it already exists
