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


#include "CartaAltaFrame.h"

BEGIN_EVENT_TABLE(CartaAltaFrame, wxDialog)
EVT_BUTTON(ID_BUTTON_OK, CartaAltaFrame::onOk)
EVT_PAINT(CartaAltaFrame::onPaint)
EVT_TEXT_ENTER(ID_TEXTFIELD_CARTA, CartaAltaFrame::onOk)
END_EVENT_TABLE()

CartaAltaFrame::CartaAltaFrame(wxWindow *parent, wxString nomeMazzo, wxFont *f) : wxDialog(parent, wxID_ANY, _("Gioco della carta alta"), wxDefaultPosition, wxSize(500,300), wxCAPTION | wxSYSTEM_MENU) , primaUtente(true) {
    wxPoint dimStringa;
	wxString str(_("Scrivere un numero da 1 a 40 che identifica la carta del mazzo da scegliere."));
    SetFont(*f);
    GetTextExtent(str, &dimStringa.x, &dimStringa.y);
    SetClientSize(dimStringa.x, dimStringa.y*15);
	valore=wxEmptyString;
	s=new wxBoxSizer(wxHORIZONTAL);
	box=new wxBoxSizer(wxVERTICAL);
	boxPulsanti=new wxBoxSizer(wxHORIZONTAL);
	IntValidator v=IntValidator(&valore, 1, 40); //inizializziamo il validatore indicando che deve prendere un numero da 1 a 40
	cartaUtente=new wxTextCtrl(this, ID_TEXTFIELD_CARTA, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER, v);
	cartaUtente->SetMaxLength(2);
	s->Add(new wxStaticText(this, wxID_ANY, _("Numero della carta da prendere: ")), 0, wxALL, 4);
	s->Add(cartaUtente, 0, wxALL, 4);
	ok=new wxButton(this, ID_BUTTON_OK, _("OK"));
	boxPulsanti->Add(ok, 0, wxALL, 4);
	cancella=new wxButton(this, wxID_CANCEL, _("Annulla"));
	boxPulsanti->Add(cancella, 0, wxALL, 4);
	inizio=new wxStaticText(this, wxID_ANY, wxString(_("Il gioco della carta alta permette di stabilire chi gioca per primo."))+"\n"+str+"\n"+_("Il computer ne scegliera' un'altra e chi avra' il valore maggiore comincera'.")+"\n\n\n");
	box->Add(inizio,0,wxALL,4);
	box->Add(s);
	msg=new wxStaticText(this, wxID_ANY, "\n\n\n");
	box->Add(msg, 0, wxALL, 4);
	box->Add(boxPulsanti);
	SetSizer(box);
	m=new mazzo(new elaboratoreCarteBriscola());
	c=c1=NULL;
	srand(time(NULL));
	ok->SetFocus();
}

void CartaAltaFrame::onOk(wxCommandEvent &evt) {
	wxString st;
	if (c!=NULL) { //se la carta e' gia' stata presa
		Close();
		return;
	}
	long l;
	valore=cartaUtente->GetValue();
	if (valore=="") //se non e' stato indicato nessun valore
		return;
	valore.ToLong(&l);
	c=carta::getCarta(m->getCarta(static_cast<size_t>(--l))); //prendiamo l'immagine della carta indicata dall'utente
	l=rand()%39; //selezioniamo una carta casuale
	c1=carta::getCarta(m->getCarta(l)); //ne prendiamo l'immagine
	primaUtente=c->getValore()>=c1->getValore(); //confrontiamo i valori
	if (!primaUtente)
		st=_("Comincia prima il pc");
	else
		st=_("Cominci prima tu");
	s->Show(false);
	cancella->Show(false);
	inizio->SetLabel(st);
	Refresh();
	ok->SetFocus();
}

void CartaAltaFrame::onPaint(wxPaintEvent &event) {
	wxPaintDC dc(this);
	dc.SetFont(GetFont());
	wxCoord x, y, x1, y1;
	wxString msg=_("Carta tua"), msg1=_("Carta del pc");
	GetTextExtent(msg, &x, &y);
	GetTextExtent(msg1, &x1, &y1);
	if (c==NULL && c1==NULL) {
		event.Skip();
		return;
	}
	if (c!=NULL) {
		dc.DrawText(msg, 0,y*2);
		dc.DrawBitmap(wxBitmap(*c->getImmagine()), 0, y*3);
		x = c->getLarghezzaImmagine();
	}
	if (c1!=NULL) {
		dc.DrawText(msg1, x, y1*2);
		dc.DrawBitmap(wxBitmap(*c1->getImmagine()), x, y1*3);
	}
}

CartaAltaFrame::~CartaAltaFrame() {
	delete m;
}
