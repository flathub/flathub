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
#ifndef _MAZZO_H_
#define _MAZZO_H_

#include "elaboratoreCarte.h"
#include "stringHelper.h"

#include <vector>

class mazzo {
	private:
		string nome;
		vector<size_t> carte; //vettore delle carte
		void mischia(); //mischia il mazzo
		elaboratoreCarte *elaboratore; //helper per personalizzare il comportamento
	public:
		mazzo(elaboratoreCarte *e);
		size_t getNumeroCarte() {return carte.size();} //restituisce il numero di carte presenti nel mazzo
		wxString getNumeroCarteStr() {return stringHelper::IntToWxStr(carte.size());} //restituisce il numero di carte presenti nel mazzo sotto forma di stringa
		size_t getCarta(); //restituisce la prima carta sulla cima del mazzo
		size_t getCarta(size_t quale); //prene una carta a caso nel mazzo
		~mazzo();
};

#endif
