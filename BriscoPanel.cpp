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

#include "BriscoPanel.h"

BEGIN_EVENT_TABLE(BriscoPanel, wxPanel)
EVT_PAINT(BriscoPanel::onPaint)
EVT_KEY_UP(BriscoPanel::onKey)
EVT_LEFT_DOWN(BriscoPanel::onClick)
EVT_TIMER(ID_TIMER, BriscoPanel::onTimer)
END_EVENT_TABLE()

BriscoPanel::BriscoPanel(wxWindow *parent, elaboratoreCarteBriscola *el, cartaHelperBriscola *br, bool primaUt, bool briscolaDaPunti, bool ordinaCarte, int millisecondi, bool avvisaFineTallone, wxString& nomeMazzo, wxString& nomeUtente, wxString& nomeCpu, wxFont *f, wxColour coloreTesto, wxColour coloreSfondo, bool twitter)  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(600,500)) {
	e=el;
	b=br;
	this->coloreTesto=coloreTesto;
	this->coloreSfondo = coloreSfondo;
	avvisatoFineTallone=false;
    primaPartita=true;
    primaUtente=primaUt;
    punteggioUtente=0;
    punteggioCpu=0;
    this->briscolaDaPunti=briscolaDaPunti;
    this->ordinaCarte=ordinaCarte;
    this->millisecondi=millisecondi;
    this->avvisaFineTallone=avvisaFineTallone;
    this->nomeMazzo=nomeMazzo;
	abilitaTwitter = twitter;
	m=new mazzo(e);
	semeBriscola=b->getSeme(e->getCartaBriscola());
	motoreCpu=new giocatoreHelperCpu(e->getCartaBriscola());
	utente=new giocatore(new giocatoreHelperUtente(), nomeUtente, ordinaCarte);
	cpu=new giocatore(motoreCpu,nomeCpu);

	if (primaUtente) {//se dal gioco della carta alta e' risultato prima l'utente o se non si e' fatto proprio
		primo=utente;
		secondo=cpu;
	} else {
		primo=cpu;
		secondo=utente;
	}

	for (size_t i=0; i<3; i++) { //assegnazione delle carte in mano ai giocatori
		primo->addCarta(m);
		secondo->addCarta(m);
	}

	immagineBriscola=new wxBitmap(*(carta::getImmagine(e->getCartaBriscola()))); //caricamento delle immagini
	immagineTallone=new wxBitmap(wxImage(carta::getPathCarte()+"retro carte mazzo.png"));
	t=new wxTimer(this, ID_TIMER); //inizializzazione del timer
	if (primo==cpu) //se deve giocare prima la cpu
		primo->gioca(0);
	SetFont(*f);
	SetBackgroundColour(coloreSfondo);
}

BriscoPanel::~BriscoPanel() {
    wxFont f=GetFont();
	delete m;
	delete utente;
	delete cpu;
	delete immagineBriscola;
	delete immagineTallone;
	delete t;
}

void BriscoPanel::getDimensioni(wxCoord &x, wxCoord & y) {
	wxString s=_("Nel mazzo rimangono ")+m->getNumeroCarteStr()+_(" carte.");
	wxPoint dimStringa, dimNome; //dimensioni della stringa del mazzo e del nome dell'utente
	wxCoord temp;
	GetTextExtent(s, &dimStringa.x, &dimStringa.y);//si vede quanto e' grande la scritta del mazzo
	GetTextExtent(utente->getNome(), &dimNome.x, &dimNome.y); //e quanto e' grande il nome dell'utente
	x=carta::getLarghezzaImmagine()*3+80+dimStringa.x; //si calcola la dimensione delle immahini
	y=carta::getAltezzaImmagine()*3+dimStringa.y*2;
	temp=carta::getLarghezzaImmagine()*3+110+dimStringa.x;
	if( x<temp) //si vede dove si puo' disegnare
		x=temp;
	if (carta::getAltezzaImmagine()>carta::getLarghezzaImmagine())
		temp=carta::getAltezzaImmagine()-carta::getLarghezzaImmagine();
	else
		temp=carta::getLarghezzaImmagine()-carta::getAltezzaImmagine();
	temp=30+dimNome.y*4+temp;
	if (y<temp)
		y=temp;
}

