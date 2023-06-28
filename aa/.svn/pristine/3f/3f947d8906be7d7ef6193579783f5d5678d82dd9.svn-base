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

/* Global containing time when page was generated */
$_starttime = 0;

function Generate_HTML_Headers()
{
	global $CONFIG;
	global $_starttime;

	/*  Start time for page generation */
	$_starttime = explode( ' ', microtime() );
	$_starttime = $_starttime[1] + $_starttime[0];

	echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n";	
	echo "<html>\n";
	echo "<head>\n";
	echo "    <title>".$CONFIG['title']."</title>\n";
	echo "    <base href=\"".$CONFIG['baseurl']."browser/\">\n";
	echo "        <meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n";
	echo "        <meta name=\"keywords\" content=\"Code Red CodeRed Alien Arena 2006 2007 GE UE Uranium Edition Server Browser\">\n";
	echo "        <meta name=\"description\" content=\"Alien Arena server browser - live server, player and map statistics.\">\n";
	echo "    <link rel=\"stylesheet\" href=\"".$CONFIG['stylesheet']."\">\n";
	echo "</head>\n";
	echo "<body>\n";
}

function Generate_HTML_Footers()
{
	global $_starttime;
	$endtime = explode( ' ', microtime() );
	$endtime = $endtime[1] + $endtime[0];

	/* Section to display when database was last updated */
	echo "<p class=\"cdfooter\">Database updated on ".date(DATE_RFC822, GetLastUpdated()).". Page generated in ".round($endtime-$_starttime, 3)." seconds.<br>\n";
	echo "&copy 2007 Tony Jackson - tonyj[at]cooldark[dot]com</p>\n";
	echo "</body></html>\n";
}

/* Build control array and sanitise URL input */
function BuildControl()
{
/* Array of actions, allowed parameters, and defaults/limits/allowed values for these parameters */
$actionparams = array(	
	'liveservers' => array(),
	'liveplayers' => array(),
	'serverstats' => array(
		'orderby' => array('default'=>'playertime', 'uptime', 'maxplayers'),
		'sort' => array('default'=>'desc', 'asc'),
		'history' => array('default'=>24, 'min'=>1, 'max'=>24),
		'results' => array('default'=>20, 'min'=>1, 'max'=>50)
		),
	'playerstats' => array(
		'orderby' => array('name', 'totalscore', 'default'=>'playertime', 'fragrate'),
		'sort' => array('default'=>'desc', 'asc'),
		'history' => array('default'=>24, 'min'=>1, 'max'=>24),
		'results' => array('default'=>20, 'min'=>1, 'max'=>50)		
		),
	'mapstats' => array(
		'orderby' => array('mapname', 'servedtime', 'default'=>'playertime', 'maxplayers'),
		'sort' => array('default'=>'desc', 'asc'),
		'history' => array('default'=>24, 'min'=>1, 'max'=>24),
		'results' => array('default'=>20, 'min'=>1, 'max'=>50)				
		),
	'serverinfo' => array(
		'history' => array('default'=>24, 'min'=>1, 'max'=>24),
		'id'=>array('default'=>0, 'min'=>0)
		),
	'playerinfo' => array(
		'history' => array('default'=>24, 'min'=>1, 'max'=>24),
		'id'=>array() /* Anything accepted */
		),
	'mapinfo' => array(
		'history' => array('default'=>24, 'min'=>1, 'max'=>24),
		'id'=>array() /* Anything accepted */
		),
	'serversearch' => array(),
	'playersearch' => array(),
	'mapsearch' => array(),
	);

//	print_r($actioncontrol);
		
	/* Parse url for valid parameters */
	$control = array();

	if(array_key_exists('action', $_GET) === FALSE)
	{   /* action= not defined */
		$control['action'] = array_shift(array_keys($actionparams)); /* Use first action in array */
	}
	elseif(array_key_exists($_GET['action'], $actionparams) === FALSE)
	{	/* action=<unknown value> */
		$control['action'] = array_shift(array_keys($actionparams)); /* Use first action in array */
	}
	else
	{	/* action=<valid action> - ready to go! */
		$control['action'] = $_GET['action'];
		$paramlist = $actionparams[$control['action']];
		foreach($paramlist as $param=>$check)
		{
			if(array_key_exists($param, $_GET))
			{	/* URL specifies this parameter */
				if(count($check) == 0) /* Anything accepted */
				{
					$control[$param] = $_GET[$param];
				}
				else
				{
					if(is_int($check['default']))
					{
						$value = intval($_GET[$param]);
						if(array_key_exists('min', $check))
							if($value < $check['min'])
								$value = $check['min'];

						if(array_key_exists('max', $check))
							if($value > $check['max'])
								$value = $check['max'];
						
						$control[$param] = $value;
					}
					elseif(is_string($check['default']))
					{  
						if(array_search($_GET[$param], $check) === FALSE)
							$control[$param] = $check['default'];
						else
							$control[$param] = $_GET[$param];
					}
				}
			}
			else
			{	/* param not given */
				if(array_key_exists('default', $check))
					$control[$param] = $check['default'];
			}
		}
	}
// print_r($control);
  return $control;
}

