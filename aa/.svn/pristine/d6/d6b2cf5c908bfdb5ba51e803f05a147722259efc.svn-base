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

function GenerateLivePlayerTable(&$control)
{
	$lastupdated = GetLastUpdated();
	$filename = GetFilename();
	/* Get all servers from last update which responded */
	
	/*  Section to display player list */
	$query  = "SELECT COUNT(name) AS numplayers FROM playerlog WHERE time = '{$lastupdated}' AND ping != '0'";
	$pl_result = mysql_query($query);
	$numplayers = mysql_fetch_array($pl_result);
	$numplayers = $numplayers['numplayers'];
	
	$query  = " SELECT serverlogid, serverid, mapname, realplayers"
			. " FROM serverlog"
			. " WHERE time = '{$lastupdated}' AND realplayers > '0'"
			. " ORDER BY realplayers DESC";
	$sv_result = mysql_query($query);

	$numservers = mysql_num_rows($sv_result);

	echo "<p class=\"cdsubtitle\">{$numplayers} players using {$numservers} servers<p>\n";

	/* Section to build table of servers */
	echo "<p>\n<table id=cdtable>\n";
	echo "<tr><th>Player</th><th>Score</th><th>Ping</th><th>Server</th><th>Country</th><th>Map</th></tr>\n";

	while($sv_row = mysql_fetch_array($sv_result, MYSQL_ASSOC))
	{	
		$query  = " SELECT name, score, ping FROM playerlog"
				. " WHERE serverlogid = '{$sv_row['serverlogid']}' AND ping > '0'"
				. " ORDER BY score DESC";
		$pl_result = mysql_query($query);
		$pl_numrows = mysql_num_rows($pl_result);  /* Get number of players (rows) in mysql result */
		$query  = "SELECT ip, port, hostname, admin, website FROM servers WHERE serverid = '{$sv_row['serverid']}'";
		$svinfo_result = mysql_query($query);
		$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);

		while($pl_row = mysql_fetch_array($pl_result, MYSQL_ASSOC))
		{
		    echo "<tr>";

			echo "<td><b>".GenerateInfoLink("player", $pl_row['name'])."</b></td>";
			echo "<td>{$pl_row['score']}</td>";
			echo "<td>{$pl_row['ping']} ms</td>";
							
			echo "<td><a href=\"{$filename}?action=serverinfo&amp;id={$sv_row['serverid']}\">".LimitString($svinfo_row['hostname'],40)."</a>";
			echo "</td>";
		
			echo '<td>';
			$cc = GetCountryCode($svinfo_row['ip']);
			ShowCountryFlag($cc);
			echo '  '.GetCountryName($cc);
			echo '</td>';

			echo "<td>";
				echo "<a href=\"{$filename}?action=mapinfo&amp;id={$sv_row['mapname']}\">";
				echo $sv_row['mapname'].'</a>';
			echo "</td>";

			echo "</tr>\n";
		}

		mysql_free_result($pl_result);
	} 

	mysql_free_result($sv_result);

	echo "</table>\n";
}

function GenerateTotalPlayers(&$control)
{
	$query = 'select count(distinct name) as total_players from playerlog;';
	$result = mysql_query($query);
	$row = mysql_fetch_array($result, MYSQL_ASSOC);
	
	echo '<p class="cdsubtitle">'.$row['total_players'].' unique players in the last '.$control['history'].' hours</p>';
}

function GeneratePlayerTable(&$control)
{
  $endtime = GetLastUpdated();
  $starttime = $endtime - $control['history']*60*60;
  
	$query = 'SELECT name, SUM( score ) AS totalscore, COUNT(name) as playertime, SUM( score )/COUNT(name) as fragrate'
	        . ' FROM playerlog'
	        . ' WHERE ping >0 AND name != \'Player\''
	        . ' AND time > '.$starttime.' AND time <= '.$endtime			
	        . ' GROUP BY name'
	        . ' ORDER BY '.$control['orderby'].' '.$control['sort']
	        . ' LIMIT 0 , '.$control['results'];
	
	//echo $query."<br>\n";
	
	$pllog_result = mysql_query($query);
	if($pllog_result === FALSE)
	{
		echo "<p class=\"cdbody\">Unable to display stats at this time.</p>\n";
		return;
	}
		
	echo "<table id=cdtable>\n";
	echo "<tr>";
	Insert_Table_Sorter($control, $display = 'Name', $orderby = 'name'); 
//	Insert_Table_Sorter($control, $display = 'Total score', $orderby = 'totalscore'); 
	Insert_Table_Sorter($control, $display = 'Time played', $orderby = 'playertime'); 
//	Insert_Table_Sorter($control, $display = 'Fragrate', $orderby = 'fragrate'); 
	echo "</tr>\n";

	while($pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC))
	{
	    echo "<tr>";
		
		echo "<td>".GenerateInfoLink("player", $pllog_row['name'])."</td>";
		//		echo "<td>{$pllog_row['totalscore']}</td>";
		echo "<td>".MinutesToString($pllog_row['playertime'])."</td>";
//		echo "<td>{$pllog_row['fragrate']}</td>";
		echo "</tr>\n";
	} 
	mysql_free_result($pllog_result);

	echo "</table>\n";
}

