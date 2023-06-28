<?php

/*
    ALIEN ARENA WEB SERVER BROWSER
    Copyright (C) 2007 Tony Jackson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Tony Jackson can be contacted at tonyj@cooldark.com
*/

/* General configuration */
$CONFIG['title'] = 'Alien Arena :: Server Stats Browser';
$CONFIG['baseurl'] = 'http://www.chaingun.org/alienarena/tools/'; /* Trailing slash important */
$CONFIG['stylesheet'] = $CONFIG['baseurl'].'style.css';
$CONFIG['dbHost'] = 'localhost';
$CONFIG['dbName'] = 'your_db_name_here';
$CONFIG['dbUser'] = 'your_db_username_here';
$CONFIG['dbPass'] = 'your_db_password_here';

/* Contact e-mail shown when master uncontactable or mysql problems */
$CONFIG['contact'] = 'your@email.here';

/* Graphs that are shown on browser pages */
$CONFIG['graphwidth'] = 500;
$CONFIG['graphheight'] = 100;

/* Server scanner - duration (in seconds) to keep server
   and player data in the database */
$CONFIG['dbExpire'] = 60 * 60 * 24;  /* Default - 24 hours */

/* RSS "popular servers" feed - duration in seconds to consider */
$CONFIG['duration'] = 6*60*60; /* Default - 6 hours */
$CONFIG['statslink'] = $CONFIG['baseurl'].'browser/index.php';

error_reporting (E_NONE);

?>
