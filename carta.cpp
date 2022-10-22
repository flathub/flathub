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

#include "carta.h"
wxString carta::path;
wxString carta::nomeMazzo;
cartaHelper *carta::helper;
vector<carta *> carta::carte;

carta::carta(size_t n) {
	seme=helper->getSeme(n);
	valore=helper->getValore(n);
	punteggio=helper->getPunteggio(n);
	img=NULL; //l'immagine andra' caricata appositamente
}

void carta::inizializza(size_t n, cartaHelper *h, wxString nomeMazzo) {
	if (carte.size()>0) //se il vettore delle carte non e' vuoto
		throw logic_error("Chiamato carta::inizializza con carte.size()=="+stringHelper::IntToStr(carte.size()));
	if (h==NULL)
		throw logic_error("Chiamato carta::inizializza con h==NULL");
	helper=h;
	for (size_t i=0; i<n; i++) { //riempiamo il vettore delle carte
		carte.push_back(new carta(i));
	}
	caricaImmagini(nomeMazzo); //carichiamo le immagini
}

carta * const carta::getCarta(size_t quale) {
	if (quale>=carte.size()) //se la carta non e' presente nel mazzo
		throw overflow_error("Chiamato carta::getCarta con quale>carte.size. quale="+stringHelper::IntToStr(quale)+" carte.size()="+stringHelper::IntToStr(carte.size()));
	return carte[quale];
}

void carta::caricaImmagini(wxString mazzo) {
	wxString pathCompleta;
#ifdef _WIN32
	pathCompleta = wxT("C:\\Program Files\\wxBriscola");
#else
	pathCompleta = wxGetHomeDir();
#endif //_WIN32
	path=pathCompleta+wxFileName::GetPathSeparator()+wxT("Mazzi")+wxFileName::GetPathSeparator(); //recuperiamo la path completa della cartella mazzi
	nomeMazzo=mazzo;
	pathCompleta=path+mazzo+wxFileName::GetPathSeparator(); //recuperiamo la path completa delle immagini
	wxString s;
	for (size_t i=0; i<carte.size(); i++) {
		s=pathCompleta+stringHelper::IntToWxStr(i)+wxT(".png"); //recuperiamo la path completa della carta
		if (!wxFileExists(s)) {
            s=_("Il file ")+s+_(" non esiste.");
			throw invalid_argument(string(s.mb_str()));
			return;
		}
		carte[i]->setImmagine(s);
        carte[i]->TipoCarta=1001;
        if (mazzo==wxT("Bergamasco") || mazzo==wxT("Bolognese") || mazzo==wxT("Bresciano")  || mazzo==wxT("Napoletano") || mazzo==wxT("Romagnolo") || mazzo==wxT("Sardo") || mazzo==wxT("Siciliano") || mazzo==wxT("Trientino") || mazzo==wxT("Trevigiano") || mazzo==wxT("Trentino") || mazzo==wxT("Triestino"))
            carte[i]->TipoCarta=1000;
        carte[i]->semeStr=helper->getSemeStr(carte[i]->getNumero(), carte[i]->getTipo());
    }
}


ostream& operator<<(ostream& os, carta &c) {
	os<<c.valore+1<<" "<<c.semeStr;
	return os;
}

bool operator<(carta &c, carta &c1) {
	return carta::helper->compara(carta::helper->getNumero(c.getSeme(), c.getValore()), carta::helper->getNumero(c1.getSeme(),c1.getValore()))==carta::helper->MAGGIORE_LA_SECONDA;
}

const wxImage* carta::getImmagine(size_t quale) {
	if (quale>=carte.size())
		throw overflow_error("Chiamata a carte::getImmagine con quale="+stringHelper::IntToStr(quale));
	return carte[quale]->getImmagine();
}

size_t carta::getTipo() {
return TipoCarta;
}


wxString carta::getSemeStr(size_t quale) {
	if (quale>=carte.size())
		throw overflow_error("Chiamata a carte::getImmagine con quale="+stringHelper::IntToStr(quale));
	return carte[quale]->getSemeStr();
}

const wxString carta::getValoreStr() {
	wxString s;
	s.Printf(wxT("%d"), valore+1);
	return s;
}

void carta::dealloca() {
	for (size_t i=0; i<carte.size(); i++)
		delete carte[i];
	carte.clear();
	delete helper;
}

carta::~carta() {
	delete img;
}
