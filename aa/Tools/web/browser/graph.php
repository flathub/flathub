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

include('../config.php'); /* Database config, username, password */
include('support.php'); /* For GetLastUpdated() */
error_reporting (E_ALL);

/* Given a value, finds highest next match from sequence 5,10,20,50,100,200,500,1000,2000,5000 etc */
function GetFriendlyMax($num)
{
	if($num < 1)
		return 1;
	$exponent = floor(log($num,10));
	$mantissa = $num / pow(10, $exponent);

	if($mantissa <= 1)
		$mantissa = 1;
	elseif($mantissa <= 2)
		$mantissa = 2;
	elseif($mantissa <= 5)
		$mantissa = 5;
	else
		$mantissa = 10;
		
	return ($mantissa * pow(10, $exponent));
}

//foreach(array(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,22,45,5533,25235,2555,1900) as $n)
//	echo $n."=>".GetFriendlyMax($n)."<br>";


/* Case-sensitive parameters to parse result for */
$parameters = array('show','history','id');
/* Parse url for valid parameters */
$control = array();
foreach ($_GET as $key => $value)
{
	if(!(array_search($key, $parameters) === FALSE))
		$control[$key] = $value;
}

/* Set defaults */
if(!array_key_exists('show', $control))
	$control['show'] = 'players';
if(!array_key_exists('history', $control))
	$control['history'] = 24;
if(!array_key_exists('id', $control))
	$control['id'] = 0;

/* Bounds check duration */
if($control['history'] < 1)
	$control['history'] = 1;

if($control['history'] > 24)
	$control['history'] = 24;
	
$control['id'] = intval($control['id']);  /* Just in case someone tries mysql injection with a string */
if($control['id'] < 0)
	$control['id'] = 0;
	
$conn = mysql_connect($CONFIG['dbHost'], $CONFIG['dbUser'], $CONFIG['dbPass']) or die ('Cannot connect to the database because: ' . mysql_error());
mysql_select_db($CONFIG['dbName']) or die ('Database not found on host');

/* Find time of last database update */
$endtime = GetLastUpdated();
$starttime = $endtime - $control['history']*60*60;

switch($control['show'])
{
	case 'players':
		$query  = "SELECT time, SUM(realplayers) AS value FROM serverlog WHERE time >= {$starttime} AND time <= {$endtime} GROUP BY time";
		$ydesc = "Players";
	break;
	case 'servers':
		$query  = "SELECT time, COUNT(serverid) AS value FROM serverlog WHERE time >= {$starttime} AND time <= {$endtime} GROUP BY time";
		$ydesc = "Servers";
	break;
	case 'server':
		$query  = "SELECT time, realplayers AS value FROM serverlog WHERE serverid = {$control['id']} AND time >= {$starttime} AND time <= {$endtime} GROUP BY time";
		$ydesc = "Players";
	break;
	default:
		$query  = "SELECT time, SUM(realplayers) AS value FROM serverlog WHERE time >= {$starttime} AND time <= {$endtime} GROUP BY time";
		$ydesc = "Players";
	break;
}	

$result = mysql_query($query);

$numresults = mysql_num_rows($result);

//echo "Got {$numresults} results.<br>\n";

$data = array();
while($row = mysql_fetch_array($result, MYSQL_ASSOC))
{
	$data[$row['time']] = $row['value'];
}
mysql_free_result($result);
mysql_close($conn);

//echo "Last value is ".array_pop($data)."<br>\n";

if($CONFIG['graphwidth'] > 100)
	$width=$CONFIG['graphwidth'];
else
	$width = 100;

if($CONFIG['graphheight'] > 50)
	$height=$CONFIG['graphheight'];
else
	$height=50;
	
// Create image and define colors
$image=imagecreate($width, $height);

