#ifndef HEADER_EC648212512BB37D
#define HEADER_EC648212512BB37D

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

#ifndef _BRISCOFRAME_H_
#define _BRISCOFRAME_H_

#include "BriscoPanel.h"
#include "OpzioniFrame.h"

#include <wx/dir.h>
#include <wx/frame.h>
#include <wx/aboutdlg.h>
#include <wx/fontdlg.h>
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include <wx/app.h>
#include <wx/intl.h>
#include <wx/colordlg.h>
#include <wx/colourdata.h>
#ifndef _WIN32
#include "wxbriscola.xpm"
#endif

class BriscoFrame: public wxFrame {
	private:
		enum {ID_NUOVA_PARTITA=10001, ID_OPZIONI, ID_FONT, ID_AGGIORNAMENTO, ID_SITOWEB, ID_COLORE_TESTO, ID_COLORE_SFONDO};
		BriscoPanel *p;
		wxConfig *config;
		wxMenu *menuMazzi,
                *menuTraduzioni;
        wxArrayLong idTraduzioni,
                    idMenuTraduzioni;
		wxPoint dim;
		wxString nomeMazzo,
                 versione,
                 paginaWeb,
                 pathTraduzioni;
		wxFont *font;
		bool cartaAlta, //se si deve fare il gioco della carta alta
			primaUtente,
			aggiornamenti;
		elaboratoreCarteBriscola *el;
		cartaHelperBriscola *br;
       // wxHTTP client;
        wxLocale *traduzione;
        wxColour coloreTesto, coloreSfondo;
        wxColourData d, d1;
        int loc;
		void aggiungiMenu();
		void onEsci(wxCommandEvent& WXUNUSED(evt));
		void onInfo(wxCommandEvent& WXUNUSED(evt));
        	void onNuovaPartita(wxCommandEvent& WXUNUSED(evt));
		void onOpzioni(wxCommandEvent& WXUNUSED(evt));
		void onFont(wxCommandEvent& WXUNUSED(evt));
		void getMenuMazzi(wxMenu *menu);
		void onMenuMazzi(wxCommandEvent& evt);
		//void onAggiornamenti(wxCommandEvent& evt);
		void onSitoWeb(wxCommandEvent& evt);
		void OnMenuTraduzioni(wxCommandEvent &evt);
		void leggiFont();
		//bool Aggiornamenti(wxString& nuovaVersione) throw (std::domain_error);
		void getMenuTraduzioni(wxMenu *menu);
		void OnColoreTesto(wxCommandEvent& evt);
		void OnColoreSfondo(wxCommandEvent& evt);
		void CreaVoceTraduzione(wxMenu* menu, const wxLanguageInfo* lang);
		DECLARE_EVENT_TABLE()
	public:
		BriscoFrame(int l, wxConfig *c, wxString path);
		~BriscoFrame();
		void giocoCartaAlta();
};

#endif
#endif // header guard 

