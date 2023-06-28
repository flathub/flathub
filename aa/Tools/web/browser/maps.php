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

function ShowMapImage($mapname, $thumbnail = 0, $addlink = 1)
{
	$filename = GetFilename();
	$info = '';  /* Function returns an information string */
	if($thumbnail)
	{   /* Thumbnail sizes */
		$width = 80;
		$height = 60;
	}
	else
	{	/* Normal sizes */
		$width = 341;
		$height = 256;
	}
	
	if($addlink)
		echo "<a href=\"{$filename}?action=mapinfo&amp;id={$mapname}\">";

	echo "<img border=1 alt={$mapname} width={$width} height={$height} src=\"";
	
	$mapfile = "maps/1st/{$mapname}_{$width}x{$height}.jpg";
	if(file_exists($mapfile))
	{
		echo $mapfile;  /* 1st party map */
		$info = "supplied with the game";
	}
	else
	{
		$mapfile = "maps/3rd/{$mapname}_{$width}x{$height}.jpg";
		if(file_exists($mapfile))
		{	/* 3rd party map */
			echo $mapfile;
			$info = "3rd party add-on";
		}
		else
		{ /* Unknown map */
			echo "maps/default_{$width}x{$height}.jpg";
			$info = "unknown - a new map!";
		}
	}
	echo "\"><br>";
	if($addlink)
		echo "</a>";

	return $info;  
}

function GenerateMapTable(&$control)
{

	$filename = GetFilename();
  $endtime = GetLastUpdated();
  $starttime = $endtime - $control['history']*60*60;

	$query = 'SELECT mapname , SUM( realplayers ) as playertime , COUNT( realplayers ) as servedtime , MAX( realplayers ) AS maxplayers'
	        . ' FROM serverlog '
	        . ' WHERE time > '.$starttime.' AND time <= '.$endtime
	        . ' GROUP BY mapname '
	        . ' ORDER BY '. $control['orderby'] .' '.$control['sort'].' LIMIT 0, '.$control['results'];

	$svlog_result = mysql_query($query);
	if($svlog_result === FALSE)
	{
		echo "<p class=\"cdbody\">Unable to display stats at this time.</p>\n";
		return;
	}
	
	echo "<table id=cdtable>\n";
	echo "<tr>";
	Insert_Table_Sorter($control, $display = 'Map name', $orderby = 'mapname'); 
	Insert_Table_Sorter($control, $display = 'Total player time', $orderby = 'playertime'); 
	Insert_Table_Sorter($control, $display = 'Time served', $orderby = 'servedtime'); 
	Insert_Table_Sorter($control, $display = 'Most players at once', $orderby = 'maxplayers'); 
	echo "</tr>\n";

	while($svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC))
	{
	    echo "<tr>";
		echo "<td>";
			ShowMapImage($mapname=$svlog_row['mapname'], $thumbnail=1, $addlink=1);
			echo "{$svlog_row['mapname']}";
/*		<a href=\"{$filename}?action=mapinfo&amp;id={$svlog_row['mapname']}\">{$svlog_row['mapname']}</a> */
		echo "</td>";
		echo "<td>".MinutesToString($svlog_row['playertime'])."</td>";
		echo "<td>".MinutesToString($svlog_row['servedtime'])."</td>";
		echo "<td>{$svlog_row['maxplayers']}</td>";
		echo "</tr>\n";
	} 
	mysql_free_result($svlog_result);

	echo "</table>\n";
}


function GenerateMapInfo(&$control)
{
  /* Find time of last database update */
  $endtime = GetLastUpdated();
  $starttime = $endtime - $control['history']*60*60;

  $filename = GetFilename();
  
	$query = 'SELECT SUM( realplayers ) as playertime , COUNT( realplayers ) as servedtime, MAX( realplayers ) AS maxplayers'
	        . ' FROM serverlog '
    		. ' WHERE mapname = \''.$control['id'].'\''
	        . ' AND time > '.$starttime.' AND time <= '.$endtime
	        . ' GROUP BY mapname ';
//	        . ' ORDER BY '. $control['orderby'] .' '.$control['sort'].' LIMIT 0, '.$control['numresults'];

	$svlog_result = mysql_query($query);
	$svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC);

	echo "<p class=\"cdsubtitle\">Map information covering the last {$control['history']} hours</p>\n";

	echo "<p class=cdbody>\n";
	$info = ShowMapImage($control['id'], $thumbnail=0, $addlink=0);
	echo "</p>\n";

	
	echo "<table id=cdtable>\n";

	echo "<tr><th>Map name</th>";
	echo "<td>{$control['id']}</td></tr>\n";

	echo "<tr><th>Info</th>";
	echo "<td>{$info}</td></tr>\n";
	
	echo "<tr><th>Total player time</th>";
	echo "<td>".MinutesToString($svlog_row['playertime'])."</td></tr>\n";
	
	echo "<tr><th>Time served</th>";
	echo "<td>".MinutesToString($svlog_row['servedtime'])."</td>\n";
  	
	echo "<tr><th>Most players at once</th>";
	echo "<td>{$svlog_row['maxplayers']}</td></tr>\n";
	
	echo "</table>\n";

	mysql_free_result($svlog_result);
	
	/* Now get a list of (real) players that have used this map */
	$query = 'SELECT name, COUNT( name ) as time'
		. ' FROM playerlog '
		. ' WHERE mapname = \''.$control['id'].'\''
		. ' AND ping > 0'
		. ' AND time > '.$starttime.' AND time <= '.$endtime
		. ' GROUP BY name '
		. ' ORDER BY time DESC';

