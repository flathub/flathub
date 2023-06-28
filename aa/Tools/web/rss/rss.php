<?php

/*
    ALIEN ARENA RSS FEEDER
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

include ('../config.php');

function mins_to_string ($mins, $long=false)
{
  // reset hours, mins, and secs we'll be using
  $hours = 0;
  $mins = intval ($mins);;
  $t = array(); // hold time periods to return as string
  
  // now handle hours and left-over mins    
  if ($mins >= 60) {
      $hours += (int) floor ($mins / 60);
      $mins = $mins % 60;
    }
    // we're done! now save time periods into our array
    $t['hours'] = (intval($hours) < 10) ? "0" . $hours : $hours;
    $t['mins'] = (intval($mins) < 10) ? "0" . $mins : $mins;
  
  // decide how we should name hours, mins, sec
  $str_hours = ($long) ? "hr" : "hr";

  $str_mins = ($long) ? "min" : "min";

  // build the pretty time string in an ugly way
  $time_string = "";
  $time_string .= ($t['hours']) ? $t['hours'] . " $str_hours" . ((intval($t['hours']) == 1) ? "" : "s") : "";
  $time_string .= ($t['mins']) ? (($t['hours']) ? ", " : "") : "";
  $time_string .= ($t['mins']) ? $t['mins'] . " $str_mins" . ((intval($t['mins']) == 1) ? "" : "s") : "";
//  $time_string .= ($t['hours'] || $t['mins']) ? (($t['secs'] > 0) ? ", " : "") : "";
//  $time_string .= ($t['secs']) ? $t['secs'] . " $str_secs" . ((intval($t['secs']) == 1) ? "" : "s") : "";

  return empty($time_string) ? 0 : $time_string;
} 

function rss_header($title, $description, $link)
{
	header("Content-type: application/xml");
	echo "<?xml version=\"1.0\" ?>\n<rss version=\"2.0\">\n";
	echo "<channel>\n";
	echo "<title>{$title}</title>\n";
	echo "<description>{$description}</description>\n";
	echo "<link>{$link}</link>\n";
}

function rss_footer()
{
	echo "</channel>\n</rss>";
}

function show_popular($lastupdated)
{
	global $CONFIG;
	$query = 'SELECT serverid , SUM( realplayers ) AS playertime , COUNT( serverid ) AS uptime , MAX( realplayers ) AS maxplayers'
        . ' FROM serverlog '
        . ' WHERE time > '.($lastupdated - $CONFIG['duration']).' AND time <= '.$lastupdated
        . ' GROUP BY serverid '
        . ' ORDER BY playertime DESC LIMIT 0, 15';
	/* Max 15 results for RSS feed */

	/* Get all servers from last update which responded */
	//$query  = "SELECT serverid FROM serverlog WHERE time = '{$lastupdated}'";
	$svlog_result = mysql_query($query);
	$numservers = mysql_num_rows($svlog_result);

	$duration = mins_to_string($CONFIG['duration']/60);

	rss_header("Alien Arena :: Popular servers", "Most popular servers over the last {$duration}", $CONFIG['statslink']);

	while($svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC))
	{
		$query  = "SELECT ip, port, hostname, admin, website FROM servers WHERE serverid = '{$svlog_row['serverid']}'";
		$svinfo_result = mysql_query($query);
		$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);
		$svinfo_row['hostname'] = htmlspecialchars($svinfo_row['hostname'], ENT_QUOTES);
		echo "<item>\n";
		if($svinfo_row['hostname'] == "")
			echo "<title>[unnamed server]</title>\n";
		else
			echo "<title>{$svinfo_row['hostname']}</title>\n";
		$svlog_row['playertime'] = mins_to_string($svlog_row['playertime']);
		$svlog_row['uptime'] = mins_to_string($svlog_row['uptime']);
		echo "<description>";
		echo "Playtime {$svlog_row['playertime']}; Peak {$svlog_row['maxplayers']} players";
		if($svinfo_row['admin'] != "")
		{
			$svinfo_row['admin'] = htmlspecialchars($svinfo_row['admin'], ENT_QUOTES);
			echo "; Run by {$svinfo_row['admin']}";
		}
		echo "</description>\n";
		$linkstr = $CONFIG['statslink']."?action=serverinfo&id={$svlog_row['serverid']}";
		$linkstr = htmlspecialchars($linkstr, ENT_QUOTES);
		echo "<link>{$linkstr}</link>\n";
		echo "</item>\n";
	}
	mysql_free_result($svlog_result);
	mysql_free_result($svinfo_result);
	rss_footer();
}

