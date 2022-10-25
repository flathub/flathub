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

#ifndef _CARTA_H_
#define _CARTA_H_

#include "cartaHelper.h"
#include "stringHelper.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <wx/image.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/utils.h> 

using namespace std;

/* Questa classe rappresenta una generica carta da gioco.
   Si basa su cartahelper al fine di ottenere alcuni algoritmi specifici per il gioco in questione (in questo caso la briscola).
   Dato che le carte sono le stesse per ogni mazzo, questa classe non e' istanziabile dall'esterno e l'unica cosa modificabile e' l'immagine della stessa.
*/



class carta {
	private:
		size_t seme, //seme della carta
			   valore, //valore della carta
			   punteggio; //punteggio della carta
		wxString semeStr; //seme della carta in formato leggibile dall'uomo
		static cartaHelper *helper; //aiutante che contiene algoritmi specifici per il gioco in questione
		carta(size_t n);
		~carta();
		wxImage *img; //immagine della carta
		static vector<carta *> carte; //vettore che contiene le carte preistanziate
		static wxString path, nomeMazzo;  //path delle immagini e nome del mazzo
		size_t TipoCarta;
	public:
		/*
		 Istanzia un numero ben definito di carte.
		 Parametri:
			INPUT:
				n: numero delle carte da istanziare
				h: classe aiutante che contiene gli algoritmi specifici per il tipo di gioco
				nomeMazzo: nome che corrisponde alla cartella all'interno della quale si trovano le immagini
		 Lancia l'eccezione logic_error se viene chiamata per istanziare un nuovo mazzo di carte o se il parametro h e' null.
		 Rilancia l'eccezione invalid_argument se il parametro nomeMazzo non e' valido.
		 */
		static void inizializza(size_t n, cartaHelper *h, wxString nomeMazzo);
		/*
		 Restituisce una carta dal mazzo senza toglierla dal vettore
		 Parametri:
			INPUT:
				n: posizione della carta da prendere
		 Lancia un overflow_error se la posizione indicata non esiste
		 */
		static carta *const getCarta(size_t quale);
		/*
		 Carica le immagini delle carte.
		 PARAMETRI:
			Input:
				mazzo: cartella all'interno della quale si trovano le carte
		 Lancia un invalid_argument se il parametro mazzo non e' valido
		 */
		static void caricaImmagini(wxString mazzo);
		//dealloca tutte le carte esistenti. Da chiamare prima di una nuova chiamata ad inzializza
		static void dealloca();
		//restituisce il seme della carta
		size_t getSeme() {return seme;}
		//restituisce il valore della carta
		size_t getValore() {return valore;}
		//restituisce il punteggio della carta
		size_t getPunteggio() {return punteggio;}
		//restituisce la posizione nel vettore relativa alla carta
		size_t getNumero() {return helper->getNumero(seme, valore);}
		//restituisce il valore della carta in formato stringa
		const wxString getValoreStr();
		//restituisce il seme della carta in formato stringa
		const wxString getSemeStr() {return semeStr;}
		/*imposta l'immagine di una singola carta
		 PARAMETRI:
			Input: path - cartella nella quale si trova l'immagine. Deve essere valida
		 */
		void setImmagine(wxString path) {
			if (img!=NULL)
				delete img;
			img=new wxImage(path);
		}
		//Restituisce l'immagine della carta
		const wxImage *getImmagine() {return img;}
		/*Restituisce il valore true se le due carte hanno lo stesso seme. False altrimenti.
		 Parametri:
			INPUT:
				c1: carta con cui confrontare
		 */
		bool stessoSeme(carta *c1) {return seme==c1->getSeme();}
		friend ostream& operator<<(ostream &s, carta &c);
		friend bool operator<(carta &c, carta &c1);
		//Restitisce l'altezza dell'immagine di cui e' dotata la carta
		static wxCoord getAltezzaImmagine() {return carte[0]->img->GetHeight();}
		//restituisce la larghezza dell'immagine di cui e' dotata la carta
		static wxCoord getLarghezzaImmagine() {return carte[0]->img->GetWidth();}
		//Restituisce l'indirizzo della cartella Mazzi
		static wxString getPathMazzi() {return path;}
		//Restituisce l'indirizzo della cartella conentenete le carte
		static wxString getPathCarte() {return path+nomeMazzo+wxFileName::GetPathSeparator();}
		//Restituisce il nome della cartella in cui sono contenute le carte
		static wxString getNomeMazzo() {return nomeMazzo;}
		/* Restituisce l'immagine di una carta
			PARAMETRI:
				Input: quale - indice della carta di cui si vuole l'immagine*/
		static const wxImage* getImmagine(size_t quale);
		/* Restituisce il seme della carta sotto forma di stringa.
			PARAMETRI:
				Input: quale - indice della carta di cui si vuole il seme*/
		static wxString getSemeStr(size_t quale);

		size_t getTipo();
};

#endif