/* Trims long strings, returning a string on length $len ending in ... */
function LimitString($string, $len)
{
    $len = intval($len);
    
    if(strlen($string) > $len)
	{
      if($len>3)
		$string = substr($string, 0, $len-3)."...";
      else
        $string = "...";
    }
	
	return $string;
    
}

function GenerateInfoLink($type = "player", $name)
{
	$filename = GetFilename();
	$string  = "<a href=\"{$filename}?action={$type}info&amp;id={$name}\">{$name}</a>";
	return $string;
}

function GenerateMapLink($name)
{
	$filename = GetFilename();
	$string  = "<a href=\"{$filename}?action=mapinfo&amp;id={$name}\">{$name}</a>";
	return $string;
}

function GetServerNameFromID($id = 0)
{
	$filename = GetFilename();
	$query  = "SELECT ip, port, hostname FROM servers WHERE serverid = '{$id}'";
	$svinfo_result = mysql_query($query);
	$svinfo_row = mysql_fetch_array($svinfo_result, MYSQL_ASSOC);
	
	$string = "<a href=\"{$filename}?action=serverinfo&amp;id={$id}\">";
	if($svinfo_row['hostname'] == "")
		$string = $string."(unnamed: {$svinfo_row['ip']})</a>";
	elseif($svinfo_row['hostname'] == "noname")
		$string = $string."noname ({$svinfo_row['ip']})</a>";
	else
		$string = $string.LimitString($svinfo_row['hostname'],40)."</a>";
		
	return $string;
}


function GenerateSearchInput($searchtype="serversearch", $description="Search")
{
		$filename = GetFilename();
		echo "<form action=\"".$filename."?action=".$searchtype."\" method=\"post\">";
		echo "<p class=cdsubtitle>{$description}<br><input name=\"searchstring\" type=\"text\">"; 
		echo "<input type=\"submit\"></p>";
		echo "</form>";
}

function GetFilename()
{
	$filename = explode('/', $_SERVER["REQUEST_URI"]);
	$filename = explode('?', $filename[count($filename)-1]);
	//print_r($filename);
	$filename = $filename[0];
	if($filename == '')
		$filename = 'index.php';  /* For when just directory is accessed */
	return $filename;
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

// $display is the string to show in the header, $orderby is the sort order
// uses a copy of $control in order not modify original array
function Insert_Table_Sorter($control, $display, $orderby)
{
	echo "<th>".$display."<br>";
	$control['orderby'] = $orderby;
	$control['sort'] = 'desc';
	echo "<a href=\"".Generate_URL($control)."\"><img border=0 alt=up src=\"img/up.gif\"></a>";
	$control['sort'] = 'asc';
	echo "<a href=\"".Generate_URL($control)."\"><img border=0 alt=down src=\"img/down.gif\"></a>";
	echo "</th>";
}

function MinutesToString ($mins, $long=false)
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
	if(0)
	{	// With leading zeros
		$t['hours'] = (intval($hours) < 10) ? "0" . $hours : $hours;
		$t['mins'] = (intval($mins) < 10) ? "0" . $mins : $mins;
	}
	else
	{	// Without leading zeros
	    $t['hours'] = $hours;
		$t['mins'] = $mins;
	}
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

function GenerateNumResultsSelector($control)
{
	$current_numresults = $control['results'];
	echo "<p class=\"cdbody\">Results: ";
	for ($i=10; $i <=50; $i+=10)
	{
		$control['results'] = $i;
		if($i == $current_numresults)
		{
			echo $i.' ';
		}
		else
		{
			echo "<a href=\"".Generate_URL($control)."\">".$i."</a> ";
		}
	}
	echo "</p>\n";
}

function CheckDBLive()
{
	global $CONFIG;
	$lastupdated = GetLastUpdated();
	$now = time();
	if(($now - $lastupdated) > 60*60)
	{
		echo "<p class=cdsubtitle>Error: Database out-of-date!<br>(Serious network/database problems)</p>\n";
		echo "<p class=cdbody>Run a <a href=healthcheck.php>healthcheck</a>, and please contact {$CONFIG['contact']} with the results.</p>\n";
	}
	else if(($now - $lastupdated) > 15*60)
		echo "<p class=cdsubtitle>Warning: Database more than 15 minutes out-of-date<br>(Temporary network/database problems?)</p>\n";
	else if(($now - $lastupdated) > 3*60)
		echo "<p class=cdsubtitle>Notice: Database slightly out-of-date<br>(Network glitch?)</p>\n";
}

/* W3C complient generation of URL for linking */
function Generate_URL($control)
{
	$url = GetFileName().'?'.http_build_query($control);
	$url = str_replace('&','&amp;', $url);
//	echo $url;
	return $url;
}

?>
