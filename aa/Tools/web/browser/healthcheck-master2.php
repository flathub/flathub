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

/*********************************
This php script contacts the Alien Arena master server, 
gets a list of games servers, then queries them.

The script diagnoses any problems with the system.
*********************************/

define("VERSION", "Beta 0.2");

define("MAX_SERVERS",256); /* Used to be hardcoded to 64! */
define("MASTER_QUERY","query"); /* Query string to sent to master server */
define("SERVER_QUERY","ÿÿÿÿstatus\n"); /* Query string to send to individual games servers */
define("MASTER_ADDRESS",'master2.corservers.com');
define("MASTER_PORT",27900);
define("SERVER_RETRIES",3); /* Maximum number of times to try querying a games server */

/*********************
*  Support functions *
**********************/

Function hexstr($hexstr) {
  $hexstr = str_replace(' ', '', $hexstr);
  $retstr = pack('H*', $hexstr);
  return $retstr;
}

Function strhex($string) {
  $hexstr = unpack('H*', $string);
  return array_shift($hexstr);
}

Function die_message($string)
{
	die ($string."<br>\nAborting.<br>\n</body>\n</html>\n");
}

Function die_socket($socket, $string)
{
	$socketerr = socket_strerror(socket_last_error($socket));
	socket_close($socket);
	die_message($string." (".$socketerr.").");
}

Function QueryMasterServer()
{
	global $debug;
	$master_ip = gethostbyname(MASTER_ADDRESS);
	if($master_ip == MASTER_ADDRESS) /* Test for fail */
		die_message("Failed to get master $master_ip IP address.");

//	$master_ip = '111.111.111.111';  /* False address to test robustness */
//	$master_ip = '68.48.211.231'; /* FeelDBurn's master */
//	$master_ip= '0';

	$socket = socket_create (AF_INET, SOCK_DGRAM, SOL_UDP);
	if($socket === FALSE) /* Socket resource or FALSE */
		die_socket($socket, "Failed in socket_create()");

	echo "Connecting to master '".MASTER_ADDRESS."' ($master_ip) on port ".MASTER_PORT."...";
	if (socket_connect ($socket, $master_ip, MASTER_PORT) === FALSE) /* TRUE on success, FALSE on fail */
		die_socket($socket, "Failed in socket_connect()");
		
	if(socket_set_option(
	  $socket,
	  SOL_SOCKET,  // socket level
	  SO_RCVTIMEO, // timeout option
	  array(
	   "sec"=>5, // Timeout in seconds
	   "usec"=>0  // I assume timeout in microseconds
	   )
	  ) === FALSE) /* False on failure */
		die_socket($socket, "Failed in socket_set_option()");
	
	
	$try = 1;
	do
	{
		if(socket_write ($socket, MASTER_QUERY, strlen (MASTER_QUERY)) === FALSE) /* Returns bytes written or FALSE */
			die_socket($socket, "Failed in socket_write()");

		if(socket_recv($socket, $buffer, 12+6*MAX_SERVERS, 0) < 1) /* No documentation on return values - this seems to work */
		{

			if(socket_last_error($socket) == 111) /* 111 is connection refused - don't know if it has a name */
			{
				die_socket($socket, "master server refused connection");
			}
			else
			{
				/* Retry on any other kind of error */
				socket_clear_error($socket);
			}
		}
		else /* Success */
		{
			break;
		}
	} while($try++ < SERVER_RETRIES);
	
	socket_close ($socket); /* No documentation on return values */

	if($try >= SERVER_RETRIES) /* If maximum retries reached */
		die_message("timed out after three retries.");

	echo "OK<br>\n";
	/* Got this far, everything OK */
	$len = strlen($buffer);
	$numservers = ($len-12)/6;

	echo $numservers ." servers registered.<br>\n";

	/**********************************************
	    Buffer is 0xFFFF long in game (65536)
	    Data is this format:
	    Ignore first 12 bytes
	    Four byte address
	    Two byte port
	    Four byte address
	    Two byte port
	    ....
	    Up to MAX_SERVERS servers (hard coded limit in game) 
	************************************************/

	//echo $buffer."<br>\n";
	$index = 12;

	$serverlist = array();
	/* Generate array of arrays; ip addresses and port numbers */
	while ($index < $len)
	{
		$server=substr($buffer,$index,4);
		$port=substr($buffer,$index+4,2);
		$serverlist[] = array("ip" => inet_ntop($server), "port" => (("0x".strhex($port))+0));
		$index += 6;
	}
	return $serverlist;
}

