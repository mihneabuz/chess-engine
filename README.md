## Fried Liver

Motor de sah scris in C++
	- reprezentare bitboard
	- precalculated attack tables
	- magic bitboards
	- copy-make
	- piece-square tables evaluation
	- midgame/endgame
	- Minimax cu Alpha-Beta prunning
	- quiescence search

	Fisierele sursa principale:

	1. boardstate
	Contine reprezentarea interna a jocului. Tabla este formata din 15 bitboards (uint
	pe 64 biti), cate un bitboard pentru fiecare combinatie de piesa/culoare, cate unul 
	pentru toate piesele de o culoare, si unul pentru toate piesele de pe tabla.

	2. interface
	Face legatura dintre XBoard si reprezentarea interna. Citeste comenzile primite	de
	la XBoard si le executa printr-un std::map de la string la functie. De asemenea, 
	comenzile sunt preluate de logger si salvate in log.txt, impreuna cu alte informatii
	pentru debugging.

	3. move_gen
	Cuprinde functii care genereaza toate mutarile pseudo-legale. Mutarile sunt encodate
	ca un int pe 32 de biti. Mutarile sunt generate cu precalculated attack tables si
	magic bitboards pentru sliding pieces. Magic bitboards se gasesc in magics.h si	pot
	fi generate cu comanda "make generate_magics".

	4. search
	Contine algoritmul Minimax cu Alpha-Beta prunning care proceseaza mutarile
	generate de move_gen. La finalul seach-ului, se face un quiescence search care
	viseaza doar mutarile de capture. 

	5. evaluate
	Contine tabelele piece-square folosite pentru evaluare. Evaluarea este facuta
	progresiv si retinuta in boardstate, dar poate fi facuta si static pentru debug.
