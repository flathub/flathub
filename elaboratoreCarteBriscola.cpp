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

#include "elaboratoreCarteBriscola.h"

elaboratoreCarteBriscola::elaboratoreCarteBriscola(bool punti):inizio(true), briscolaDaPunti(punti) {
	doppione.assign(numeroCarte, false);
	srand(static_cast<unsigned int>(time(NULL)));
	cartaBriscola=0;
}

/* Restituisce la carta elaborata */
size_t elaboratoreCarteBriscola::getCarta() {
	size_t fine=rand()%numeroCarte, //indice per verificare se sono state elaborate tutte le carte o no
		carta=(fine+1)%numeroCarte; //carta da salvare
	while(doppione[carta] && carta!=fine) //fin quando non si trova uno spazio libero o non si finisce il ciclo
		carta=(carta+1)%numeroCarte; //si aumenta il valore della carta d 1, non si puo' elaborare un nuovo valore se no si rischia di finire in ciclo infinito
	if (doppione[carta]) //se non e' stato trovato uno spazio libero
		throw overflow_error("Chiamato elaboratoreCarteItaliane::getCarta() quando non ci sono piu' carte da elaborare");
	else {
		if (inizio) { //bisogna salvare il valore della carta di briscola
			size_t valore=carta%10;
			if (!briscolaDaPunti && (valore==0 || valore==2 || valore>6)) { //se la briscola non deve dare punti si assegna il 2 di quel seme
				carta=carta-valore+1;
			}
			cartaBriscola=carta;
			inizio=false;
		}
		doppione[carta]=true;
	}
	return carta;
}