void BriscoPanel::onPaint(wxPaintEvent &event) {
	wxPaintDC dc(this);
	dc.SetTextForeground(coloreTesto);
	wxString s=_("Nel mazzo rimangono ")+m->getNumeroCarteStr()+_(" carte.");
	wxPoint p, p1; //punti in cui disegnare
	wxCoord len, y; //dimensione della stringa del mazzo e punto di disegno
	dc.SetFont(GetFont()); //si setta il font
	GetTextExtent(s, &len,&y);//si prendono le dimensioni
	spaziaturaNome=y;
	p=cpu->paint(dc, y); //disegno della cpu
	p1=utente->paint(dc, y); //disegno dell'utente
	p.x+=50;
	p1.x+=50;
	y=30+dc.GetCharHeight()*4+carta::getAltezzaImmagine()-carta::getLarghezzaImmagine(); //calcolo del punto di disegno
	dc.DrawText(_("Punti di ")+utente->getNome()+_(": ")+utente->getPunteggioStr(), p.x,30); //disegno delle stringhe
	dc.DrawText(_("Punti di ")+cpu->getNome()+_(": ")+cpu->getPunteggioStr(), p.x,30+dc.GetCharHeight());
	dc.DrawText(s, p.x,30+dc.GetCharHeight()*2);
	dc.DrawText(_("Il seme di briscola e': ")+carta::getSemeStr(e->getCartaBriscola()), p.x, 30+dc.GetCharHeight()*3);
	if (m->getNumeroCarte()>0) { //disegno del tallone
		if (carta::getAltezzaImmagine()>=carta::getLarghezzaImmagine()) {
			dc.DrawBitmap(*immagineBriscola, p.x+(len-carta::getLarghezzaImmagine())/2,30+dc.GetCharHeight()*4);
			dc.DrawBitmap(*immagineTallone, p.x+(len-carta::getAltezzaImmagine())/2, y);
		} else {
			dc.DrawBitmap(*immagineBriscola, p.x+(len-carta::getLarghezzaImmagine())/2,30+dc.GetCharHeight()*4+carta::getLarghezzaImmagine()-carta::getAltezzaImmagine());
			dc.DrawBitmap(*immagineTallone, p.x+(len-carta::getAltezzaImmagine())/2, 30+dc.GetCharHeight()*4);
		}
	}
}

void BriscoPanel::onKey(wxKeyEvent &evt){
    int codice=NESSUN_TASTO_NUMPAD;
    if (utente->hasCartaGiocata())
        return;
    switch(evt.GetKeyCode()) {
        case WXK_NUMPAD1: ;
        case 49: codice=0; break;
        case WXK_NUMPAD2: ;
        case 50: codice=1; break;
        case WXK_NUMPAD3: ;
        case 51: codice=2; break;
    }
	gioca(codice);
}

void BriscoPanel::gioca(int codice) {
	try {
		utente->gioca(codice);
	} catch (range_error &e) {
		return;
	}
	if (!cpu->hasCartaGiocata())
		cpu->gioca(utente, 0);
	Refresh();
	c=primo->getCartaGiocata();
	c1=secondo->getCartaGiocata();
	if ((primo->stessoSemeCartaGiocata(secondo) && b->compara(c->getNumero(), c1->getNumero())==2) || (secondo->stessoSeme(carta::getCarta(e->getCartaBriscola())) && !primo->stessoSeme(carta::getCarta(e->getCartaBriscola())))) {
		temp=primo;
		primo=secondo;
		secondo=temp;
	}
	t->Start(millisecondi, wxTIMER_ONE_SHOT);
}

