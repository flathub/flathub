<?php
/*********************************
This php script contacts the Alien Arena master server, 
gets a list of games servers, then queries them.
This information is stored in a mysql database.

In debug mode, all actions will be performed except the
database storage.

To invoke debug mode, use:
scanservers.php?debug
*********************************/
include '../config.php';  /* Database config */

define("VERSION", "1.0.0");

define("MAX_INSTANCES",5); /* Maximum number of instances of this script allowed to run at once */
define("MAX_SERVERS",256); /* Used to be hardcoded to 64! */
define("MASTER_QUERY","query"); /* Query string to sent to master server */
define("SERVER_QUERY","ÿÿÿÿstatus\n"); /* Query string to send to individual games servers */
define("MASTER_ADDRESS",'master.corservers.com');
define("MASTER_PORT",27900);
define("SERVER_RETRIES",3); /* Maximum number of times to try querying a games server */
define("SOCKET_TIMEOUT",6); /* socket_select() timeout, in seconds */

$debug = 0; /* Global for debug configuration */

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

/* Safety check - since this runs on a frequent cron job, we must check we don't have too many other instances running */
Function CheckInstances()
{
	echo "Checking for other instances...";
	
	$result = shell_exec('ps x | egrep \'scan\\.php|devscan\\.php\' | egrep -v grep');
	$result = explode("\n", $result); /* Last entry is always empty - must be other characters after the \n */
	$numinstances = count($result)-1; 

	if($numinstances > 1)
	{
		if($numinstances > MAX_INSTANCES)
			echo "error<br>\n{$numinstances} instances running, ".MAX_INSTANCES." allowed.<br>\nInstance list (including this one):<br>\n";
		else
			echo "OK<br>\n{$numinstances} instances running, ".MAX_INSTANCES." allowed.<br>\nInstance list (including this one):<br>\n";
		foreach($result as $instance)
		{
			echo "&nbsp;&nbsp;{$instance}<br>\n";
		}
	}
	else
		echo "OK<br>\n";

	if($numinstances > MAX_INSTANCES)
		die_message("Too many instances (mysql tables locked?)");
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
		
	echo "OK.<br>\n";

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
		echo "Sending query...";
		if(socket_write ($socket, MASTER_QUERY, strlen (MASTER_QUERY)) === FALSE) /* Returns bytes written or FALSE */
			die_socket($socket, "Failed in socket_write()");
		echo "OK.<br>\n";

		echo "Reading response...";  
		if(socket_recv($socket, $buffer, 12+6*MAX_SERVERS, 0) < 1) /* No documentation on return values - this seems to work */
		{
			if($debug)
				echo "Failure in socket_recv() (" . socket_strerror(socket_last_error($socket)).").<br>\n";

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
			echo "OK.<br>\n";
			break;
		}
		echo "no response.<br>\n";
	} while($try++ < SERVER_RETRIES);
	
	socket_close ($socket); /* No documentation on return values */

	if($try >= SERVER_RETRIES) /* If maximum retries reached */
		die_message("Maximum retries reached.");

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
			$result = socket_select($read, $write=NULL, $except=NULL, SOCKET_TIMEOUT, 0);
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
						if($debug)
							echo "Failure in socket_recv() " . socket_strerror(socket_last_error($socket)) .", ".$serverlist[$id]["ip"].":".$serverlist[$id]["port"].".<br>\n";

						if(socket_last_error($socket) == 111) /* 111 is connection refused - don't know if it has a name */
						{
							socket_close($socket);
							unset($socketlist[$id]); /* Strike from list of sockets to try again */
							unset($serverlist[$id]); /* Strike from list of servers */
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

						if($debug)
						{
							echo "Server data: ";
							print_r($server); echo "<br>\n";
						}
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
		if($debug)
			echo "Unresponsive: ".$serverlist[$id]["ip"].":".$serverlist[$id]["port"]." (Socket ".$socket.")<br>\n";
		unset($serverlist[$id]);  /* Remove server from list */
		socket_close($socket);
	}
}

