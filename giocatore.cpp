/**********************************************************************************
 *   Copyright (C) 2022 by Giulio Sorrentino                                      *
 *   gsorre84@gmail.com                                                           *
 *                                                                                *
 *   This program is free software; you can redistribute it and/or modify         *
 *   it under the terms of the GNU Lesser General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or            *
 *   (at your option) any later version.                                          *
 *                                                                                *
 *   This program is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
 *   GNU General Public License for more details.                                 *
 *                                                                                *
 *   You should have received a copy of the GNU General Public License            *
 *   along with this program; if not, write to the                                *
 *   Free Software Foundation, Inc.,                                              *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                    *
 **********************************************************************************/

#include "giocatore.h"
#include <iostream>
/* PARAMETRI DI INPUT:
	h: helper per personalizzare i comportamenti della classe. Viene deallocato automaticamente alla cancellazione della classe. Non deve essere NULL
	n: nome del giocatore
	ordina: indica se le carte in mano devono essere ordinate in qualche modo
	carte: numero massimo di carte che il giocatore puo' avere in mano */
giocatore::giocatore(giocatoreHelper *h, wxString n,  bool ordina, size_t carte) : ordinaMano(ordina), numeroCarte(carte), iCartaGiocata(NESSUNA_CARTA_GIOCATA), punteggio(0) {
	helper=h;
	nome=n;
}

giocatore::~giocatore() {
	delete helper;
	mano.clear();
}


/* Aggiunge una carta alla mano del giocatore.
	PARAMETRI DI INPUT:
		m: mazzo da cui prendere la carta
	Lancia un overflow_error se il numero di carte che ha in mano e' gia' il massimo, rilancia un underflow error se il mazzo non ha piu' carte ed il giocatore ha finito le carte.
 */
void giocatore::addCarta(mazzo *m) {
	if (mano.size()==numeroCarte && iCartaGiocata==NESSUNA_CARTA_GIOCATA)
		throw overflow_error("Chiamato giocatore::setCarta con mano.size()==numeroCarte=="+stringHelper::IntToStr(numeroCarte));
	carta *c;
	if (iCartaGiocata!=NESSUNA_CARTA_GIOCATA) {
		size_t i;
		vector<carta *>::iterator j;
		for (i=0, j=mano.begin(); i<iCartaGiocata; i++, j++); //si scorre tutto il mazzo alla ricerca della carta giocata
		mano.erase(j); //cancella la carta giocata
		iCartaGiocata=NESSUNA_CARTA_GIOCATA;
	}
	try {
		c=carta::getCarta(m->getCarta());
	} catch (underflow_error &e) { //se il mazzo non ha piu' carte
		numeroCarte--;
		if (numeroCarte==0) //se sono finite le carte
			throw;
		return;
	}
	if (!ordinaMano)
		mano.push_back(c);
	else {
		vector<carta *>::iterator i;
		for (i=mano.begin(); i!=mano.end() && **i<*c; i++); //insertion sort
		mano.insert(i, c);
	}

}

/* Da richiamare quando il giocatore che deve giocare e' primo di mano.
	PARAMETRI DI INPUT:
 i e' puramente fittizio e va impostato nell'indice della carta da giocare dal giocatore utente quando e' primo di mano. Se e' il pc non viene considerato
 */
void giocatore::gioca(int i) {
	iCartaGiocata=helper->gioca(mano, i);
}

/* Da richiamare quando il giocatore che deve giocare e' il secondo di mano.
	PARAMETRI DI INPUT:
	g1: giocatore che ha giocato come primo di mano
 i e' puramente fittizio e va impostato nell'indice della carta da giocare dal giocatore utente quando e' primo di mano. Se e' il pc non viene considerato
 */
void giocatore::gioca(giocatore *g1, int i) {
	iCartaGiocata=helper->gioca(mano, g1->getCartaGiocata(), i);
}

/*Restituisce la carta giocata dal giocatore
	Lancia un range error se il giocatore non ha giocato
*/
carta *giocatore::getCartaGiocata() {
	if (iCartaGiocata==NESSUNA_CARTA_GIOCATA)
		throw range_error("Chiamato giocatre::getCartaGiocata() quando non ci sono carte giocate");
	return mano[iCartaGiocata];
}

/* Aggiorna il punteggio del giocatore corrente.
	PARAMETRI DI INPUT:
		g: il giocatore avversario, viene usato per sapere la carta giocata dall'altro giocatore. Non dev'essere NULL, pena un range_error.
	Lancia un range_error se uno dei due giocatori non ha giocato */
void giocatore::aggiornaPunteggio(giocatore *g) {
	if (g==NULL)
		throw range_error("Chiamata a giocatore::aggiornaPunteggio con g==NULL");
	if (iCartaGiocata==NESSUNA_CARTA_GIOCATA)
		throw range_error("Chiamata a giocatore::aggiornaPunteggio con iCartaGiocata==NESSUNA_CARTA_GIOCATA");
	punteggio+=helper->getPunteggio(getCartaGiocata(), g->getCartaGiocata());
}

/* Per sapere se le due carte giocate hanno lo stesso seme.
	Lancia un invalid argument se uno dei due giocatori non ha giocato
 Restituisce true se le due carte giocate hanno lo stesso seme, false altrimenti*/
bool giocatore::stessoSemeCartaGiocata(giocatore *g) {
	if (iCartaGiocata==NESSUNA_CARTA_GIOCATA || g->iCartaGiocata==NESSUNA_CARTA_GIOCATA)
		throw invalid_argument("Chiamata a giocatore::stessoSemeCartaGiocata con almeno 1 carta giocata mancante");
	return stessoSeme(g->getCartaGiocata());
}

/* Per sapere se una carta ha lo stesso seme della carta giocata da questo giocatore.
 PARAMETRI DI INPUT:
	c: carta che si vuole confrontare con la giocata.
 Lancia un invalid argument se il giocatore non ha giocato.
 Restituisce true se le due carte hanno lo stesso seme, false altrimenti.
*/
bool giocatore::stessoSeme(carta *c) {
	if (iCartaGiocata==NESSUNA_CARTA_GIOCATA)
		throw invalid_argument("Chiamata a giocatore::stessoSeme con carta giocata mancante");
	return mano[iCartaGiocata]->getSeme()==c->getSeme();
}
