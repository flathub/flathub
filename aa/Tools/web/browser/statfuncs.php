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

function GetFilename()
{
	$filename = explode('/', $_SERVER["REQUEST_URI"]);
	$filename = explode('?', $filename[count($filename)-1]);
	//print_r($filename);
	if($filename == "")
		$filename = "stats.php";  /* For when just directory is accessed */
	return $filename[0];
}

function GetLastUpdated()
{
	$query  = "SELECT lastupdated FROM stats WHERE id = '0'";
	$result = mysql_query($query);
	$row = mysql_fetch_array($result, MYSQL_ASSOC);
	$lastupdated = $row['lastupdated'];
	mysql_free_result($result);
	return $lastupdated;
}

function ShowMapImage($mapname, $thumbnail = 0, $addlink = 1)
{
	$filename = GetFilename();
	$title = '';
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

	echo "<img border=0 alt={$mapname} width={$width} height={$height} src=\"";
	
	$mapfile = "maps/1st/{$mapname}_{$width}x{$height}.jpg";
	if(file_exists($mapfile))
	{
		echo $mapfile;  /* 1st party map */
		$title = "{$mapname} (supplied with the game)";
	}
	else
	{
		$mapfile = "maps/3rd/{$mapname}_{$width}x{$height}.jpg";
		if(file_exists($mapfile))
		{	/* 3rd party map */
			echo $mapfile;
			$title = "{$mapname} (3rd party add-on)";
		}
		else
		{
			echo "maps/default_{$width}x{$height}.jpg";
			$title = "{$mapname} (unknown - a new map!)";
		}
	}
	echo "\"><br>";
	if($thumbnail) /* Short description for thumbnails */
		echo $mapname;
	else
		echo $title;

	if($addlink)
		echo "</a>";

}

function GenerateLiveServerTable(&$control)
{
	$lastupdated = GetLastUpdated();
	$filename = GetFilename();
	/* Get all servers from last update which responded */
	$query  = "SELECT serverlogid, serverid, mapname, realplayers FROM serverlog WHERE time = '{$lastupdated}' ORDER BY realplayers DESC";
	$sv_result = mysql_query($query);

	$numservers = mysql_num_rows($sv_result);

	echo "<p class=\"cdbody\">{$numservers} live servers:</p>\n";

	/* Section to build table of servers */
	echo "<p>\n<table id=cdtable>\n";
	echo "<tr><th>Server</th><th>Map</th><th colspan=3>Players</th></tr>\n";

	while($sv_row = mysql_fetch_array($sv_result, MYSQL_ASSOC))
	{	
		$query  = "SELECT name, score, ping FROM playerlog WHERE serverlogid = '{$sv_row['serverlogid']}' ORDER BY score DESC";
		$pl_result = mysql_query($query);
		$pl_numrows = mysql_num_rows($pl_result);  /* Get number of players (rows) in mysql result */
		$query  = "SELECT ip, port, hostname, admin, website FROM servers WHERE serverid = '{$sv_row['serverid']}'";
		$svinfo_result = mysql_query($query);
		$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);

		if($pl_numrows > 4)
			$rowspan = $pl_numrows;
		else
			$rowspan = 4;
		
	    echo "<tr>";
		//echo "<td>{$svinfo_row['ip']}:{$svinfo_row['port']}</td>";
		
		/* If hostname is too long to display, trim and add ... to end of string */
		if(strlen($svinfo_row['hostname']) > 40)
			$svinfo_row['hostname']=substr($svinfo_row['hostname'], 0, 37)."...";
					
		echo "<td><a href=\"{$filename}?action=serverinfo&amp;id={$sv_row['serverid']}\">{$svinfo_row['hostname']}</a><br>";
		echo "</td>";
//		echo "<td>Admin: {$svinfo_row['admin']}</td>\n";

		echo "<td rowspan={$rowspan}>";
			ShowMapImage($mapname=$sv_row['mapname'], $thumbnail=1, $addlink=1);
		echo "</td>";
		
		$count = 0;
		while(($pl_row = mysql_fetch_array($pl_result, MYSQL_ASSOC)) or $count < 4)
		{
			
			if($count > 0)  /* no tr tag for first row, completing row above */
			{
				echo "<tr>";
				switch($count)
				{
					/* These are the rows below servername, admin etc */
					case 1:
						echo "<td>&nbsp;&nbsp;&nbsp;&nbsp;{$svinfo_row['ip']} port {$svinfo_row['port']}</td>";
					break;
					case 2:
						echo "<td>";
						if($svinfo_row['admin'] != '')
							echo "&nbsp;&nbsp;&nbsp;&nbsp;Admin: {$svinfo_row['admin']}";
						echo "</td>"; 
					break;
					case 3:
						echo "<td>";
						if($svinfo_row['website'] != "")
						{
							echo "&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"{$svinfo_row['website']}\"><img border=0 alt=www src=\"img/www.gif\"></a>";		
						}
						echo "</td>";
					break;
					default:
						echo "<td></td>"; /* Below servername, admin etc */
					break;
				}
			}
			/* Then image */
			if($pl_row > 0)
			{
				if($pl_row['ping'] == 0) /* Bots ping at 0ms */
					echo "<td>{$pl_row['name']}</td>";					
				else /* Real players ping at > 0ms and are marked in bold */
					echo "<td><b>{$pl_row['name']}</b></td>";
				echo "<td>score {$pl_row['score']}</td>";
				echo "<td>ping {$pl_row['ping']} ms</td>";
			}
			else
				echo "<td></td><td></td><td></td>";
			echo "</tr>\n";
			$count++;
		}
		/*
				if($svinfo_row['website'] != "")
		{
			echo "<a href=\"{$svinfo_row['website']}\"><img border=0 alt=www src=\"img/www.gif\"></a>";		
		}
		echo " {$svinfo_row['ip']} port {$svinfo_row['port']}";
		*/
		mysql_free_result($pl_result);
		echo "<tr><td colspan=3></td><tr>";
		echo "<tr><td colspan=3></td><tr>";
	} 

	mysql_free_result($sv_result);

	echo "</table>\n";
}

