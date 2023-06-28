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

function GenerateSearchInput($searchtype = "serversearch")
{
		$filename = GetFilename();
		echo "<form action=\"".$filename."?action=".$searchtype."\" method=\"post\">";
		echo "<p class=cdbody>Search: <input name=\"searchstring\" type=\"text\">"; 
		echo "<input type=\"submit\"></p>";
		echo "</form>";
}

function GenerateServerInfo(&$control)
{
	global $CONFIG;
	echo "<p class=cdbody>Server information for the last ".mins_to_string(($control['endtime'] - $control['starttime'])/60)."</p>\n";
//	echo "<p class=cdbody>Eventually there will be lots more information on uptimes, peak usage times, players that have played on it here.</p>\n";
	
	if($control['id'] == "")
	{
		echo "<p class=cdbody>No id specified</p>\n";
		return;
	}

	$query  = "SELECT ip, port, hostname, admin, website, version FROM servers WHERE serverid = '".$control['id']."'";
	$sv_result = mysql_query($query);
	$sv_row = mysql_fetch_array($sv_result, MYSQL_ASSOC);
	
	$query = 'SELECT SUM( realplayers ) AS playertime , COUNT( serverid ) AS uptime , MAX( realplayers ) AS maxplayers , SUM( realplayers ) / COUNT( serverid ) as popularity '
		. ' FROM serverlog '
		. ' WHERE time > '.$control['starttime'].' AND time <= '.$control['endtime']
		. ' AND serverid = \''.$control['id'].'\' '
		. ' GROUP BY serverid ';
				
	//echo $query;
				
	$svlog_result = mysql_query($query);
	$svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC);


	echo "<table id=cdtable>";
	echo "<tr>";
	echo "<th>IP & port</th>";
	echo "<th>Hostname</th>";
	echo "<th>Admin</th>";
	echo "<th>Uptime</th>";
	echo "<th>Total player time</th>";
	echo "<th>Most players at once</th>";
	echo "<th>Popularity</th>";
	echo "<th>Server version</th>";
	echo "</tr>\n";

	echo "<tr>";
	echo "<td>{$sv_row['ip']} port {$sv_row['port']}</td>";
	echo "<td>{$sv_row['hostname']} ";
	if($sv_row['website'] != "")
	{
		echo "<a href=\"{$sv_row['website']}\"><img border=0 alt=www src=\"img/www.gif\"></a>";
	}
	echo "</td>";
	echo "<td>{$sv_row['admin']}</td>";
	echo "<td>".mins_to_string($svlog_row['uptime'])."</td>";
	echo "<td>".mins_to_string($svlog_row['playertime'])."</td>";
	echo "<td>{$svlog_row['maxplayers']}</td>";
	echo "<td>{$svlog_row['popularity']}</td>";
	echo "<td>{$sv_row['version']}</td>";
	echo "</tr>\n";
	echo "</table>";
	mysql_free_result($sv_result);	
	echo "<br>\n";
	echo "<img width={$CONFIG['graphwidth']} height={$CONFIG['graphheight']} alt=\"Usage graph\" src=\"graph.php?show=server&amp;id={$control['id']}\">\n";
	
}

function DoServerSearch(&$control, $searchstring)
{
	$filename = GetFilename();

	$searchstring = $_POST['searchstring'];
	
	if($searchstring != "")
	{ /* No server to show, but a string to search with */
		
		$query = 'SELECT serverid, hostname '
		        . ' FROM servers '
				. ' WHERE hostname LIKE \'%'.$searchstring.'%\''
		        . ' GROUP BY hostname ';
					
		$sv_result = mysql_query($query);

		echo '<p class=cdbody>'.mysql_num_rows($sv_result).' results for \''.$searchstring.'\'</p>';
		
		echo "<p style=cdbody>";
		while($sv_row = mysql_fetch_array($sv_result, MYSQL_ASSOC))
		{
			$control['action'] = 'serverinfo';
			$control['id'] = $sv_row['serverid'];
			echo "<a href=\"".$filename."?".http_build_query($control)."\">";

			echo $sv_row['hostname']."</a><br>\n";
		} 
		mysql_free_result($svlog_result);
		echo "</p>\n";
	}
	else
	{ 	/* User did null search? */
		echo '<p class=cdbody>Oops, looks like you forgot to enter a search string. :)</p>';
	}
}


function GenerateMapInfo(&$control)
{
	echo "<p class=cdbody>Map information for the last ".mins_to_string(($control['endtime'] - $control['starttime'])/60)."</p>\n";
	
	if($control['id'] == "")
	{
		echo "<p class=cdbody>No id specified</p>\n";
		return;
	}

	$query = 'SELECT SUM( realplayers ) as playertime , COUNT( realplayers ) as servedtime, MAX( realplayers ) AS maxplayers'
	        . ' FROM serverlog '
			. ' WHERE mapname = \''.$control['id'].'\''
	        . ' AND time > '.$control['starttime'].' AND time <= '.$control['endtime']
	        . ' GROUP BY mapname ';
//	        . ' ORDER BY '. $control['orderby'] .' '.$control['sort'].' LIMIT 0, '.$control['numresults'];

	$svlog_result = mysql_query($query);

	echo "<table id=cdtable>\n";
	echo "<tr>";
	echo "<th>Map name</th>";
	echo "<th>Total player time</th>";
	echo "<th>Time served</th>";	
	echo "<th>Most players at once</th>";
	echo "</tr>\n";
	
	$svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC);
	echo "<tr>";
	echo "<td>{$control['id']}</td>";
	echo "<td>".mins_to_string($svlog_row['playertime'])."</td>";
	echo "<td>".mins_to_string($svlog_row['servedtime'])."</td>";
	echo "<td>{$svlog_row['maxplayers']}</td>";
	echo "</tr>\n";

	mysql_free_result($svlog_result);

	echo "</table>\n";

	echo "<p class=cdbody>\n";
	ShowMapImage($control['id'], $thumbnail=0, $addlink=0);
	echo "</p>\n";

}

?>