// First imagecolorallocate() defines default fill for the image
$colour['white']=imagecolorallocate($image, 255, 255, 255);
$colour['green']=imagecolorallocate($image, 0, 255, 0);
$colour['darkgreen']=imagecolorallocate($image, 0, 200, 0);
$colour['blue']=imagecolorallocate($image, 0, 0, 255);
$colour['red']=imagecolorallocate($image, 255, 0, 0);
$colour['darkgrey']=imagecolorallocate($image, 50, 50, 50);
$colour['lightgrey']=imagecolorallocate($image, 245, 245, 245);
$colour['grey']=imagecolorallocate($image, 235, 235, 235);
$colour['lightred']=imagecolorallocate($image, 255, 140, 140);

$xorigin = 50;
$yorigin = $height - 15;
$gwidth = $width - $xorigin - 10;
$gheight = $yorigin - 5;

/* Now draw basic structure */
imagealphablending($image, false);
/* Filled rectangles must define upper-left then lower-right, otherwise
    won't work with PHP5-CGI implementation of gd lib */
imagefilledrectangle($image, 0, 0, $width, $height, $colour['grey']);
imagefilledrectangle($image, $xorigin, $yorigin-$gheight, $xorigin+$gwidth, $yorigin, $colour['white']);
/* Add x markers */
for($x = 0; $x <= $control['history']; $x++)
{
	$xposition = $xorigin+floor($x*($gwidth/$control['history']));
	/* Line across graph */
	imageline($image, $xposition, ($yorigin-$gheight)+1, $xposition, $yorigin-1, $colour['grey']);
	/* Ticks at bottom */
	imageline($image, $xposition, $yorigin+1, $xposition, $yorigin+3, $colour['darkgrey']);
}

// Find max values in each array
if($numresults > 0)
{
	$maxvalue = max($data);
	$avgvalue = round(array_sum($data)/count($data), 0);
}
else
{
	$maxvalue = 0;
	$avgvalue = 0;
}

/* Set "minimum allowed maximum" for the y axis */
if($maxvalue >= 10)
{
	/* Calculate $ystep - the value by which the y-axis lable increments */
	$ystep = GetFriendlyMax((14*$maxvalue)/$gheight);  /* 14 here is roughly the minimum allowed pixel seperation for markers on the y-axis */
	
	/* If maximum value is approaching an upper y-marker, increase the y-axis so that that marker is seen */
	$nearestmarker = round($maxvalue/$ystep)*$ystep;

	if($maxvalue < $nearestmarker)
		$ymax = $nearestmarker;
	else
		$ymax = $maxvalue;
}
else
	$ymax = 10;
	
$xpersec = $gwidth / ($endtime-$starttime);  // TODO - check for div 0
$yperplayer = $gheight / $ymax;
//echo $xpersec."<br>\n";

/* Calculate $ystep - the value by which the y-axis lable increments */
$ystep = GetFriendlyMax((14*$ymax)/$gheight);  /* 14 here is the minimum allowed pixel seperation for markers on the y-axis */
/* Add y-axis ticks now we know the scale */
if($ystep > 0)
	for($y = 0; $y <= $ymax; $y+=$ystep)
	{
		$yposition = $yorigin-floor($y*($gheight/$ymax));
		/* Line across graph */
		imageline($image, $xorigin+1, $yposition, $xorigin+$gwidth-1, $yposition, $colour['lightgrey']);	
		/* Tag at bottom */
		imageline($image, $xorigin-3, $yposition, $xorigin-1, $yposition, $colour['darkgrey']);
		$string = strval($y);
		imagestring ($image, 1, $xorigin-5-(strlen($string)*imagefontwidth(1)), $yposition-4, $string, $colour['darkgrey']);
	}

/* x-axis lables */
imagestring ($image, 1, $xorigin-10, $yorigin+5, "-{$control['history']}H", $colour['darkgrey']);
imagestring ($image, 1, $xorigin+$gwidth-5, $yorigin+5, "0H", $colour['darkgrey']);

