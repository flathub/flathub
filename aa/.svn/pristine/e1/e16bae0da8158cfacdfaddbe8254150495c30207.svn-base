<?
# Many thanks to:
#
#   phptutorial.info for making this functionality possible
#     http://www.phptutorial.info/iptocountry/the_script.html
#
#   software77.net for the data
#
#   Mase (from supercrab.com) for the flags

include("ip_files/countries.php");

function GetCountryCode($ip)
{
    $numbers = preg_split( "/\./", $ip);
    include("ip_files/".$numbers[0].".php");
    $code=($numbers[0] * 16777216) + ($numbers[1] * 65536) + ($numbers[2] * 256) + ($numbers[3]);
    foreach($ranges as $key => $value){
        if($key<=$code){
            if($ranges[$key][0]>=$code){$two_letter_country_code=$ranges[$key][1];break;}
            }
    }
    if ($two_letter_country_code==""){$two_letter_country_code="unknown";}
    return $two_letter_country_code;
}

function ShowCountryFlag($two_letter_country_code)
{
	global $countries;

	$country_name=$countries[$two_letter_country_code][1];

	$flagfile="flags/$two_letter_country_code.gif";

	if (file_exists($flagfile))
    	echo "<img class=countryflag src=\"$flagfile\" alt=\"$country_name\" width=21 height=15>";
    else
		echo "<img class=countryflag src=\"flags/noflag.gif\" alt=\"$country_name\" width=21 height=15>";
}

function GetCountryName($two_letter_country_code)
{
	global $countries;
	return $countries[$two_letter_country_code][1];
}
?>