function GeneratePlayerInfo(&$control)
{
	/* Find time of last database update */
	$endtime = GetLastUpdated();
	$starttime = $endtime - $control['history']*60*60;

	$query = 'SELECT COUNT(name) as playertime'
	        . ' FROM playerlog '
    		. ' WHERE name = \''.$control['id'].'\''
	        . ' AND time > '.$starttime.' AND time <= '.$endtime
	        . ' GROUP BY name ';
//	        . ' ORDER BY '. $control['orderby'] .' '.$control['sort'].' LIMIT 0, '.$control['numresults'];

	$pllog_result = mysql_query($query);
	$pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC);

	echo "<p class=\"cdsubtitle\">Player information covering the last {$control['history']} hours</p>\n";

	echo "<table id=cdtable>\n";

	echo "<tr><th>Name</th>";
	echo "<td>{$control['id']}</td></tr>\n";

	echo "<tr><th>Time playing</th>";
	echo "<td>".MinutesToString($pllog_row['playertime'])."</td></tr>\n";
		
	echo "</table>\n";
	mysql_free_result($pllog_result);
	
	/* Show which servers this player has been playing on */
	$query = 'SELECT serverid, mapname, COUNT( serverid ) as playertime, ROUND(AVG(ping),1) as avgping'
//			. ' GROUP_CONCAT(DISTINCT name ORDER BY name DESC SEPARATOR \', \')'
	        . ' FROM playerlog '
    		. ' WHERE name = \''.$control['id'].'\''
	        . ' AND time > '.$starttime.' AND time <= '.$endtime
			. ' AND ping > 0'
	        . ' GROUP BY serverid'
			. ' ORDER BY playertime DESC';
	$svlog_result = mysql_query($query);
	$num_servers = mysql_num_rows($svlog_result);	
	
	if($num_servers > 0)
	{
		echo "<p class=cdsubtitle>{$control['id']} has played on {$num_servers} servers</p>\n";

		if($num_servers > 50)
			echo "<p class=cdbody>Top 50 results shown</p>";
		
		echo "<table id=cdtable>\n";
		echo "<tr><th>Hostname</th><th>Player time</th><th>Average ping</th></tr>\n";

		$count = 0;
		
		while(($svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC)) && ($count++ < 50))
		{
			$query  = "SELECT ip, port, hostname FROM servers WHERE serverid = '{$svlog_row['serverid']}'";
			$svinfo_result = mysql_query($query);
			$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);

			echo "<tr>";
			echo "<td>".GetServerNameFromID($svlog_row['serverid'])."</td>";
			echo "<td>".MinutesToString($svlog_row['playertime'])."</td>";
			echo "<td>".$svlog_row['avgping']." ms</td>";
			echo "</tr>\n";
		}
		echo "</table>\n";
	}
	mysql_free_result($svlog_result);	

	/* Now get a list of maps that this player has played on */
	$query = 'SELECT time, mapname, COUNT( mapname ) as playertime'
		. ' FROM playerlog '
		. ' WHERE name = \''.$control['id'].'\''
		. ' AND ping > 0'
		. ' AND time > '.$starttime.' AND time <= '.$endtime
		. ' GROUP BY mapname '
		. ' ORDER BY playertime DESC';