Function PopulateServerEntry(&$server, $buffer)
{
	global $debug;
	if(strlen($buffer) == 0)
		return;

	$exploded = explode("\n", $buffer);

	if(array_key_exists(1, $exploded))
		$serverstring = explode("\\", $exploded[1]);
	else
		$serverstring = '';

	/* Case-sensitive parameters to parse result for */
	$parameters = array('hostname', 'mapname', 'version', 'website', 'Admin');
	foreach ($parameters as $string)
	{
		$key=array_search($string, $serverstring);
		if($key === FALSE)
			$server[strtolower($string)] = ''; /* Not found (but must add entry otherwise have to check existance later in mysql queries */
		else
			$server[strtolower($string)] = addslashes($serverstring[$key+1]);
	}
	
	/* Strip out ^n colour codes from server name */
	$server['hostname'] = preg_replace('/\^([0-9])/', '', $server['hostname']);
	/* Make sure map names are always lower case */
	$server['mapname'] = strtolower($server['mapname']);
	$server['playerinfo'] = array();

	$players = array_slice($exploded, 2, -1);

//	echo "PLAYERS:<br>\n";
//	print_r($players);

	foreach($players as $player)
	{
		/* $element here contains a space seperated string, including score, ping, name, and sometimes ip address
			name and ip address are surrounded by quotes */
		$space_delimited = explode(' ', $player);
		$quote_delimited = explode('"', $player);
		$player = array();  /* Convert type to array (within array of players) */
		$player['score'] = $space_delimited[0];
		$player['ping']  = $space_delimited[1];			
		$player['name']  = addslashes(trim($quote_delimited[1],' "')); /* Strip off trailing/leading whitespace and quotes, fix any escape characters */
    	$player['name'] = preg_replace('/\^([0-9])/', '', $player['name']); /* Strip out ^n colour codes */
		if(array_key_exists(3, $quote_delimited))
			$player['ip'] = addslashes(trim($quote_delimited[3],' "')); /* Strip off trailing/leading whitespace and quotes, fix any escape characters */
		else
			$player['ip'] = '';
		$server['playerinfo'][] = $player;
	}

//	echo "SERVER DATA:<BR>\n";
//	print_r($server);
}

/* Take $serverlist and put entries into mysql database */
Function UpdateDatabase($serverlist, $time)
{
	echo "<p>Connecting DB...";
	global $debug;
	global $CONFIG;

	$dbname = $CONFIG['dbName'];
	$dbhost = $CONFIG['dbHost'];
	$dbuser = $CONFIG['dbUser'];
	$dbpass = $CONFIG['dbPass'];
	$dbexpire = $CONFIG['dbExpire'];

	$conn = mysql_connect($dbhost, $dbuser, $dbpass) or die ('Unable to connect database: ' . mysql_error());
	mysql_select_db($dbname);

	// Temporary measure - clear tables and enter new value
	//$query = 'TRUNCATE TABLE `servers`';
	//$result = mysql_query($query);
	//$query = 'TRUNCATE TABLE `players`';
	//$result = mysql_query($query);

	echo "OK<br>\n";
	foreach ($serverlist as $id => $server)
	{
		$versionnum = floatval(array_shift(explode(' ', $server['version'])));
		if($versionnum >= 4) /* We only want to record servers version 3 and above */
		{
			/* First check to see if this server has been seen before */
			$query  = "SELECT serverid FROM servers WHERE ip = '{$server['ip']}' AND port = '{$server['port']}' AND hostname = '{$server['hostname']}' AND admin = '{$server['admin']}' AND version = '{$server['version']}' AND website = '{$server['website']}'";
			if($debug)
				echo $query."<br>\n";
			$result = mysql_query($query);
				
			if(mysql_num_rows($result) == 0)
			{ /* Not seen before, add to server list */
				$query = "INSERT INTO servers (ip, port, hostname, admin, version, website, lastseen) VALUES ('{$server['ip']}','{$server['port']}','{$server['hostname']}','{$server['admin']}','{$server['version']}','{$server['website']}','{$time}');";
				if($debug)
					echo $query."<br>\n";
				else
					$result = mysql_query($query);		
				$serverid = mysql_insert_id();
			}
			else
			{ /* We know about this one already */
				if($debug)
					echo "One we know about<br>\n";
				$row = mysql_fetch_array($result, MYSQL_ASSOC);
				$serverid = $row['serverid'];	
				mysql_free_result($result);

				/* Update the time this server was last seen (so we can purge old servers from the database) */
				$query = "UPDATE servers SET lastseen = '{$time}' WHERE serverid = '{$serverid}'";
				if($debug)
					echo $query."<br>\n";
				else		
					$result = mysql_query($query);

				
			}
			
		//	echo "<p>serverinfoid = ".$serverinfoid."</p>\n";
		
			/* Now we can insert our server entry */
			$query = "INSERT INTO serverlog (serverid, time, mapname) VALUES ('{$serverid}','{$time}','{$server['mapname']}');";
			if($debug)
				echo $query."<br>\n";
			else
				$result = mysql_query($query);
		
			/* Get ID from last inserted auto-increment field, ID */
			$lastid = mysql_insert_id();
			
			$realplayers = 0;
			
			foreach ($server["playerinfo"] as &$player)
			{
				$query = "INSERT INTO playerlog (serverlogid, serverid, time, name, score, ping, mapname) VALUES ('{$lastid}','{$serverid}','{$time}','{$player['name']}','{$player['score']}','{$player['ping']}','{$server['mapname']}');";
				if($debug)
					echo $query."<br>\n";
				else
					$result = mysql_query($query);
				if($player["ping"] > 0) /* Assume real player, rather than bot */
				{
					$realplayers++;
				}
			}
			$query = "UPDATE serverlog SET realplayers = '{$realplayers}' WHERE serverlogid = '{$lastid}'";
			if($debug)
				echo $query."<br>\n";
			else		
				$result = mysql_query($query);
		}
	}

	$query = "UPDATE stats SET lastupdated = '{$time}' WHERE id = '0'";
	if($debug)
		echo $query."<br>\n";
	else
		$result = mysql_query($query);

	/********* CLEAN OUT OLD ENTRIES ***********/

	if(!$debug)
	{
		echo "Deleting database entries where time < ".($time - $dbexpire).".<br>\n";
		$result = mysql_query("DELETE FROM serverlog WHERE time < ".($time - $dbexpire));
		$result = mysql_query("DELETE FROM playerlog WHERE time < ".($time - $dbexpire));
		$result = mysql_query("DELETE FROM servers WHERE lastseen < ".($time - $dbexpire));
		/* Remove all previous bot entries from DB */
		$result = mysql_query("DELETE FROM playerlog WHERE time <> '".$time."' AND ping = '0'");
		echo "Deleted ".mysql_num_rows($result)." old entries.<br>\n";
	}

	mysql_close($conn);
}

