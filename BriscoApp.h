#ifndef HEADER_4535FE1ECA93D1CE
#define HEADER_4535FE1ECA93D1CE

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

#ifndef _BRISCOAPP_H_

#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/intl.h>
#include "BriscoFrame.h"

class BriscoApp : public wxApp {
	private:
		BriscoFrame *f;
	public:
		virtual bool OnInit();
};

DECLARE_APP(BriscoApp)
IMPLEMENT_APP(BriscoApp)

#endif
#endif // header guard

