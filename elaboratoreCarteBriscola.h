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

#ifndef _ELABORATORE_CARTE_ITALIANE_H_
#define _ELABORATORE_CARTE_ITALIANE_H_

#include "elaboratoreCarte.h"
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

/* Un helper che elavora le care di briscola. Si puo' chiamare un massimo di 40 volte, vengono elaborati numeri pesudocasuali sempre diversi. */

class elaboratoreCarteBriscola : public elaboratoreCarte {
	private:
		static const size_t numeroCarte=40; //numero di carte complessive da elaborare
		vector<bool> doppione; //indice per verificare se la carta corrispettiva all'indice sia gia' uscito
		size_t cartaBriscola; //indica la carta di briscola
		bool inizio, //indica se la carta di briscola deve essere salvata o no
			 briscolaDaPunti; //indica se la carta che esce come briscola puo' dare punti
	public:
		elaboratoreCarteBriscola(bool punti=true);
		virtual ~elaboratoreCarteBriscola() {;}
		virtual size_t getCarta(); //elabora la carta, restituisce un overflow error se e' stato chiamato piu' volte del numero previsto
		size_t getCartaBriscola() {return cartaBriscola;} //restituisce il valore della carta di briscola
};

#endif
