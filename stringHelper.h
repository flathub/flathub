#ifndef HEADER_A3856DA34CF0ED12
#define HEADER_A3856DA34CF0ED12

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

#ifndef _STRING_HELPER_H_
#define _STRING_HELPER_H_

#include <sstream>
#include <wx/string.h>

class stringHelper {
	public:
		static string IntToStr(size_t i) {
			ostringstream ss;
			ss<<i;
			return ss.str();
		}
		static wxString IntToWxStr(size_t i) {
			wxString s;
#ifdef _WIN32
            s.Printf("%Id", i);
        #else
			s.Printf("%zd", i);
#endif // _WIN32
			return s;
		}
};
#endif
#endif // header guard 

