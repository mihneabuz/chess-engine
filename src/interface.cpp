#include <iostream>
#include <string>
#include <map>
#include "evaluate.h"
#include "interface.h"
#include "logger.h"
#include "boardstate.h"
#include "move.h"
#include "move_gen.h"
#include "transpositions.h"
#include "zobrist.h"

#define UNUSED(x) (void)(x) // mark args as redundant to silence compiler warnings
#define output std::cout
#define feature_args "feature variants=\"3check\" sigint=0 san=0 name=1 myname=\"FriedLiver\" done=1\n"
#define MAX_DEPTH 6

std::map<std::string, void (*)(std::string args)> commands;
Boardstate game;
bool forcing = false;
uint32_t time_remaining;

bool is_move(std::string str) {
	/* From gnu chess interface
	standard:
	Normal moves:	e2e4
	Pawn promotion:	e7e8q
	Castling:	e1g1, e1c1, e8g8, e8c8
	non-standard:
	Bughouse/crazyhouse drop:	P@h3 - not needed, bughouse variant disabled
	ICS Wild 0/1 castling:	d1f1, d1b1, d8f8, d8b8 - not needed, wild is disabled
	FischerRandom castling:	O-O, O-O-O (oh, not zero) - not needed, not FischerRandom
	Multi-leg move:	c4d5,d5e4 (legs separated by comma) - not needed, no mlm
	Null move:	@@@@ - possibly needed to be compatible to analysis mode
	*/

	// normal moves are 4 chars, pawn promotions are 5
	if (str.length() < 4 || str.length() > 5) {
		return false;
	}
	 // null move, used in analysis mode to change whose move should be next
	if(str.compare("@@@@") == 0) {
		return true;
	}
	// the file of the source
	if (str[0] < 'a' || str[0] > 'h') {
		return false;
	}
	// the rank of the source
	if (str[1] < '1' || str[1] > '8') {
		return false;
	}
	// the file of the destination
	if (str[2] < 'a' || str[2] > 'h') {
		return false;
	}
	// the rank of the destination
	if (str[3] < '1' || str[3] > '8') {
		return false;
	}
	// in case of pawn promotion, the 5th char has to be a piece name
	if (str.length() == 5) {
		return str[4] == 'q' || str[4] == 'n' || str[4] == 'b' || str[4] == 'r';
	}
	return true;
};

void quit(std::string args) {
	UNUSED(args);
	exit(0);
}

void protover(std::string args) {
	int a = std::stoi(args.substr(args.find(' ')));
	if (a != 2) {
		exit(-1);
	}
	init_move_tables();
	init_eval_tables();
	init_zobrist_table(0x0);
	output << feature_args;
}

void new_game(std::string args) {
	UNUSED(args);
	game.reset();
	clear_trans_table();
	forcing = false;
	log(game.get_state());
}

void move(std::string args) {
	// get move poz index
	square old_poz = ('h' - args[0]) + (args[1] - '1') * 8;
	square new_poz = ('h' - args[2]) + (args[3] - '1') * 8;

	// get piece type
	piece p = game.get_piece(old_poz);

	// check for promotion
	piece promotion = p;
	if (args.length() == 5) {
		switch (args[4]) {
			case 'q': promotion = QUEEN; break;
			case 'r': promotion = ROOK; break;
			case 'b': promotion = BISHOP; break;
			case 'n': promotion = KNIGHT; break;
		}
	}

	// set flags
	uint8_t flags = 0;
	if (game.get_piece(new_poz) != NULL_PIECE)     					flags |= CAPTURE;
	else if (game.is_castle(old_poz, new_poz, p))  					flags |= CASTLE;
	else if (game.is_enpass(old_poz, new_poz, p))  					flags |= ENPASSANT;
	else if (new_poz == game.enpassant)            					flags |= ENPASSANT + CAPTURE;
	if ((p == ROOK && game.is_rk_init_sq(old_poz)) || p == KING)  	flags |= UNCASTLE;

	// create move
	Move m = encode(old_poz, new_poz, p, promotion, flags);

	// pass move to gamestate
	if (game.player_move(m, forcing)) {
		log(game.get_state());

		// tell engine to make a move
		if (not forcing) {
			std::string engine_move = game.engine_move(time_remaining, MAX_DEPTH);
			log("Engine move " + engine_move);
			output << engine_move;
			log(game.get_state());
		}
	}
	else
		output << "Illegal move: " + args + "\n";
}

void force(std::string args) {
	UNUSED(args);
	forcing = true;
}

void go(std::string args) {
	UNUSED(args);
	forcing = false;

	// tell engine to make a move
	std::string engine_move = game.engine_move(time_remaining, MAX_DEPTH);
	log("Engine move " + engine_move);
	output << engine_move;
	log(game.get_state());
}

void resign(std::string args) {
	UNUSED(args);
	output << "resign\n";
}

void time(std::string args) {
	time_remaining = std::stoi(args.substr(args.find(' ')));
	log("TIME: " + std::to_string(time_remaining));
}

void init_interface() {
	commands["protover"] = protover;
	commands["new"] = new_game;
	commands["force"] = force;
	commands["go"] = go;
	commands["quit"] = quit;
	commands["resign"] = resign;
	commands["move"] = move;
	commands["time"] = time;
}

void execute(std::string cmd, std::string args) {
	// args is like argv[], it includes cmd, basically its the entire line of the command

	if (is_move(cmd)) {
		execute("move", args);
		return;
	}

	auto to_execute = commands[cmd];
	if (to_execute != nullptr) {
		log("-> executing: " + args);
		to_execute(args);
	}
}