//	echo "Query=".$query."<br>\n";

	$pllog_result = mysql_query($query);
	$num_players = mysql_num_rows($pllog_result);
	
	if($num_players > 0)
	{
		echo "<p class=cdsubtitle>{$num_players} players have been using {$control['id']}</p>\n";

		if($num_players > 50)
			echo "<p class=cdbody>Top 50 results shown</p>";
		
		echo "<table id=cdtable>\n";
		echo "<tr><th>Name</th><th>Time</th></tr>\n";

		$count = 0;
		
		while(($pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC)) && ($count++ < 50))
		{
			echo "<tr>";
			if($pllog_row['name'] == 'Player')
				echo "<td>".GenerateInfoLink("player", "Player")." <i>(cumilative time)</i></td>";
			else
				echo "<td>".GenerateInfoLink("player", $pllog_row['name'])."</td>";
			echo "<td>".MinutesToString($pllog_row['time'])."</td>";
			echo "</tr>\n";
		}
		echo "</table>\n";
	}
	mysql_free_result($pllog_result);
	
	/* Show which servers this map has been served from */
	$query = 'SELECT serverid, COUNT( serverid ) as servedtime, SUM( realplayers ) as playertime, MAX( realplayers ) AS maxplayers'
	        . ' FROM serverlog '
    		. ' WHERE mapname = \''.$control['id'].'\''
	        . ' AND time > '.$starttime.' AND time <= '.$endtime
	        . ' GROUP BY serverid '
			. ' ORDER BY playertime DESC';
	$svlog_result = mysql_query($query);
	$num_servers = mysql_num_rows($svlog_result);	
	
	if($num_servers > 0)
	{
		echo "<p class=cdsubtitle>{$num_servers} servers have hosted {$control['id']}</p>\n";

		if($num_servers > 50)
			echo "<p class=cdbody>Top 50 results shown</p>";
		
		echo "<table id=cdtable>\n";
		echo "<tr><th>Hostname</th><th>Served time</th><th>Player time</th><th>Max players</th></tr>\n";

		$count = 0;
		
		while(($svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC)) && ($count++ < 50))
		{
			$query  = "SELECT ip, port, hostname FROM servers WHERE serverid = '{$svlog_row['serverid']}'";
			$svinfo_result = mysql_query($query);
			$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);

			echo "<tr>";
			echo "<td>".GetServerNameFromID($svlog_row['serverid'])."</td>";
			echo "<td>".MinutesToString($svlog_row['servedtime'])."</td>";
			echo "<td>".MinutesToString($svlog_row['playertime'])."</td>";
			echo "<td>{$svlog_row['maxplayers']}</td>";
			echo "</tr>\n";
		}
		echo "</table>\n";
	}
	mysql_free_result($svlog_result);	
}

function DoMapSearch(&$control)
{
	$filename = GetFilename();

	$searchstring = addslashes($_POST['searchstring']);
	
	if($searchstring != "")
	{

    $query = 'SELECT mapname'
	        . ' FROM serverlog '
    			. ' WHERE mapname LIKE \'%'.$searchstring.'%\''
	        . ' GROUP BY mapname ';
					
		$svlog_result = mysql_query($query);
		if($svlog_result === FALSE)
		{
			echo "<p class=\"cdbody\">Unable to perform search at this time.</p>\n";
			return;
		}

		echo '<p class=cdsubtitle>'.mysql_num_rows($svlog_result).' results for \''.$searchstring.'\'</p>';
		
		echo "<p style=cdbody>";
		while($svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC))
		{
			$control['action'] = 'mapinfo';
			$control['id'] = $svlog_row['mapname'];
			echo "<a href=\"".Generate_URL($control)."\">".$svlog_row['mapname']."</a><br>\n";
		} 
		mysql_free_result($svlog_result);
		echo "</p>\n";
	}
	else
	{ 	/* User did null search? */
		echo '<p class=cdbody>Oops, looks like you forgot to enter a search string. :)</p>';
	}
}


?>
