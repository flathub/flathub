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

#include "OpzioniFrame.h"

OpzioniFrame::OpzioniFrame(wxWindow* parent, wxString& nUser, wxString& nCpu, bool abilitaBriscolaAlta, bool ordina, bool avvisa, bool cartaAlta, double secs, bool aggiornamenti, bool twitter) : wxDialog(parent, wxID_ANY, _("Opzioni"), wxDefaultPosition) {
	wxBoxSizer* boxLabel = new wxBoxSizer(wxVERTICAL), * boxText = new wxBoxSizer(wxVERTICAL), * controlBox = new wxBoxSizer(wxHORIZONTAL), * mainBox = new wxBoxSizer(wxVERTICAL);
	s.Printf(wxT("%f"), secs);
	s.Truncate(5);
	DoubleValidator v = DoubleValidator(&s, 1, 10);
	nomeUtente = new wxTextCtrl(this, ID_TEXTFIELD_UTENTE, nUser);
	nomeCpu = new wxTextCtrl(this, ID_TEXTFIELD_CPU, nCpu);
	valoreTimer = new wxTextCtrl(this, ID_TEXTFIELD_TIMER, "", wxDefaultPosition, wxDefaultSize, 0, v);
	valoreTimer->SetMaxLength(5);
	nomeUtente->SetMaxLength(15);
	nomeCpu->SetMaxLength(15);
	briscolaAlta = new wxCheckBox(this, ID_ABILITA_BRISCOLA, _("La carta che designa la briscola puo' dar punti"));
	briscolaAlta->SetValue(abilitaBriscolaAlta);
	ordinaCarte = new wxCheckBox(this, ID_ORDINA_CARTE, _("Ordina le carte che mi capitano"));
	ordinaCarte->SetValue(ordina);
	abilitaAvviso = new wxCheckBox(this, ID_ABILITA_AVVISO, _("Avvisa quando il tallone finisce"));
	abilitaAvviso->SetValue(avvisa);
	abilitaCartaAlta = new wxCheckBox(this, ID_ABILITA_CARTA_ALTA, _("Fai il gioco della carta piu' alta all'avvio"));
	abilitaCartaAlta->SetValue(cartaAlta);
	abilitaTwitter = new wxCheckBox(this, ID_ABILITA_TWITTER, _("Abilita la notifica su twitter quando la partita e' finita"));
	abilitaTwitter->SetValue(twitter);
	abilitaAggiornamenti = new wxCheckBox(this, ID_ABILITA_AGGIORNAMENTO, _("Notifica nuove versioni all'avvio"));
	abilitaAggiornamenti->SetValue(false);
	abilitaAggiornamenti->Enable(false);
	boxLabel->Add(new wxStaticText(this, wxID_ANY, _("Nome utente: ")), 0, wxALL, 4);
	boxLabel->Add(new wxStaticText(this, wxID_ANY, _("Nome cpu: ")), 0, wxALL, 4);
	boxLabel->Add(new wxStaticText(this, wxID_ANY, _("Secondi in cui mostrare le giocate")), 0, wxALL, 4);
	boxText->Add(nomeUtente);
	boxText->Add(nomeCpu);
	boxText->Add(valoreTimer);
	controlBox->Add(boxLabel);
	controlBox->Add(boxText);
	mainBox->Add(controlBox);
	mainBox->Add(briscolaAlta, 0, wxALL, 4);
	mainBox->Add(ordinaCarte, 0, wxALL, 4);
	mainBox->Add(abilitaAvviso, 0, wxALL, 4);
	mainBox->Add(abilitaCartaAlta, 0, wxALL, 4);
	mainBox->Add(abilitaTwitter, 0, wxALL, 4);
	mainBox->Add(abilitaAggiornamenti, 0, wxALL, 4);
	mainBox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALL, 5);
	SetSizer(mainBox);
	Layout();
	Fit();
}