function GenerateLiveUserTable(&$control)
{

	$lastupdated = GetLastUpdated();
	$filename = GetFilename();

	/*  Section to display player list */
	$query  = "SELECT name, serverid, score, ping FROM playerlog WHERE time = '{$lastupdated}' AND ping != '0'";
	$pl_result = mysql_query($query);
	$numplayers = mysql_num_rows($pl_result);

	echo "<p class=\"cdbody\">{$numplayers} players online:<p>\n";
	echo "<p>\n<table id=cdtable>\n";
	echo "<tr><th>Name</th><th>Host</th><th>Score</th><th>Ping</th></tr>\n";

	while($pl_row = mysql_fetch_array($pl_result, MYSQL_ASSOC))
	{
		echo "<tr>";
		echo "<td><b>{$pl_row['name']}</b></td>";
		$query  = "SELECT hostname FROM servers WHERE serverid = '{$pl_row['serverid']}'";
		$svinfo_result = mysql_query($query);
		$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);

		echo "<td><a href=\"{$filename}?action=serverinfo&amp;id={$pl_row['serverid']}\">{$svinfo_row['hostname']}</a></td>";
		echo "<td>{$pl_row['score']}</td>";
		echo "<td>{$pl_row['ping']} ms</td>";
		echo "</tr>\n";
	}
	echo "</table>\n";
	mysql_free_result($pl_result);
}

function GenerateServerTable(&$control)
{
	$filename = GetFilename();
	
	if(!($control['numresults'] > 0 AND $control['numresults'] <= 50))
		$control['numresults'] = 10;

	if(!in_array($control['orderby'], array('uptime', 'playertime', 'maxplayers', 'popularity')))
		$control['orderby'] = 'playertime';

	if(!in_array($control['sort'], array('asc', 'desc')))
		$control['sort'] = 'desc';
		
	//echo "Time = ".$starttime." -> ".$endtime."<br>\n";
	$query = 'SELECT serverid , SUM( realplayers ) AS playertime , COUNT( serverid ) AS uptime , MAX( realplayers ) AS maxplayers , SUM( realplayers ) / COUNT( serverid ) as popularity '
	        . ' FROM serverlog '
	        . ' WHERE time > '.$control['starttime'].' AND time <= '.$control['endtime']
	        . ' GROUP BY serverid '
	        . ' ORDER BY '.$control['orderby'].' '.$control['sort'].' LIMIT 0, '.$control['numresults'];
					
	$svlog_result = mysql_query($query);
	
	echo "<table id=cdtable>\n";
	echo "<tr>";
	echo "<th>IP & port</th>";
	echo "<th>Hostname</th>";
	echo "<th>Admin</th>";

	Insert_Table_Sorter($control, $display = 'Uptime', $orderby = 'uptime'); 
	Insert_Table_Sorter($control, $display = 'Total player time', $orderby = 'playertime'); 
	Insert_Table_Sorter($control, $display = 'Most players at once', $orderby = 'maxplayers'); 
	Insert_Table_Sorter($control, $display = 'Popularity', $orderby = 'popularity'); 
	
	echo "</tr>\n";

	while($svlog_row = mysql_fetch_array($svlog_result, MYSQL_ASSOC))
	{
		$query  = "SELECT ip, port, hostname, admin, website FROM servers WHERE serverid = '{$svlog_row['serverid']}'";
		$sv_result = mysql_query($query);
		$sv_row = mysql_fetch_array($sv_result, MYSQL_ASSOC);

	    echo "<tr>";
		echo "<td>{$sv_row['ip']}<br>port {$sv_row['port']}</td>";
		echo "<td><a href=\"".$filename."?action=serverinfo&amp;id=".$svlog_row['serverid']."\">".$sv_row['hostname']."</a> ";
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
		echo "</tr>\n";
		mysql_free_result($sv_result);
	} 
	mysql_free_result($svlog_result);
	echo "</table>\n";
}