Function QueryGamesServers(&$serverlist)
{
	global $debug;
	$error = "";
	$socketlist = array();  /* This will contain the list of successfully opened sockets */

	foreach ($serverlist as $id => &$server) /* & indicates pointer, not copy */
	{
			
		$buffer="";

		$socket = socket_create (AF_INET, SOCK_DGRAM, SOL_UDP);
		if($socket === FALSE)
		{
			$error = "Failed in socket_create() (" . socket_strerror(socket_last_error($result)) . ")";
			break;
		}
		
		if(socket_connect ($socket, $server["ip"], $server["port"]) === FALSE)
		{
			$error = "Failed in socket_connect() (" . socket_strerror(socket_last_error($result)) . ")";
			break;
		}
		
		if(socket_write ($socket, SERVER_QUERY, strlen (SERVER_QUERY)) === FALSE)
		{
		    $error = "Failed in socket_write() (" . socket_strerror(socket_last_error($result)) . ")";
			break;
		}
			
		/* No need to configure socket for timeout here - this is handled by the socket_select() function instead */
		if(	socket_set_nonblock($socket)===FALSE)
		{
			$error = "failed in socket_set_nonblock() (" . socket_strerror(socket_last_error($result)) . ")<br>\n";	
			break;
		}
		$socketlist[$id] = $socket;  // Add socket to list, using id from $serverlist to allow future cross reference
	//	echo $socket." => ".$server["ip"].":".$server["port"]."<br>\n";
		
	}

	if($error != "")
	{
		die_socket($socket, $error);
	}

	$try=1;
	do /* Retry loop */
	{
		do /* Wait for response loop */
		{
			if($debug)
				echo "<br>Waiting for replies...<br>";
			$buffer = "";
			$read = $socketlist; /* Take copy of list, as socket_select() writes back to it's params */
			$result = socket_select($read, $write=NULL, $except=NULL, 3, 0);
			if($result === FALSE)
				die_message("Failure in socket_select() (".socket_strerror(socket_last_error()).")"); 

			if($debug)
				echo "Got ".$result." socket event(s).<br>\n";
			
			if($result > 0)
			{
				foreach($read as $socket)
				{
					$id = array_search($socket, $socketlist); // Get id in $socketlist (matchs $serverlist entry)
					if(socket_recv($socket, $buffer, 1024, 0) < 1)  /* Receive is not error checked - timeouts are OK! */
					{
						$serverlist[$id]["status"] = socket_strerror(socket_last_error($socket)).".";

						if(socket_last_error($socket) == 111) /* 111 is connection refused - don't know if it has a name */
						{
							socket_close($socket);
							unset($socketlist[$id]); /* Strike from list of sockets to try again */
						}
						else
						{
							/* Retry on any other kind of error */
							socket_clear_error($socket);
						}
					}
					else
					{   /* Received OK */
						socket_close ($socket);
						unset($socketlist[$id]); /* Finished with this socket */
						$server = &$serverlist[$id];
						PopulateServerEntry($server, $buffer);
						$server["status"] = "OK.";
					}
				} /* End of for loop for each server */
			} /* End of if */
		} while ($result > 0); /* Repeat until no more responses come back */

		$unresponsive = count($socketlist);
		if($try < SERVER_RETRIES && $unresponsive > 0)
		{
			echo "Retrying ".$unresponsive." unresponsive servers...<br>\n";
			foreach($socketlist as $socket)
				if(socket_write ($socket, SERVER_QUERY, strlen (SERVER_QUERY)) === FALSE)
				{
				    $error = "Failed in socket_write() (" . socket_strerror(socket_last_error($result)) . ")";
					break;
				}
		}
	} while($try++ < SERVER_RETRIES && $unresponsive > 0);

	foreach ($socketlist as $id => $socket)
	{
		socket_close($socket);
	}
}