void BriscoPanel::onTimer(wxTimerEvent &evt) {
	bool mostraRichiesta = true;
	primo->aggiornaPunteggio(secondo);	try {
		primo->addCarta(m);
		secondo->addCarta(m);
	} catch (underflow_error &e) { //sono finite le carte
		if (primaPartita) {//se e' la prima partita
			if (wxMessageBox(_("Vuoi effettuare la seconda partita?"), _("Continuare?"), wxYES_NO | wxICON_INFORMATION) == wxYES) {
				nuovaPartita(false, false); //se l'utente dice di si se ne comincia un'altra
				return;
			}
			else mostraRichiesta = false;
        }
		wxString s; //si mette insieme la stringa dell'esito
		if (utente->getPunteggio()+punteggioUtente==cpu->getPunteggio()+punteggioCpu)
			s=_("La partita e' patta.");
		else {
			if (utente->getPunteggio()+punteggioUtente>cpu->getPunteggio()+punteggioCpu)
				s=_("vinto ");
			else
				s=_("perso");
			s=_("Hai ")+s+_(" per ")+stringHelper::IntToWxStr(labs(utente->getPunteggio()+punteggioUtente-cpu->getPunteggio()-punteggioCpu))+_(" punti.");
		}
		Refresh();
		if (abilitaTwitter)
			wxLaunchDefaultBrowser(wxT("http://twitter.com/intent/tweet?text=Con%20la%20wxBriscola%20la%20partita%20") + utente->getNome() + wxT("%20contro%20") + cpu->getNome() + wxT("%20con%20mazzo%20") + carta::getNomeMazzo() + wxT("%20%C3%A8%20finita%20") + utente->getPunteggioStr() + wxT("%20a%20") + cpu->getPunteggioStr() + wxT("&url=https%3A%2F%2Fgithub.com%2Fnumerunix%2FwxBriscola"));

		if (mostraRichiesta)
			if (wxMessageBox(wxString(_("La partita e' finita.")) + "\n" + s + "\n" + _("Vuoi effettuare una nuova partita?"), _("Partita finita"), wxYES_NO | wxICON_QUESTION) == wxNO) {
				GetParent()->Close();
				return;
			}
			else {
				nuovaPartita(false, true);
				return;
			}
		else {
			wxMessageBox(wxString(_("La partita e' finita.")) + "\n" + s, _("Partita finita"), wxOK | wxICON_INFORMATION);
			GetParent()->Close();
			return;
		}
	}
	if (m->getNumeroCarte()==2 && !avvisatoFineTallone && avvisaFineTallone) { //se e' finito il tallone
		wxNotificationMessage *msg = new wxNotificationMessage(_("Tallone finito"), _("Il tallone e' finito"), this);
		msg->Show();
		delete msg;
		msg = NULL;
		avvisatoFineTallone=true;
	}
	if (primo == cpu) {
		primo->gioca(0);
		if (primo->getCartaGiocata()->stessoSeme(carta::getCarta(e->getCartaBriscola()))) {
			wxNotificationMessage* msg = new wxNotificationMessage(_("Carta di Briscola"), _("La cpu ha giocato il ") + stringHelper::IntToWxStr(primo->getCartaGiocata()->getValore() + 1) + _(" di briscola"), this);
			msg->Show();
			delete msg;
		}
		else if (primo->getCartaGiocata()->getPunteggio() > 0)
		{
			wxNotificationMessage* msg = new wxNotificationMessage(_("Carta con valore"), _("La cpu ha giocato il ") + stringHelper::IntToWxStr(primo->getCartaGiocata()->getValore() + 1) + _(" di ") + primo->getCartaGiocata()->getSemeStr(), this);
			msg->Show();
			delete msg;
		}
	}
	Refresh();
}