function GenerateMapTable(&$control)
{
	$filename = GetFilename();
	if(!($control['numresults'] > 0 AND $control['numresults'] <= 50))
		$control['numresults'] = 10;

	if(!in_array($control['orderby'], array('mapname', 'servedtime', 'playertime', 'maxplayers')))
		$control['orderby'] = 'playertime';
		
	if(!in_array($control['sort'], array('asc', 'desc')))
		$control['sort'] = 'desc';

	$query = 'SELECT mapname , SUM( realplayers ) as playertime , COUNT( realplayers ) as servedtime , MAX( realplayers ) AS maxplayers'
	        . ' FROM serverlog '
	        . ' WHERE time > '.$control['starttime'].' AND time <= '.$control['endtime']
	        . ' GROUP BY mapname '
	        . ' ORDER BY '. $control['orderby'] .' '.$control['sort'].' LIMIT 0, '.$control['numresults'];

	$svlog_result = mysql_query($query);

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
/*		<a href=\"{$filename}?action=mapinfo&amp;id={$svlog_row['mapname']}\">{$svlog_row['mapname']}</a> */
		echo "</td>";
		echo "<td>".mins_to_string($svlog_row['playertime'])."</td>";
		echo "<td>".mins_to_string($svlog_row['servedtime'])."</td>";
		echo "<td>{$svlog_row['maxplayers']}</td>";
		echo "</tr>\n";
	} 
	mysql_free_result($svlog_result);

	echo "</table>\n";
}

function GeneratePlayerTable(&$control)
{
	if(!($control['numresults'] > 0 AND $control['numresults'] <= 50))
		$control['numresults'] = 10;

	if(!in_array($control['orderby'], array('name', 'totalscore', 'playertime', 'fragrate')))
		$control['orderby'] = 'fragrate';
		
	if(!in_array($control['sort'], array('asc', 'desc')))
		$control['sort'] = 'desc';
		
	$query = 'SELECT name, SUM( score ) AS totalscore, COUNT(name) as playertime, SUM( score )/COUNT(name) as fragrate'
	        . ' FROM playerlog'
	        . ' WHERE time > '.$control['starttime'].' AND time <= '.$control['endtime']			
	        . ' AND ping >0 AND name != \'Player\''
	        . ' GROUP BY name'
	        . ' ORDER BY '.$control['orderby'].' '.$control['sort']
	        . ' LIMIT 0 , '.$control['numresults'];
	
	$pllog_result = mysql_query($query);

	echo "<table id=cdtable>\n";
	echo "<tr>";
	Insert_Table_Sorter($control, $display = 'Name', $orderby = 'name'); 
	Insert_Table_Sorter($control, $display = 'Total score', $orderby = 'totalscore'); 
	Insert_Table_Sorter($control, $display = 'Time played', $orderby = 'playertime'); 
	Insert_Table_Sorter($control, $display = 'Fragrate', $orderby = 'fragrate'); 
	echo "</tr>\n";

	while($pllog_row = mysql_fetch_array($pllog_result, MYSQL_ASSOC))
	{
	    echo "<tr>";
		echo "<td>{$pllog_row['name']}</td>";
		echo "<td>{$pllog_row['totalscore']}</td>";
		echo "<td>".mins_to_string($pllog_row['playertime'])."</td>";
		echo "<td>{$pllog_row['fragrate']}</td>";
		echo "</tr>\n";
	} 
	mysql_free_result($svlog_result);

	echo "</table>\n";
}

function Generate_HTML_Headers()
{
	global $CONFIG;
	echo "<html>\n";
	echo "<head>\n";
	echo "    <title>".$CONFIG['title']."</title>\n";
	echo "    <base href=\"".$CONFIG['baseurl']."browser/\">\n";
	echo "        <meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n";
	echo "        <meta name=\"keywords\" content=\"Code Red CodeRed Alien Arena 2006 2007 GE UE Uranium Edition Server Browser\">\n";
	echo "        <meta name=\"description\" content=\"Alien Arena Server Browser\">\n";
	echo "    <link rel=\"stylesheet\" href=\"".$CONFIG['stylesheet']."\">\n";
	echo "</head>\n";
	echo "<body>\n";
}

function Generate_HTML_Footers($lastupdated, $starttime)
{
	$endtime = explode( ' ', microtime() );
	$endtime = $endtime[1] + $endtime[0];

	/* Section to display when database was last updated */
	echo "<p class=\"cdfooter\">Database updated on ".date(DATE_RFC822, $lastupdated).". Page generated in ".round($endtime-$starttime, 3)." seconds.<br>\n";
	echo "&copy 2007 Tony Jackson - tonyj[at]cooldark[dot]com</p>\n";
	echo "</body></html>\n";
}

?>
