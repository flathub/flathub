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

#ifndef _CARTA_HELPER_H_
#define _CARTA_HELPER_H_

#include <iostream>
#include <stdexcept>
#include <wx/string.h>

/* Questa classe rappresenta una classe che incapsula gli algoritmi di gioco specifici per il gioco scelto.
	e' usata da carta per, ad esempio, restituire il seme sotto forma di stringa, per restituire il punteggio della carta e cosi' via*/

using namespace std;

class cartaHelper {
	protected:
		cartaHelper() {;}
	public:
		//Enumerazione usata per comparare due carte
		enum RISULTATI_COMPARAZIONE {UGUALI=0, MAGGIORE_LA_PRIMA=1, MAGGIORE_LA_SECONDA=2};
		/* Restituisce il seme della carta sotto forma di numero intero.
			PARAMETRI:
				Input: carta - e' un intero che rappresenta il numero della carta. Sara' anche l'indice a cui corrispondera' l'elemento carta nel vettore
						delle carte
			Restituisce un invalid_argument se il numero risulta non valido.*/
		virtual size_t getSeme(size_t carta) = 0;
		/* Restituisce valore facciale della carta
		 PARAMETRI:
		 Input: carta - e' un intero che rappresenta il numero della carta. Sara' anche l'indice a cui corrispondera' l'elemento carta nel vettore
		 delle carte
		 Restituisce un invalid_argument se il numero risulta non valido.*/
		virtual size_t getValore(size_t carta) =0;
		/* Restituisce il punteggio associato alla carta
		 PARAMETRI:
		 Input: carta - e' un intero che rappresenta il numero della carta. Sara' anche l'indice a cui corrispondera' l'elemento carta nel vettore
		 delle carte
		 Restituisce un invalid_argument se il numero risulta non valido.*/
		virtual size_t getPunteggio(size_t carta) =0;
		/* Restituisce il seme della carta sotto forma di stringa.
		 PARAMETRI:
		 Input: carta - e' un intero che rappresenta il numero della carta. Sara' anche l'indice a cui corrispondera' l'elemento carta nel vettore
		 delle carte
		 Restituisce un invalid_argument se il numero risulta non valido.*/
		virtual wxString getSemeStr(size_t carta, size_t tipo) =0;
		virtual wxString getSemeStrItaliana(size_t carta) =0;
		virtual wxString getSemeStrFrancese(size_t carta) =0;

		/* Restituisce il numero corrispondente ad una carta
		 PARAMETRI:
		 Input:
			seme - intero associato al seme di una carta
			valore - valore facciale di una carta
		 Restituisce un invalid_argument se seme o valore risulta non valido.*/
		virtual size_t getNumero(size_t seme, size_t valore) =0;
		/* Compara il numero di una carta col numero di un'altra
			PARAMETRI:
				Input:
					carta: numero della prima carta
					carta1: numero della seconda carta
		 */

		virtual ~cartaHelper() {};
		virtual RISULTATI_COMPARAZIONE compara(size_t carta, size_t carta1)=0;
};

#endif
