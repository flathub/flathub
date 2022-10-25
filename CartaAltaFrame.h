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
#ifndef _CARTA_ALTA_FRAME_H_
#define _CARTA_ALTA_FRAME_H_

#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/wx.h>

#include "mazzo.h"
#include "elaboratoreCarteBriscola.h"
#include "carta.h"
#include "IntValidator.h"
#include "cartaHelperBriscola.h"

class CartaAltaFrame: public wxDialog {
private:
	enum {ID_TEXTFIELD_CARTA=101, ID_BUTTON_OK};
	bool primaUtente; //indica se e' l'utente che deve giocare prima
	wxTextCtrl *cartaUtente;
	mazzo *m; //mazzo per giocare
	wxStaticText *inizio, *msg; //box di testo che contiene il numero della carta da scegliere
	wxBoxSizer *s, *box, *boxPulsanti;
	wxButton *cancella, *ok;
	carta *c, *c1; //le due carte prese
	wxString valore;

	void onOk(wxCommandEvent &evt);
	void onPaint(wxPaintEvent &event);
	DECLARE_EVENT_TABLE()
public:

	CartaAltaFrame(wxWindow *parent, wxString nomeMazzo, wxFont *f);
	~CartaAltaFrame();
	bool giocaPrimaUtente() {return primaUtente;}
};

#endif