//	echo "Query=".$query."<br>\n";

	$pllog_result = mysql_query($query);
	$num_maps = mysql_num_rows($pllog_result);
	
	if($num_maps > 0)
	{
		echo "<p class=cdsubtitle>{$control['id']} has played on {$num_maps} maps</p>\n";

		if($num_maps > 50)
			echo "<p class=cdbody>Top 50 times shown</p>";
		
		echo "<table id=cdtable>\n";
		echo "<tr><th>Map name</th><th>Time</th></tr>\n";

		$count = 0;
		
		while(($pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC)) && ($count++ < 50))
		{
			echo "<tr>";
			echo "<td>".GenerateInfoLink("map", $pllog_row['mapname'])."</td>";
			echo "<td>".MinutesToString($pllog_row['playertime'])."</td>";
			echo "</tr>\n";
		}
		echo "</table>\n";
	}
	mysql_free_result($pllog_result);
		
		return;
		
	/* Show which servers this player has been playing on */
	$query = 'SELECT score, MIN(time) as starttime, MAX(time) as endtime, CONCAT_WS(\',\', serverid, mapname) as serverandmap, serverid, mapname, COUNT( serverid ) as playertime'
//			. ' GROUP_CONCAT(DISTINCT name ORDER BY name DESC SEPARATOR \', \')'
	        . ' FROM playerlog '
    		. ' WHERE name = \''.$control['id'].'\''
	        . ' AND time > '.$starttime.' AND time <= '.$endtime
			. ' AND ping > 0'
	        . ' GROUP BY serverandmap'
			. ' ORDER BY starttime ASC';
	$pllog_result = mysql_query($query);
	$num_matches = mysql_num_rows($pllog_result);
	
	if($num_matches > 0)
	{
		echo "<p class=cdsubtitle>Experimental: Tracking {$control['id']} across {$num_matches} matches</p>\n";

		if($num_maps > 50)
			echo "<p class=cdbody>First 50 matches shown</p>";
		
		echo "<table id=cdtable>\n";
		echo "<tr><th>Start time</th><th>End time</th><th>Server</th><th>Map</th><th>Time</th><th>Score progress by minute</th></tr>\n";

		$count = 0;
		
		while(($pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC)) && ($count++ < 50))
		{

			echo "<tr>";
			echo "<td>".date("H:i:s", $pllog_row['starttime'])." GMT</td>\n";
			echo "<td>".date("H:i:s", $pllog_row['endtime'])." GMT</td>\n";
			
			echo "<td>".GetServerNameFromID($pllog_row['serverid'])."</td>\n";
			
			echo "<td>{$pllog_row['mapname']}</td>\n";
			echo "<td>".MinutesToString($pllog_row['playertime'])."</td>\n";
			/* Get final score */
/*			$query = 'SELECT score'
			        . ' FROM playerlog '
		    		. ' WHERE time = '.$pllog_row['endtime']
					. ' AND name = \''.$control['id'].'\'';
			$score_result = mysql_query($query);
			$score_row = mysql_fetch_array($score_result, MYSQL_ASSOC);*/

			/* Get list of scores for match */
			$query = 'SELECT score'
			        . ' FROM playerlog '
		    		. ' WHERE time >= '.($pllog_row['starttime'])
		    		. ' AND time <= '.($pllog_row['endtime']+480)
					. ' AND name = \''.$control['id'].'\''
					. ' AND mapname = \''.$pllog_row['mapname'].'\''
					. ' ORDER BY time ASC';
			$score_result = mysql_query($query);
//			$score_rows = mysql_num_rows($score_result);
			echo "<td>";
			while($score_row = mysql_fetch_array($score_result, MYSQL_ASSOC))
				echo "{$score_row['score']} ";
			echo "</td>\n";
			echo "</tr>\n";
		}
		echo "</table>\n";
	}
	mysql_free_result($pllog_result);			
	
}

function DoPlayerSearch(&$control)
{
	$searchstring = addslashes($_POST['searchstring']);
	
	if($searchstring != "")
	{

		$query = 'SELECT name'
	        . ' FROM playerlog '
    			. ' WHERE name LIKE \'%'.$searchstring.'%\''
	        . ' GROUP BY name ';
					
		$pllog_result = mysql_query($query);
		if($pllog_result === FALSE)
		{
			echo "<p class=\"cdbody\">Unable to perform search at this time.</p>\n";
			return;
		}

		echo '<p class=cdsubtitle>'.mysql_num_rows($pllog_result).' results for \''.$searchstring.'\'</p>';
		
		echo "<p style=cdbody>";
		while($pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC))
		{
			$control['action'] = 'playerinfo';
			$control['id'] = $pllog_row['name'];
			echo "<a href=\"".Generate_URL($control)."\">".$pllog_row['name']."</a><br>\n";
		} 
		mysql_free_result($pllog_result);
		echo "</p>\n";
	}
	else
	{ 	/* User did null search? */
		echo '<p class=cdbody>Oops, looks like you forgot to enter a search string. :)</p>';
	}
}



?>
