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

#include "giocatoreHelperUtente.h"

/* Richiamata quando il giocatore e' primo di mano.
	PARAMETRI IN INPUT:
		v: vettore delle carte in mano al giocatore
		iCarta: indice della carta da giocare
	Restituisce l'indice della carta da giocare (banalmente iCarta).
	Lancia un range_error se iCarta non e' un valore accettabile
*/
size_t giocatoreHelperUtente::gioca(const vector<carta *> &v, size_t iCarta) {
	if (v.size()==0)
		throw range_error("Chiamata a giocatoreHelperUtente::gioca(vector<carta *>, int iCarta) con v.size()==0");
	if (iCarta>=v.size() || iCarta<0)
		throw range_error("Chiamata a giocatoreHelperUtente::gioca con iCarta>mano.size");
	return static_cast<size_t> (iCarta);
}

/* Richiamata quando il giocatore e' secondo di mano
	PARAMETRI DI INPUT:
		v: vettore delle carte da giocare
		c: carta giocata dall'altro giocatore
		iCarta: indice della carta da giocare
	Restituisce l'indice della carta da giocare (banalmente iCarta)
	Lancia un range_error se iCarta non e' un valore accettabile
 */
size_t giocatoreHelperUtente::gioca(const vector<carta *> &v, carta *c, size_t iCarta) {
	if (iCarta>v.size() || iCarta<0)
		throw range_error("Chiamata a giocatoreHelperUtente::gioca con iCarta>mano.size");
	return static_cast<size_t> (iCarta);
}

/* Restituisce il punteggio delle carte
	PARAMETRI DI INPUT
		c, c1: le due carte giocate
	Lancia un range_error se c o c1 sono NULL
 */
size_t giocatoreHelperUtente::getPunteggio(carta *c, carta *c1) {
	if (c==NULL)
		throw range_error("Chiamata a giocatoreHelperUtente::getPunteggio con c==NULL");
	if (c1==NULL)
		throw range_error("Chiamata a giocatoreHelperUtente::getPunteggio con c1==NULL");
	return c->getPunteggio() + c1->getPunteggio();
}

/*	Disegna il giocatore sullo schermo
 PARAMETRI DI INPUT:
 nome: nome del giocatore
 mano: carte in mano al giocatore
 iCartaGiocata: indice della carta giocata
 PARAMETRI DI INPUT/OUTPUT:
 dc: Device Context del frame
 restituisce le coordinate su cui si possono disegnare le scritte successive
 */
wxPoint giocatoreHelperUtente::paint(wxPaintDC &dc, const wxString nome, const vector<carta *> mano, const size_t iCartaGiocata) {
	wxCoord c=dc.GetCharHeight(),x,y;
	wxPoint p;
	p.x=3*(carta::getLarghezzaImmagine()+10); // si calcola dove si puo' scrivere successivamente
	c=c+(carta::getAltezzaImmagine()*2); //Dove disegnare le carte del giocatore
	for (size_t i=0; i<mano.size(); i++) {
		if (i!=iCartaGiocata) { //si calcolano le coordinate se non e' la carta giocata
			x=i*(carta::getLarghezzaImmagine()+10);
			y=c;
		} else {
			x=static_cast<wxCoord>(carta::getLarghezzaImmagine()*1.5); //si calcolano le coordinate pr la carta giocata
			y=carta::getAltezzaImmagine()+dc.GetCharHeight();
		}
		dc.DrawBitmap(wxBitmap(*(mano[i]->getImmagine())), x, y);//si disegna la carta
	}
	p.y=c+carta::getAltezzaImmagine();//coordinate per il nome
	dc.DrawText(nome, 0, p.y); //si disegna il nome
	p.y+=dc.GetCharHeight();
	return p;
}