function show_liveplayers($lastupdated)
{
	global $CONFIG;

	$query = 'SELECT serverlogid, serverid, mapname, realplayers'
			.' FROM serverlog'
			.' WHERE time = '.$lastupdated.' AND realplayers > 0'
			.' ORDER BY realplayers'
			.' DESC LIMIT 0, 15';
	/* Max 15 results for RSS feed */

	$svlog_result = mysql_query($query);
	$numservers = mysql_num_rows($svlog_result);

	$duration = mins_to_string($CONFIG['duration']/60);

	rss_header("Alien Arena :: Live players", "Real humans online now", $CONFIG['statslink']);

	while($svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC))
	{
		$query = 'SELECT ip, port, hostname, admin, website'
				.' FROM servers'
				.' WHERE serverid = '.$svlog_row['serverid'];
		$svinfo_result = mysql_query($query);
		$query = 'SELECT name, score, ping'
				.' FROM playerlog'
				.' WHERE serverlogid = '.$svlog_row['serverlogid'].' AND ping > 0'
				.' ORDER BY score DESC';
		$pllog_result = mysql_query($query);

		$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);
		$svinfo_row['hostname'] = htmlspecialchars($svinfo_row['hostname'], ENT_QUOTES);
		echo "<item>\n";
		if($svinfo_row['hostname'] == "")
			echo "<title>[unnamed server]</title>\n";
		else
			echo "<title>{$svinfo_row['hostname']}</title>\n";
		echo "<description>";
		echo $svlog_row['realplayers'].' players: ';
		while($pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC))
		{
			echo "{$pllog_row['name']} ({$pllog_row['score']}) ";
		}

		echo "</description>\n";
		$linkstr = $CONFIG['statslink']."?action=serverinfo&id={$svlog_row['serverid']}";
		$linkstr = htmlspecialchars($linkstr, ENT_QUOTES);
		echo "<link>{$linkstr}</link>\n";
		echo "</item>\n";
	}
	mysql_free_result($pllog_result);
	mysql_free_result($svlog_result);
	mysql_free_result($svinfo_result);
	rss_footer();
}

/****************************
 ****************************
   PROGRAM BODY STARTS HERE
 ****************************
 ****************************/


/* Case-sensitive parameters to parse result for */
$parameters = array('show');
/* Parse url for valid parameters */
$control = array();
foreach ($_GET as $key => $value)
{
        if(!(array_search($key, $parameters) === FALSE))
                $control[$key] = $value;
}

/* Set defaults */
if(!array_key_exists('show', $control))
        $control['show'] = 'popular';

$conn = mysql_connect($CONFIG['dbHost'], $CONFIG['dbUser'], $CONFIG['dbPass']) or die ('Error connecting to database');

mysql_select_db($CONFIG['dbName']);

/* Get time of last database update */
$query  = "SELECT lastupdated FROM stats WHERE id = '0'";
$result = mysql_query($query);
$row = mysql_fetch_array($result, MYSQL_ASSOC);
$lastupdated = $row['lastupdated'];
mysql_free_result($result);

switch($control['show'])
{
	case 'popular':
		show_popular($lastupdated);
	break;
	case 'liveplayers':
		show_liveplayers($lastupdated);
	break;
	default:
		mysql_close($conn);
		return;
	break;
}

mysql_close($conn);

?> 
