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

#include "DoubleValidator.h"

BEGIN_EVENT_TABLE(DoubleValidator, wxValidator)
EVT_CHAR(DoubleValidator::OnChar)
END_EVENT_TABLE()

DoubleValidator::DoubleValidator(wxString *v, double min, double max) {
    if (!v)
        throw invalid_argument("");
    if (min>max)
        throw range_error("");
    valore=v;
    this->min=min;
    this->max=max;
}

DoubleValidator::DoubleValidator(const DoubleValidator& val) {
    Copy(val);
    min = val.min;
    max = val.max;
    valore = val.valore;
}

void DoubleValidator::OnChar(wxKeyEvent& event) {
    bool skip = true; //se l'evento dev'essere ignorato dalla funzione di gestione di default
    wxChar codice = event.GetUnicodeKey(); //codice del tasto
	bool controlla=wxIsalnum(codice) || codice=='.' || codice==',' || codice =='-';
    if(controlla) {
        wxString vecchio, nuovo; //valore contenuto nella textbox e valore che diventera' quello della textbox
        if(!wxIsdigit(codice) && codice != '.' && codice != ',' && codice != '-')
            skip = false;
        else {
            wxTextCtrl *txt = wxDynamicCast(m_validatorWindow, wxTextCtrl);
            vecchio << txt->GetValue();
            long inizio, fine; //inizio e fine dell'eventuale selezione
            txt->GetSelection(&inizio, &fine); //prendiamo l'eventuale selezione dei caratteri
            nuovo=vecchio.Mid(0, (size_t) inizio); //trasformiamo il vecchio valore nel nuovo
            nuovo.Append((wxChar) codice);
            nuovo.Append(vecchio.Mid((size_t) fine));
            skip = controllaValore(nuovo, false); //verifichiamo che sia accettabile
        }
    }
    if(skip)
        event.Skip();
    else
         wxBell();
}

bool DoubleValidator::controllaValore(const wxString& v, bool valida) {
    if(!valida && v.Cmp(wxT("-")) == 0) { //se stiamo effettuando la validazione in real time ed il carattere e' solo un "-"
        if(min >= 0.0)
            return false;
        else
            return true;
    }
    double numero = 0.0;
    if(!v.ToDouble(&numero)) //se il valore non e' convertibile
        return false;
    if(numero > max)
        return false;
    if(valida) { //se non stiamo effettuando la validazione in real time
        if(numero < min)
            return false;
        return true;
    }
    if(numero < min) { //se stiamo effettuando la validazione in real time dobbiamo verificare che il valore
        //inserito possa diventare con l'aggiunta di ulteriori caratteri il minimo
        wxString minStr;
        minStr.Printf(wxT("%f"), min);
        minStr.Truncate(v.Length());
        double tmpMin;
        if(minStr.ToDouble(&tmpMin)) {
            if(numero < tmpMin)
                return false;
            return true;
        } else
            return false;
    }
    return true;
}


bool DoubleValidator::Validate(wxWindow *parent) {
    wxTextCtrl *txt;
    if (!(txt=wxDynamicCast(m_validatorWindow, wxTextCtrl)))
        return false;
    wxString value = txt->GetValue();
    int i = controllaValore(value, true);
    if (i!=0)
        wxBell();
    return true;
}

bool DoubleValidator::TransferToWindow() {
    wxTextCtrl *txt;
    if (!(txt=wxDynamicCast(m_validatorWindow, wxTextCtrl)))
        return false;
    txt->SetValue(*valore);
    return true;
}

bool DoubleValidator::TransferFromWindow() {
    wxTextCtrl *txt;
    if (!(txt=wxDynamicCast(m_validatorWindow, wxTextCtrl)))
        return false;
    *valore=txt->GetValue();
    return true;
}