/* Main program starts here */
echo "<html>\n";
echo "<head>\n";
echo "	<title>Alien Arena Server Database Updater</title>\n";
echo "</head>\n";
echo "<body>\n";	
echo "<h1>Alien Arena Server Database Updater (".VERSION.")</h1><br>\n";

foreach ($_GET as $key => $value)
{
		switch ($key)
		{
			case 'debug':
				$debug = 1;
			break;

			default:
			break;
		}
}

if($debug)
{
	echo "<b>Debug enabled</b><br>\n";
	error_reporting (E_ALL);
}
else
{
	echo "<b>Live mode - for debug use scan.php?debug</b><br>\n";
	error_reporting (E_NONE);
}

/* Check that this script isn't running too many times already - hung on mysql access? */
CheckInstances();

$serverlist = QueryMasterServer();
/* $serverlist is our master array of servers, into which we will add information about each server */

/* Add local servers */
/*
$serverlist[] = array("ip" => "localhost", "port" => 27910);
$serverlist[] = array("ip" => "localhost", "port" => 27920);
$serverlist[] = array("ip" => "localhost", "port" => 27930);
$serverlist[] = array("ip" => "localhost", "port" => 27940);
$serverlist[] = array("ip" => "localhost", "port" => 27950);
$serverlist[] = array("ip" => "localhost", "port" => 27960);
$serverlist[] = array("ip" => "localhost", "port" => 27970);
$serverlist[] = array("ip" => "localhost", "port" => 27980);
$serverlist[] = array("ip" => "localhost", "port" => 27990);
$serverlist[] = array("ip" => "localhost", "port" => 27915);
$serverlist[] = array("ip" => "localhost", "port" => 27925);
$serverlist[] = array("ip" => "localhost", "port" => 27935);
$serverlist[] = array("ip" => "localhost", "port" => 27945);
$serverlist[] = array("ip" => "localhost", "port" => 27955);
$serverlist[] = array("ip" => "localhost", "port" => 27965);
$serverlist[] = array("ip" => "localhost", "port" => 27975);
$serverlist[] = array("ip" => "localhost", "port" => 27985);
$serverlist[] = array("ip" => "localhost", "port" => 27995);
*/

echo "Please wait - querying games servers...<br>\n";

$time = time();

QueryGamesServers($serverlist);

$i = count($serverlist);
echo "Successfully retrieved data from {$i} servers.<br>\n"; 

if($debug)
	echo "Database will NOT be updated, but will be queried (debug mode)<br>\n";
	
//echo "Database access disallowed\n</body></html>";
//return;	

UpdateDatabase($serverlist, $time);

echo "</body></html>\n";
?>
