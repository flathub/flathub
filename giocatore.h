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

#ifndef _GIOCATORE_H_
#define _GIOCATORE_H_

#include "carta.h"
#include "mazzo.h"
#include "giocatoreHelper.h"
#include "stringHelper.h"

#include <wx/dcbuffer.h>
#include <vector>

/** Classie che identifica generalmente un qualsiasi giocatore */

class giocatore {
	private:
		wxString nome; //nome del giocatore
		vector<carta *> mano; //carte in suo possesso
		bool ordinaMano; //se le carte in mano al giocatore devono essere ordinate o se devono essere messe cosi' come escono
		size_t numeroCarte, //numero di carte in mano al giocatore
			   iCartaGiocata, //indice della carta giocata dal giocatore
			   punteggio; //punteggio costantemente aggiornato del giocatore
		giocatoreHelper *helper; //helper per personalizzare il comportamento della classe
	public:
		enum CARTA_GIOCATA {NESSUNA_CARTA_GIOCATA=static_cast<size_t> (-1)}; //come indice per indicare che non e' stata ancora effettuata la giocata si usa il massimo valore rappresentabile nel size_t
		giocatore(giocatoreHelper *h, wxString n=wxT(""), bool ordina=true, size_t carte=3);
		~giocatore();
		wxString& getNome() {return nome;}
		void setNome(wxString n) {nome=n;}
		bool getFlagOrdina() {return ordinaMano;} //se la mano deve essere ordinata o meno
		void setFlagOrdina(bool ordina) {ordinaMano=ordina;}
		void addCarta(mazzo *m); //aggiunge una carta alla mano del giocatore
		carta *getCartaGiocata(); //restituisce la carta giocata
		size_t getPunteggio() {return punteggio;}
		wxString getPunteggioStr() {return stringHelper::IntToWxStr(punteggio);} //restituisce il punteggio sotto forma di stringa
		void gioca(int i); //gioca una carta col giocatore primo di mano
		void gioca(giocatore *g1, int i); //gioca una carta col giocatore secondo di mano
		bool hasCartaGiocata() {return iCartaGiocata!=NESSUNA_CARTA_GIOCATA;} //se il giocatore ha giocato
		void aggiornaPunteggio(giocatore *g);
		wxPoint paint(wxPaintDC &dc, wxCoord c) {return helper->paint(dc, nome, mano, iCartaGiocata);} //disegna i dati del giocatore sul frame
		bool stessoSemeCartaGiocata(giocatore *g); //se la carta giocata dal giocatore ha lo stesso seme
		bool stessoSeme(carta *c); //confronta due carte per sapere se hanno lo stesso seme
};
#endif
