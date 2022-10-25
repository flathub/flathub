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

#ifndef _GIOCATORE_HELPER_CPU_H_
#define _GIOCATORE_HELPER_CPU_H_

#include "giocatoreHelper.h"
#include "carta.h"

#include <cstdlib>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/filename.h>
#include <wx/filefn.h>

class giocatoreHelperCpu: public giocatoreHelper {
	private:
		carta *briscola; //identifica la carta di briscola
		wxBitmap *img; //immagine della carta da mostrare sullo schermo al posto di quella dei valori delle carte
		size_t getBriscola(const vector<carta *> &mano); //cerca la piu' piccola carta di briscola
		size_t getSoprataglio(const vector<carta *> &mano, carta *c, bool maggiore); //Cerca la piu' grande carta dello stesso seme che prende, o la piu' piccola che non prende
public:
		giocatoreHelperCpu(size_t b) {
			srand(time(NULL));
			briscola=carta::getCarta(b);
			img=NULL;
			caricaImmagine();
		}
		//carica l'immagine da mostrare al posto delle carte
		void caricaImmagine() {
			wxString s=carta::getPathCarte()+wxT("retro carte pc")+wxT(".png");
			if (img!=NULL)
				delete img;
			if (!wxFileExists(s)) {
				throw invalid_argument("Il file "+string(s.mb_str())+" non esiste.");
				return;
			} else
				img=new wxBitmap(wxImage(s));
		}

		virtual size_t gioca(const vector<carta *> &mano, size_t iCarta);
		virtual size_t gioca(const vector<carta *> &mano, carta *c, size_t iCarta);
		virtual size_t getPunteggio(carta *c, carta *c1);
		virtual wxPoint paint(wxPaintDC &dc, const wxString nome, const vector<carta *> mano, const size_t iCartaGiocata);
};

#endif
