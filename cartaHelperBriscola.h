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

#ifndef _CARTA_HELPER_BRISCOLA_H_
#define _CARTA_HELPER_BRISCOLA_H_

#include "cartaHelper.h"
#include "carta.h"

/* Implementazione di cartaHelper specifica per il gioco della briscola a due giocatori
	Per i valori numerali:
	0-9 sono le carte in ordine crescente in cui 0 e' l'1 ed il 9 e' il 10
	Per i semi:
		1: bastoni
		2: coppe
		3: denari
		4: spade

	L'implementazione e' semplice: il valore nominale sono le unita' e le decine sono i semi
	Pertanto essendo le carte 40, queste vanno da 0 39.
 */
class cartaHelperBriscola : public cartaHelper {
	private:
		size_t cartaBriscola; //numero della carta di briscola uscita durante la mescolazione delle carte
	public:
	cartaHelperBriscola(elaboratoreCarteBriscola *e) : cartaBriscola(e->getCartaBriscola()) {;}
	virtual size_t getSeme(size_t carta) {
		if (carta>39)
			throw invalid_argument("Chiamato cartaHelperBriscola::getSeme() con carta="+stringHelper::IntToStr(carta));
		return carta/10; //recuperiamo il seme
	}
	virtual size_t getValore(size_t carta) {
		if (carta>39)
			throw invalid_argument("Chiamato cartaHelperBriscola::getSeme() con carta="+stringHelper::IntToStr(carta));
		return carta%10; //recuperiamo il valore nominale
	}
	virtual size_t getPunteggio(size_t carta) {
		if (carta>39)
			throw invalid_argument("Chiamato cartaHelperBriscola::getSeme() con carta="+stringHelper::IntToStr(carta));
		size_t valore=0;
		switch(carta%10) { //recuperiamo il punteggio in base al valore nominale
			case 0: valore=11; break;
			case 2: valore=10; break;
			case 9: valore=4; break;
			case 8: valore=3; break;
			case 7: valore=2; break;
		}
		return valore;
	}
	virtual wxString getSemeStr(size_t carta, size_t tipo) {
		if (tipo==1000)
			return getSemeStrItaliana(carta);
		return getSemeStrFrancese(carta);
	}


	virtual wxString getSemeStrItaliana(size_t carta) {
		if (carta>39)
			throw invalid_argument("Chiamato cartaHelperBriscola::getSeme() con carta="+stringHelper::IntToStr(carta));
		wxString s;
		switch (carta/10) { //recuperiamo il seme
			case 0: s=_("Bastoni"); break;
			case 1: s=_("Coppe"); break;
			case 2: s=_("Denari"); break;
			case 3: s=_("Spade"); break;
		}
		return s;
	}


	 virtual wxString getSemeStrFrancese(size_t carta) {
		if (carta>39)
			throw invalid_argument("Chiamato cartaHelperBriscola::getSeme() con carta="+stringHelper::IntToStr(carta));
		wxString s;
		switch (carta/10) { //recuperiamo il seme
			case 0: s=_("Fiori"); break;
			case 1: s=_("Quadri"); break;
			case 2: s=_("Cuori"); break;
			case 3: s=_("Picche"); break;
		}
		return s;

    }
	virtual size_t getNumero(size_t seme, size_t valore) {
		if (seme>4 || valore>9)
			throw logic_error("Chiamato cartaHelperBriscola::getNumero con seme="+stringHelper::IntToStr(seme)+" e valore "+stringHelper::IntToStr(valore));
		return seme*10+valore;
	}


	virtual RISULTATI_COMPARAZIONE compara(size_t carta, size_t carta1) {
		size_t punteggio=getPunteggio(carta), //punteggio della prima carta
			   punteggio1=getPunteggio(carta1), //punteggio della seconda carta
			   valore=getValore(carta), //valore nominale della prima carta
			   valore1=getValore(carta1), //valore nominale della seconda carta
			   semeBriscola=getSeme(cartaBriscola), //seme di briscola
			   semeCarta=getSeme(carta), //seme della prima carta
	       	   semeCarta1=getSeme(carta1); //seme della seconda carta
		if (punteggio<punteggio1) //se le carte hanno punteggio diverso e' maggiore chi ha punteggio piu' alto
			return MAGGIORE_LA_SECONDA;
		else if (punteggio>punteggio1)
			return MAGGIORE_LA_PRIMA;
		else {
			if (valore<valore1 || (semeCarta1==semeBriscola && semeCarta!=semeBriscola)) //se le carte hanno punteggio uguale allora si confrontano i valori ed i semi
				return MAGGIORE_LA_SECONDA;
			else if (valore>valore1 || (semeCarta==semeBriscola && semeCarta1!=semeBriscola))
				return MAGGIORE_LA_PRIMA;
			else	return UGUALI; //se hanno lo stesso valore e lo stesso seme sono uguali
		}
	}
};


#endif
