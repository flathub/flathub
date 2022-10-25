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

#ifndef _OPZIONIFRAME_H_
#define _OPZIONIFRAME_H_
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/window.h>
#include "DoubleValidator.h"

class OpzioniFrame:public wxDialog {
	private:
	enum {ID_TEXTFIELD_UTENTE=100, ID_TEXTFIELD_CPU, ID_TEXTFIELD_TIMER, ID_ABILITA_BRISCOLA, ID_ORDINA_CARTE, ID_ABILITA_AVVISO, ID_ABILITA_CARTA_ALTA, ID_ABILITA_AGGIORNAMENTO, ID_ABILITA_TWITTER};
		wxString nUser, nCpu;
		wxTextCtrl *nomeUtente, *nomeCpu, *valoreTimer;
		wxCheckBox *briscolaAlta, *ordinaCarte, *abilitaAvviso, *abilitaCartaAlta, *abilitaAggiornamenti, *abilitaTwitter;
		wxString s;
	public:
		OpzioniFrame(wxWindow *parent, wxString& nUser, wxString& nCpu, bool abilitaBriscolaAlta, bool ordina, bool avvisa, bool cartaAlta, double secs, bool aggiornamenti, bool twitter);
		bool getBriscolaAlta() {return briscolaAlta->GetValue();}
		bool getOrdinaCarte() {return ordinaCarte->GetValue();}
		bool getAbilitaAvviso() {return abilitaAvviso->GetValue();}
		bool getFlagCartaAlta() {return abilitaCartaAlta->GetValue();}
		bool getFlagAggiornamenti() {return abilitaAggiornamenti->GetValue();}
		bool getTwitter() {return abilitaTwitter->GetValue();}
		double getSecondi() {double d; s.ToDouble(&d); return d;}
		wxString getNomeCpu() {return nomeCpu->GetValue();}
		wxString getNomeUtente() {return nomeUtente->GetValue();}
};

#endif