Function PopulateServerEntry(&$server, $buffer)
{
	global $debug;
	if(strlen($buffer) == 0)
		return;

	$exploded = explode("\\", $buffer);

//	print_r($exploded);
//	echo "<br>\n";

	/* Case-sensitive parameters to parse result for */
	$parameters = array("hostname", "mapname", "version", "website", "Admin");
	foreach ($parameters as $string)
	{
		$key=array_search($string, $exploded);
		if($key === FALSE)
			$server[strtolower($string)] = ""; /* Not found (but must add entry otherwise have to check existance later in mysql queries */
		else
			$server[strtolower($string)] = addslashes($exploded[$key+1]);
	}

	/* game is a special case */
	$key=array_search("game", $exploded);
	if(!($key === FALSE))
	{
		$temp = explode("\n", $exploded[$key+1]);
		/* $server["game"] = $temp[0]; */ /* Always seems to be "arena" */
		// Get rid of game value off start of array
		array_shift($temp);
		// Get rid of spurious CR and LF pair
		array_pop($temp);
		$server["playerinfo"] = $temp;
		foreach ($server["playerinfo"] as &$player)
		{
			/* $element here contains a space seperated string, including score, ping, name, and sometimes ip address
				name and ip address are surrounded by quotes */
			$space_delimited = explode(" ", $player);
			$quote_delimited = explode("\"", $player);
			$player = array();  /* Convert type to array (within array of players) */
			$player["score"] = $space_delimited[0];
			$player["ping"]  = $space_delimited[1];			
			$player["name"]  = addslashes(trim($quote_delimited[1]," \"")); /* Strip off trailing/leading whitespace and quotes, fix any escape charaters */
			if(array_key_exists(3, $quote_delimited))
				$player["ip"] = addslashes(trim($quote_delimited[3]," \"")); /* Strip off trailing/leading whitespace and quotes, fix any escape charaters */
			else
				$player["ip"] = "";
		}
	}
}

Function compare_servers($x, $y)
{
	$x_ip = inet_pton($x["ip"]);
	$y_ip = inet_pton($y["ip"]);
	if ( $x_ip == $y_ip )
	{
		if($x["port"] == $y["port"])
			return 0;
		else if ( $x["port"] < $y["port"] )
			return -1;
		else
			return 1;
	}
	else if ( $x_ip < $y_ip )
		return -1;
	else
		return 1;
}

error_reporting (E_NONE);

/* Main program starts here */
echo "<html>\n";
echo "<head>\n";
echo "	<title>Alien Arena Server Health Check</title>\n";
echo "</head>\n";
echo "<body>\n";	
echo "<h1>Alien Arena Server Health Check (".VERSION.")</h1><br>\n";

$serverlist = QueryMasterServer();
/* $serverlist is our master array of servers, into which we will add information about each server */

echo "Please wait - querying games servers...<br>\n";

QueryGamesServers($serverlist);

$okservers = array();
$failedservers = array();

foreach($serverlist as $server)
{
	if(!array_key_exists("status", $server))
		$server["status"] = "No reply after three attempts.";

	if($server["status"] == "OK.")
		$okservers[] = $server;
	else
		$failedservers[] = $server;
}

if(count($okservers) > 0)
{
	usort($okservers, 'compare_servers');
	echo "<hr>\n<u><b>Live servers</b></u><br><br>\n<table border=1>\n";
	echo "<tr><td>IP address</td><td>Port</td><td>Result</td></tr>\n";
	foreach($okservers as $server)
		echo "<tr><td>".$server["ip"]."</td><td>".$server["port"]."</td><td>".$server["status"]." Server version <b>".$server["version"]."</b></td></tr>\n";
	echo "</table>\n";
}

if(count($failedservers) > 0)
{
	usort($failedservers, 'compare_servers');
	echo "<hr>\n<b><u>Failed servers</b></u><br><br>\n<table border=1>\n";
	echo "<tr><td>IP address</td><td>Port</td><td>Result</td></tr>\n";
	foreach($failedservers as $server)
		echo "<tr><td>".$server["ip"]."</td><td>".$server["port"]."</td><td>".$server["status"]."</td></tr>\n";
	echo "</table>\n";
}

echo "</body></html>\n";
?>
