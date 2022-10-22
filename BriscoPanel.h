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

#ifndef _BRISCOPANEL_H_
#define _BRISCOPANEL_H_

#include "mazzo.h"
#include "giocatore.h"
#include "elaboratoreCarteBriscola.h"
#include "giocatoreHelperUtente.h"
#include "giocatoreHelperCpu.h"
#include "cartaHelperBriscola.h"
#include "cartaHelper.h"
#include "CartaAltaFrame.h"
#include "carta.h"

#include <wx/filefn.h>
#include <wx/panel.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/timer.h>
#include <wx/wx.h>
#include <wx/config.h>
#include <wx/cmndata.h>
#include <wx/string.h>
#include <wx/notifmsg.h>

class BriscoPanel : public wxPanel {
	private:
		enum {NESSUN_TASTO_NUMPAD=-1, ID_TIMER=1001};
		mazzo *m;
		giocatore *utente,
					*cpu,
					*primo, //primo di mano
					*secondo, //secondo di mano
					*temp; //per scambiare primo e secondo
		elaboratoreCarteBriscola *e;
		cartaHelperBriscola *b;
		carta *c, *c1;
		giocatoreHelperCpu *motoreCpu;
		size_t semeBriscola, punteggioUtente, punteggioCpu;
		int millisecondi; //per il tempo di presa
		wxBitmap *immagineBriscola, *immagineTallone;
		wxTimer *t;
		wxString nomeMazzo;
		int spaziaturaNome; //per decidere la spaziatura tra i nomi
		bool avvisaFineTallone, //se deve avvisare che il tallone e' finito
			avvisatoFineTallone, //se si e' stati avvisati che il tallone e' finito
			primaUtente, //se deve giocare prima l'utente
			primaPartita, //se e' la prima o la seconda partita
			briscolaDaPunti, //se l'ultima briscola puo' dare punti
			ordinaCarte, //se le carte del giocatore umano devono essere ordinate
			abilitaTwitter; //se aprire twitter al termine delle due partite
        wxColour coloreTesto, coloreSfondo;
		void onKey(wxKeyEvent &evt); //pressione di un tasto
		void onTimer(wxTimerEvent &evt); //scade il timer
		void gioca(int codice); //l'utente deve giocare
		void giocoCartaAlta(); //da' inizio al gioco della carta alta
		void onPaint(wxPaintEvent &event);
		void onClick(wxMouseEvent& evt) ; //gestisce il click sull'immagine
		DECLARE_EVENT_TABLE()
	public:
		BriscoPanel(wxWindow *parent, elaboratoreCarteBriscola *el, cartaHelperBriscola *br, bool primaUt, bool briscolaDaPunti, bool ordinaCarte, int millisecondi, bool avvisaFineTallone, wxString& nomeMazzo, wxString& nomeUtente, wxString& nomeCpu, wxFont *f, wxColour coloreTesto, wxColour coloreSfondo, bool twitter);
		wxString& getNomeUtente() {return utente->getNome();}
		wxString& getNomeCpu() {return cpu->getNome();}
		bool getFlagBriscola() {return briscolaDaPunti;}
		bool getFlagOrdina() {return utente->getFlagOrdina();}
		double getIntervallo() {return millisecondi/1000;}
		int getMilliSecondi() {return millisecondi;}
		bool getFlagAvvisa() {return avvisaFineTallone;}
		void setNomeUtente(wxString n) {utente->setNome(n); Refresh();}
		void setNomeCpu(wxString n) {cpu->setNome(n); Refresh();}
		void setFlagBriscola(bool flag) {
			if (briscolaDaPunti!=flag) {
				briscolaDaPunti=flag;
				nuovaPartita(true, true);
			}
		}
		void setTwitter(bool twitter) { abilitaTwitter = twitter; }
		bool getTwitter() { return abilitaTwitter; }
		void setFlagOrdina(bool o) {ordinaCarte=o; utente->setFlagOrdina(o);}
		void setIntervallo(double secondi) {millisecondi=static_cast<int>(secondi*1000);}
		void setFlagAvvisa(bool a) {avvisaFineTallone=a;}
		void nuovaPartita(bool avvisa, bool inizializza);
		void getDimensioni(wxCoord &x, wxCoord & y);
		bool caricaImmagini(wxString mazzo, bool err=false);
		void setColoreTesto(wxColour &c);
		void setColoreSfondo(wxColour& c);
		virtual ~BriscoPanel();
};

#endif
