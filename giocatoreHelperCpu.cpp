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

#include "giocatoreHelperCpu.h"

/* Cerca la piu' piccola carta di briscola
 PARAMETRI DI INPUT:
	mano: le carte in mano al giocatore da "aiutare"
 restituisce l'indice della carta di briscola
 */
size_t giocatoreHelperCpu::getBriscola(const vector<carta *> &mano) {
	size_t i;
	for (i=0; i<mano.size() && !briscola->stessoSeme(mano[i]); i++);
	return i;
}

/*Cerca la piu' grande carta dello stesso seme che prende, o la piu' piccola che non prende
 PARAMETRI DI INPUT:
	mano: le carte in mano al giocatore da "aiutare"
	c: carta giocata dall'altro giocatore
	maggiore: flag che identifica se bisogna trovare "la piu' grande che prende" (true) o "la piu' piccola che non prende" (false)
 retituisce l'indice della carta da giocare
*/
size_t giocatoreHelperCpu::getSoprataglio(const vector<carta *> &mano, carta *c, bool maggiore) {
    bool trovata=false;
    size_t i;
    if (maggiore) { //si cerca la carta piu' grande che puo' prendere la carta giocata
        for (i=mano.size()-1; i<mano.size(); i--)
            if (c->stessoSeme(mano[i]) && *c<*mano[i]) {
                trovata=true;
                break;
            } else if (c->stessoSeme(mano[i]) && *mano[i]<*c)
                break;
    } else { //si cerca la carta piu' piccola che non puo' prendere la carta giocata
        for (i=0; i<mano.size(); i++)
            if (c->stessoSeme(mano[i]) && *c<*mano[i]) {
                trovata=true;
                break;
            }
    }
    if (trovata)
        return i;
    else
        return mano.size();
}

/* Richiamata quando la cpu e' prima di mano
	PARAMETRI DI INPUT:
	mano: carte in mano alla CPU,
	iCarta: puramente fittizio e non serve
 retituisce l'indice della carta da giocare
 */
size_t giocatoreHelperCpu::gioca(const vector<carta *> &mano, size_t iCarta) {
	size_t i;
    if (mano.size()==0)
        throw range_error("Chiamata a giocatoreHelperCpu::gioca con mano.size(mano)==0");
    for (i=mano.size()-1; i<mano.size() && (mano[i]->getPunteggio()>4 || briscola->stessoSeme(mano[i])); i--); //cerca la piu' grande carta che non sia briscola e che non sia "carico"
	if (i>mano.size()) //se non c'e' gioca la carta piu' piccola
		i=0;
    return i;
}


/* Richiamata quando la cpu e' seconda di mano
	PARAMETRI DI INPUT:
	mano: carte in mano al giocatore da aiutare
	c: carta giocata dall'laro giocatore
	iCarta: puramente fittizio. Non serve.
 */
size_t giocatoreHelperCpu::gioca(const vector<carta *> &mano, carta *c, size_t iCarta) {
	if (mano.size()==0)
		throw range_error("Chiamata a giocatoreHelperCpu::gioca(mano, c) con mano.size()==0");
	if (c==NULL)
		throw range_error("Chiamata a giocatoreHelperUtente::gioca(vector<carta *>, carta *) con carta==NULL");

	size_t i=rand(); //aggiunge un comportamento un po' piu' casuale
    if (!briscola->stessoSeme(c)) { //se la carta giocata non e' dello stesso seme di briscola
        if ((i=getSoprataglio(mano, c, true))<mano.size()) //se si puo' sopratagliare
            return i; //soprataglia
        if (c->getPunteggio()>0 && (i=getBriscola(mano))<mano.size()) {//se non si puo' sopratagliare e la carta ha un punteggio e si ha una carta di briscola
            if (c->getPunteggio()>4) //se la carta giocata e' un carico
                return i; //si gioca la briscola
            if (mano[i]->getPunteggio()>0) //se la carta che si vuole giocare e' briscola ed ha un punteggio
                if (i%10<5) //il computer decide se giocarla o meno
                    return i;
        }
    } else { //se e' di briscola
        if (i%10<5 && (i=getSoprataglio(mano, c, false))<mano.size()) //se il computer vuole giocarla e c'e' un soprataglio della carta di briscola
            return i; //si soprataglia
    }
    i=0; //se non si puo' fare niente si gioca la carta piu' piccola
	return i;
}

/* Aggiorna i punteggi delle carte
	lancia un range error se una delle due carte e' null
	restituisce la somma die punteggi.
 */
size_t giocatoreHelperCpu::getPunteggio(carta *c, carta *c1) {
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
wxPoint giocatoreHelperCpu::paint(wxPaintDC &dc, const wxString nome, const vector<carta *> mano, const size_t iCartaGiocata) {
	dc.DrawText(nome, 0, 0); //si disegna il nome
	wxCoord c=dc.GetCharHeight(); //si prende l'altezza del carattere
	wxPoint p;
	p.x=3*(img->GetWidth()+10); //si calcola il primo spazio a destra delle carte
	p.y=c;
	for (size_t i=0; i<mano.size(); i++)
		if (i!=iCartaGiocata)
			dc.DrawBitmap(*img, i*(img->GetWidth()+10), c); // si disegnano i tre o due dorsi
		else
			dc.DrawBitmap(wxBitmap(*(mano[i]->getImmagine())), img->GetWidth()/2, c+img->GetHeight()); //si disegna la carta giocata
	return p;
}