/* y-axis lables - have to use strlen and imagefontwidth to right justify againsst the axis */
/*
$string = strval($ymax);
imagestring ($image, 1, $xorigin-5-(strlen($string)*imagefontwidth(1)), 3, $string, $colour['darkgrey']);
$string = strval(0);
imagestring ($image, 1, $xorigin-5-(strlen($string)*imagefontwidth(1)), $yorigin-4, $string, $colour['darkgrey']);
*/
/* Title text down left hand side */
imagestringup ( $image, 3 /* built in font*/, 2, $yorigin, $ydesc, $colour['blue']);
imagestringup ( $image, 1 /* built in font*/, 16, $yorigin, "Peak {$maxvalue}", $colour['red']);
imagestringup ( $image, 1 /* built in font*/, 16, $yorigin-45, "Avg {$avgvalue}", $colour['darkgreen']);

/* Create border around image */
imagerectangle( $image, 0, 0, $width-1, $height-1, $colour['darkgrey']);

/* Create border around graph */
imagerectangle( $image, $xorigin, $yorigin, $xorigin+$gwidth, $yorigin-$gheight, $colour['darkgrey']);

/* Test for debug - chop out some values in the middle */
/*$x = 0;
foreach($data as $time => $count)
{
	if($x >= 500 && $x < 507)
		unset($data[$time]);
	$x++;
}
*/


/* Mark average on graph */
if($avgvalue > 0)
	imageline($image, $xorigin+1, $yorigin-floor($avgvalue * $yperplayer), $xorigin+$gwidth-1, $yorigin-floor($avgvalue * $yperplayer), $colour['green']);

$lastmax = -1;
/* Mark maximum values on graph */
if($maxvalue > 2)
	foreach($data as $time => $count)
	{	
		if($count == $maxvalue)
		{
			$x = floor(($time-$starttime) * $xpersec);
			if($lastmax >= 0)
			{	/* May span several pixels if zoomed in, so draw rectangles rather than individual lines */
				imagefilledrectangle($image, $xorigin+$lastmax, $yorigin-1, $xorigin+$x, ($yorigin-$gheight)+1, $colour['red']);				
			}
			else
			{
				imageline($image, $xorigin+$x, $yorigin-1, $xorigin+$x, ($yorigin-$gheight)+1, $colour['red']);
			}
			$lastmax = $x; /* Record position of last maximum */
		}
		else
		{
			$lastmax = -1;
		}
	}

/* Finally, draw graph */
$lastx = -1000;  /* Setting this to -1000 prevents the first value being plotted */
$lasty = 0;
foreach($data as $time => $count)
{
	$x = floor(($time-$starttime) * $xpersec);
	$y = floor($count * $yperplayer);
	
	$maxgap = floor($xpersec*60+1);
	if($maxgap < 2)
		$maxgap = 2;
	
	if(($x-$lastx) <= $maxgap)  /* Don't draw a line if there are gaps in the data */
	{
		imageline($image, $xorigin+$x, $yorigin-$y, $xorigin+$lastx, $yorigin-$lasty, $colour['blue']);
	}
	else
	{
		/* Draw some blocks where there are gaps in the data */
		if($lastx >= 0) /* Previous data */
			imagefilledrectangle($image, $xorigin+$lastx+1, $yorigin-1, $xorigin+$x, ($yorigin-$gheight)+1, $colour['lightgrey']);				
		else /* No previous data */
			if($x > 0)
				imagefilledrectangle($image, $xorigin+1, $yorigin-1, $xorigin+$x, ($yorigin-$gheight)+1, $colour['lightgrey']);							
	
	}
	$lastx = $x;
	$lasty = $y;
	
}

/* Check to see if input data ended before we reached the end of the graph */
$x = $gwidth-1;
if(($lastx >= 0) and (($x-$lastx) > $maxgap))
{
	/* Fill rest of graph with grey block */
	imagefilledrectangle($image, $xorigin+$lastx+1, $yorigin-1, $xorigin+$x, ($yorigin-$gheight)+1, $colour['lightgrey']);				
}

if($numresults == 0)
	imagestring ($image, 5, $xorigin+10, $yorigin-20, "NO DATA", $colour['blue']);


// Output graph and clear image from memory
header("Content-type: image/png");
imagepng($image);
imagedestroy($image);

?>