//Se si deve giocare una nuova partita
void BriscoPanel::nuovaPartita(bool avvisa, bool inizializza) {
	wxString nUser, nCpu;
	if (avvisa && wxMessageBox(_("La partita correntemente in corso verra' interrotta. Continuare?"), _("Richiesta conferma"), wxYES_NO|wxICON_INFORMATION)==wxNO) //se si sta giocando una partita non finita
			return;
	if (inizializza) { //se bisogna inizializzare le componenti (l'utente ha deciso di chiudere la partita precedente
		punteggioUtente=0; //inizializzazione dei punteggi
		punteggioCpu=0;
		primaPartita=true;//e' una nuova prima partita
	} else { //non bisogna inizializzare le componenti perche' e' una seconda partita
		punteggioUtente=utente->getPunteggio(); //salvataggio dei punteggi
		punteggioCpu=cpu->getPunteggio();
		primaPartita=false;
	}
	nUser=utente->getNome();
	nCpu=cpu->getNome();
	carta::dealloca();
	delete utente; //deallocazione dei vecchi oggetti
	delete cpu;
	delete m;
	delete immagineBriscola;
	e=new elaboratoreCarteBriscola(briscolaDaPunti); //nuovi oggetti
	m=new mazzo(e);
	b=new cartaHelperBriscola(e);
	semeBriscola=b->getSeme(e->getCartaBriscola());
	carta::inizializza(40, b, nomeMazzo);
	motoreCpu=new giocatoreHelperCpu(e->getCartaBriscola());
	utente=new giocatore(new giocatoreHelperUtente(), nUser, ordinaCarte);
	cpu=new giocatore(motoreCpu, nCpu);
	primaUtente=!primaUtente;
	if (primaUtente) { //se deve giocare prima l'utente
		primo=utente;
		secondo=cpu;
	} else {
		primo=cpu;
		secondo=utente;
	}
	for (size_t i=0; i<3; i++) { //si assegnano le carte in mano ai giocatori
		primo->addCarta(m);
		secondo->addCarta(m);
	}
	immagineBriscola=new wxBitmap(*(carta::getImmagine(e->getCartaBriscola()))); //nuova carta di briscola
	if (cpu==primo)
		primo->gioca(0);
	avvisatoFineTallone=false;
	Refresh();
}

/*
 Err indica che si e' verificato un errore al caricamento del mazzo precedente
 restituisce true se si e' verificato un errore nel caricamento
 */
bool BriscoPanel::caricaImmagini(wxString mazzo, bool err) {
	bool errore=false;
	wxString s;
	if (nomeMazzo==mazzo && !err) //se si sta cercando di cambiare un mazzo con lo stesso
		return false;
	try {
		carta::caricaImmagini(mazzo); //si caricano le immagini delle carte
		motoreCpu->caricaImmagine(); //si carica l'immagine della cpu
	} catch (invalid_argument &e) { //non e' riuscito il caricamento
		s=wxString(e.what(), wxConvUTF8);
		errore=true;
	}
	if (!errore) { //il caricamento e' riuscito correttamente
		delete immagineBriscola;
		delete immagineTallone;
		immagineBriscola=new wxBitmap(*(carta::getImmagine(e->getCartaBriscola()))); //si carica l'immagine della briscola
		wxString s=carta::getPathCarte()+wxT("retro carte mazzo.png");
		if (!wxFileExists(s)) {
			errore=true;
			s=_("Il file ")+s+_(" non esiste.");
		} else { //se non ci sono errori
			immagineTallone=new wxBitmap(wxImage(s)); //si carica l'immagine del tallone
			nomeMazzo=mazzo; //si assegna il nuovo nome al mazzo
			Refresh(); //si ricarica la schermata
		}
	}
	if (errore && !err) { //si e' verificato un errore nel caricamento
		wxMessageBox(s+_(" Il programma provera' a ricaricare il mazzo precedente."), _("Errore"), wxOK|wxICON_ERROR);
		if (caricaImmagini(nomeMazzo, true)==false) //si sono verificarti due errori di fila
			wxMessageBox(_("Non e' stato possibile caricare il mazzo precedente: uscire dal programma significa non poterlo avviare piu'. Caricare un mazzo completo prima di uscire."), _("Attenzione"), wxOK | wxICON_EXCLAMATION);
	}
	return !errore;
}

void BriscoPanel::setColoreTesto(wxColour &c) {
    coloreTesto=c;
    Refresh();
}

void BriscoPanel::setColoreSfondo(wxColour& c) {
	SetBackgroundColour(c);
	Refresh();
}

void BriscoPanel::onClick(wxMouseEvent& evt) {
    int y=evt.GetY(), x=evt.GetX(), i;
    if (utente->hasCartaGiocata())
        return;
    y=y-spaziaturaNome-(carta::getAltezzaImmagine()+10)*2;
    if (y<0 || carta::getAltezzaImmagine()<y)
        return;
    for (i=0; i<3 && x>carta::getLarghezzaImmagine()+10; i++, x=x-carta::getLarghezzaImmagine()-10);
    gioca(i);
}
