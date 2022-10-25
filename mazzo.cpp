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

#include "mazzo.h"

/*	Lancia un invalid argument se e e' NULL
	PARAMETRI DI INPUT:
		e: helper per personalizzare il comportamento
 */
mazzo::mazzo(elaboratoreCarte *e) {
	if (e==NULL)
		throw invalid_argument("Chiamata a mazzo::mazzo() con e==NULL");
	elaboratore=e;
	mischia();
}

mazzo::~mazzo() {
	delete elaboratore;
	carte.clear();
}

/* Restituisce il valore numerico della prima carta presente nel mazzo
	Lancia un underflow error se il mazzo e' vuoto
 */
size_t mazzo::getCarta() {
	if (carte.size()==0)
		throw underflow_error("Chiamato mazzo::getCarta con carte.size()==0");
	size_t c=*(carte.end()-1);
	carte.pop_back();
	return c;
}

/* Mischia il mazzo sfruttando l'algoritmo dell'helper.
	Restituisce invalid_argument se il mazzo no e' vuoto
 */
void mazzo::mischia() {
	if (carte.size()!=0)
		throw invalid_argument("Chiamato mazzo::mischia con carte.size()=="+stringHelper::IntToStr(carte.size()));
	else
		try {
			while(true) {
				carte.push_back(elaboratore->getCarta());
			}
		} catch (overflow_error &e) { //sono finite le carte
			;
		}
}

/* Restituisce la carta di indice quale presente nel mazzo
	Restituisce un range_error se quale non e' nei valori accettabili
 */
size_t mazzo::getCarta(size_t quale) {
	size_t c, j;
	vector<size_t>::iterator i;
	if (quale>=carte.size() || carte.size()==0)
		throw range_error("Chiamato mazzo::getCarta con quale="+stringHelper::IntToStr(quale)+" e carte.size="+stringHelper::IntToStr(carte.size()));
	for (i=carte.begin(), j=0; j<quale; j++, i++);
	c=*i;
	carte.erase(i);
	return c;
}
