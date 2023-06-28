#!/usr/bin/env ruby


=begin

    ALIEN ARENA SERVER BROWSER
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
	
=end

require 'gtk2'
require 'libglade2'
require 'socket'
require 'rbconfig.rb'
include Config

$debug = false   # set to true/false to enable/disable debug output (don't use rubyw to launch!)
$offline = false # set to true/false to enable/disable offline debug mode (extra data files required)
$title = 'Alien Arena Server Browser'
$version = '0.4a'

=begin
	The ServerLink class allows simple access to Alien Arena master/games servers, without
	having to worry about the details of the UDP protocols involved
	
	Before any other calls, set_protocol(<type>) should be called with either 'master' or 'game'.
	
	query(<'ip:port' string array>) then queries each server in the array, returning an
	array containing a list of servers that replied.  query() is blocking, so may take a
	little while depending on how many servers are being queried.
	
	Once a query is complete, information can be read about each server individually,
	using one of:
	
	get_master_list('ip:port')  - in the case of having queried one or more master servers
		=> returns array of 'server:ip' strings for each game server registered
	get_server_info('ip:port')  - in the case of having queried one or more games servers
		=> Returns hash of server parameters
	get_player_info('ip:port')  - in the case of having queried one or more games servers
		=> Returns array of hashs (each hash containing one players worth of information)
		
	On the next query() call, any old server/player/master data is cleared and replaced
=end

class ServerLink
	def initialize
		set_protocol('master') # default to master mode (rather than leave uninitialised)
		# default - try three times for each server
		#@tries = 3 # TODO: currently not used
		@buffers = Hash.new # to store the reply UDP packet payload from each server
		@pings = Hash.new   # to store array of ping times for each server
	end
	
	# set type of link - master or game
	def set_protocol (type = 'master')
		if type == 'master'
			@protocol = 'master'
		elsif type == 'game'
			@protocol = 'game'
		end
	end
	
	# query all servers in array (can be multiple masters, as well as games servers)
	def query(servers)
		if servers == nil
			return []
		end

		# in offline mode, return data from a set of files rather than by querying live servers
		if $offline
			return offline_query(servers)
		end

		# select the query string to put in the outboand UDP packet
		if @protocol == 'master'
			query = "query"
		elsif @protocol == 'game'
			query = "ÿÿÿÿstatus\n"
		else
			return []
		end
		
		connections = Hash.new
		failed = Array.new
		@buffers.clear
		@pings.clear
		
		# Get the time before sending any packets, to time server response time (ping)
		starttime = Time.now
		
		# send a UDP packet to each server in the array
		servers.each do
			| server |  # Here server is of form 'ip:port' string
			begin
				socket = UDPSocket.new  # open works in the same way
				socket.connect(server.split(':')[0], server.split(':')[1])
				socket.send(query, 0)
				#socket.send(nil, 0)  # Test failure case
				connections[socket] = server # hash keyed on socket resource, containing server string
			rescue
				# some failure making a socket or sending a message - add to list of failed servers
				#socket.close - can't use this - may not even be open
				failed << server
				next
			end
		end
		
		# remove servers from list if socket failed to create/send UDP packet
		servers -= failed
		
		# check that we have at least one open UDP socket
		if connections.length == 0
			return servers
		end
		
		selectsocketlist = connections.keys  # get list of sockets from hash
		while result = select(selectsocketlist, nil, nil, 0.5) # select() waits for one or more socket to get a read event, or times out
			ping = (Time.now - starttime)*1000
			# store the time at which this server (or multiple servers) responded, and store the replies in our array of buffers
			result[0].each do
				|socket|
				begin
					# here connections[socket] gives us the 'ip:port' string of the associated server
					@buffers[connections[socket]] = socket.recv(2048) # big enough to cover both games server replies and master reply with 256 servers (payload 12+6*256 bytes)
					@pings[connections[socket]] = ping
					selectsocketlist.delete(socket) # delete from array  now that we have a reply
					socket.close
				rescue
					next
				end
				
				# test code to dump UDP payload contents to file for offline debug mode (make sure debug/ directory exists first)
				# file = open("debug/#{connections[socket].split(':')[0]}-#{connections[socket].split(':')[1]}.dmp", 'wb')
				# file.write(@buffers[connections[socket]])
				# file.close
			end
		end
		
		# here selectsocketlist will contain an array of failed sockets
		selectsocketlist.each do
			|socket|
			servers -= [connections[socket]]
			socket.close
		end
		
		# return array of servers that responded to our queries
		return servers
	end
	
	# offline mode query (for debug), that uses local files instead of querying live servers
	def offline_query(servers)
		replied = Array.new
		if $offline != true
			return
		end

		servers.each do
			|server|
			begin
				filename = server.split(':')[0]+'-'+server.split(':')[1]+'.dmp'
				fakeserver = File.open("debug/#{filename}", 'rb')
				@buffers[server] = fakeserver.read()
				fakeserver.close
				@pings[server] = 100
				replied << server
			rescue
			end
		end
		return replied
	end
	
	# parses UDP payload received from a master server and returns array of 'ip:port' strings
	def get_master_list server
		if @protocol != 'master'
			return []
		end
		####################################
		#	Buffer is 0xFFFF long in game (65536)
		#	Data is this format:
		#	Ignore first 12 bytes 'ÿÿÿÿservers '
		#	Four byte address
		#	Two byte port
		#	Four byte address
		#	Two byte port
		#	....
		#	Up to 256 servers (hard coded limit in game) 
		####################################
		
		servers = Array.new
		buff = @buffers[server]
		if buff == nil
			return servers
		end
		buff = buff[12..-1] # Chop off first 12 chars
		0.step(buff.length-6, 6) do
			|i|
			ip,port = buff.unpack("@#{i}Nn")  #  @#{i} denotes offset 'i' into buffer
			servers << inet_ntoa(ip).to_s + ':' + port.to_s
		end
		return servers # This may be an empty array
	end
	
	# return hash of server parameters from UDP packet data
	def get_server_info server
		if @protocol != 'game'
			return []
		end
		
		#example server strings
		#buffer = "ÿÿÿÿprint\n\\mapname\\ctf-killbox\\needpass\\0\\gamedate\\Jan 31 2007\\gamename\\data1\\maxspectators\\4\\Admin\\Forsaken\\website\\http://www.alienarena.info\\sv_joustmode\\0\\maxclients\\16\\protocol\\34\\cheats\\0\\timelimit\\0\\fraglimit\\10\\dmflags\\2641944\\deathmatch\\1\\version\\6.03 x86 Jan  7 2007 Win32 RELEASE\\hostname\\Alienarena.info - CTF\\gamedir\\arena\\game\\arena\n"
		#buffer = "ÿÿÿÿprint\n\\mapname\\DM-OMEGA\\needpass\\0\\maxspectators\\4\\gamedate\\Jan  9 2007\\gamename\\data1\\sv_joustmode\\0\\maxclients\\8\\protocol\\34\\cheats\\0\\timelimit\\0\\fraglimit\\10\\dmflags\\16\\version\\6.03 x86 Jan  7 2007 Win32 RELEASE\\hostname\\pufdogs hell\\gamedir\\arena\\game\\arena\n3 17 \"test chap\" \"loopback\"\n0 0 \"Cyborg\" \"127.0.0.1\"\n3 0 \"Squirtney\" \"127.0.0.1\"\n0 0 \"Butthead\" \"127.0.0.1\"\n"
		buffer = @buffers[server]
		buffer = buffer.split("\n")
		serverinfo = buffer[1].split('\\')[1..-1]
		if serverinfo.length % 2 == 0  # check even number of keys
			serverinfo = Hash[*serverinfo]
		else
			serverinfo = Hash.new # empty hash
		end
		
		serverinfo['numplayers'] = buffer[2..-1].length
		serverinfo['ping'] = @pings[server]

		return serverinfo
	end
	
	# returns array of hashs about each player on a server
	def get_player_info server
		if @protocol != 'game'
			return []
		end
		
		#example server strings
		#buffer = "ÿÿÿÿprint\n\\mapname\\ctf-killbox\\needpass\\0\\gamedate\\Jan 31 2007\\gamename\\data1\\maxspectators\\4\\Admin\\Forsaken\\website\\http://www.alienarena.info\\sv_joustmode\\0\\maxclients\\16\\protocol\\34\\cheats\\0\\timelimit\\0\\fraglimit\\10\\dmflags\\2641944\\deathmatch\\1\\version\\6.03 x86 Jan  7 2007 Win32 RELEASE\\hostname\\Alienarena.info - CTF\\gamedir\\arena\\game\\arena\n"
		#buffer = "ÿÿÿÿprint\n\\mapname\\DM-OMEGA\\needpass\\0\\maxspectators\\4\\gamedate\\Jan  9 2007\\gamename\\data1\\sv_joustmode\\0\\maxclients\\8\\protocol\\34\\cheats\\0\\timelimit\\0\\fraglimit\\10\\dmflags\\16\\version\\6.03 x86 Jan  7 2007 Win32 RELEASE\\hostname\\pufdogs hell\\gamedir\\arena\\game\\arena\n3 17 \"test chap\" \"loopback\"\n0 0 \"Cyborg\" \"127.0.0.1\"\n3 0 \"Squirtney\" \"127.0.0.1\"\n0 0 \"Butthead\" \"127.0.0.1\"\n"
		buffer = @buffers[server]
		buffer = buffer.split("\n")
		playerbuff = buffer[2..-1]
		
		playerinfo = Array.new # array of hashs
		playerbuff.each do
			| line |
			player = Hash.new
			# each line is of form   3 17 "test chap" "12.34.56.78"  (note spaces in names)
			space_delimited = line.split(' ')
			quote_delimited = line.split('"')
			player['score'] = space_delimited[0]
			player['ping'] = space_delimited[1]
			player['name'] = quote_delimited[1]
			if quote_delimited[3] != nil
				player['ip'] = quote_delimited[3]
			end
			playerinfo << player
		end
		
		return playerinfo
	end
		
	def inet_aton ip
		ip.split(/\./).map{|c| c.to_i}.pack("C*").unpack("N").first
	end
	
	def inet_ntoa n
		[n].pack("N").unpack("C*").join "."
	end
	
	# get raw UDP payload response from a particular server (debug only)
	def get_response(server)
		if @buffers.include?(server)
			return @buffers[server]
		else
			return nil
		end
	end
	
end

=begin
	This is the application class responsible for handling the GUI
	and launching games/URLS.  It uses the ServerLink class to query
	servers and get meaningful responses.
=end
class Browser
	def initialize
		if DetectWindows()
			require 'win32ole'
		end
		
		@config = Hash.new
		@saveconfig = false # This flag, if set to true, saves the contents of @config to file on application exit
		
		# load the configuration data from a file, if it exists (otherwise sets default values)
		LoadConfig()
		
		# Create our serverlink object for talking to master/games servers
		@serverlink = ServerLink.new
		
		# load the Glade XML interface and automatically connect handlers with this class
		@glade = GladeXML.new('browser.glade') { |handler|
			if respond_to?(handler)
				if $debug == true
					puts "Handler connected '#{handler}'"
				end
				# this is the important bit that connects the string to the method
				method(handler)
			else
				if $debug == true
					puts "No method defined to handle '#{handler}'"
				end
			end
		}
		
		# Load image file into banner
		@glade['imgTitle'].file = 'browser.png'
		
		# Set the title text (used elsewhere in the program too)
		@window = @glade['wndBrowser']
		@window.title = $title + ' - ' + $version
		
		# Build and connect the storage models for server/player GTK TreeViews
		AddServerModelToView()
		AddPlayerModelToView()
	end
	
	def onQuit
		SaveConfig()
		Gtk.main_quit
	end
		
	def AddServerModelToView
		#	 IP/port, Name, mapname, version, website, admin, players, ping 
		@serverlist = Gtk::ListStore.new(String, String, String, Integer, Integer)
		# Sort by increasing ping, default 
		@serverlist.set_sort_column_id(4, Gtk::SORT_ASCENDING)

		@glade['trvServers'].model = @serverlist

		cell_renderer = Gtk::CellRendererText.new
		
		cell_renderer_limited = Gtk::CellRendererText.new
		cell_renderer_limited.set_property('width', 100)
		
		col_ip      = Gtk::TreeViewColumn.new('IP:port', cell_renderer,         :text => 0)
		col_server  = Gtk::TreeViewColumn.new('Server',  cell_renderer_limited, :text => 1)
		col_map     = Gtk::TreeViewColumn.new('Map',     cell_renderer,         :text => 2)
		col_players = Gtk::TreeViewColumn.new('Players', cell_renderer,         :text => 3)
		col_ping    = Gtk::TreeViewColumn.new('Ping',    cell_renderer,         :text => 4)

		col_ip.     set_resizable(true)
		col_server. set_resizable(true)
		col_map.    set_resizable(true)
		col_players.set_resizable(true)
		col_ping.   set_resizable(true)

		col_ip.     set_sort_column_id(0)
		col_server. set_sort_column_id(1)
		col_map.    set_sort_column_id(2)
		col_players.set_sort_column_id(3)
		col_ping.   set_sort_column_id(4)
	
		@glade['trvServers'].append_column(col_server)
		@glade['trvServers'].append_column(col_ip)
		@glade['trvServers'].append_column(col_map)
		@glade['trvServers'].append_column(col_players)
		@glade['trvServers'].append_column(col_ping)

		#  We only want to select a single server at a time
		@glade['trvServers'].selection.mode = Gtk::SELECTION_SINGLE
		# Tie a function to the 'changed' signal 
		@glade['trvServers'].selection.signal_connect('changed') {
			|widget, event|
			if widget.selected != nil
				ShowPlayers(widget.selected[0])
			end
		}

	end

	def AddPlayerModelToView
		# Name, IP/port, players, ping
		@playerlist = Gtk::ListStore.new(String, Integer, Integer)
		@playerlist.set_sort_column_id(1, Gtk::SORT_DESCENDING)
		
		@glade['trvPlayers'].model = @playerlist
		@glade['trvPlayers'].selection.mode = Gtk::SELECTION_NONE

		cell_renderer = Gtk::CellRendererText.new
		
		col_name  = Gtk::TreeViewColumn.new('Name',  cell_renderer, :markup => 0) # bind the markup property to the column contents
		col_score = Gtk::TreeViewColumn.new('Score', cell_renderer, :text => 1)
		col_ping  = Gtk::TreeViewColumn.new('Ping',  cell_renderer, :text => 2)

		col_name.set_sort_column_id(0)
		col_score.set_sort_column_id(1)
		col_ping.set_sort_column_id(2)

		col_name.set_resizable(true)
		col_score.set_resizable(true)
		col_ping.set_resizable(true)
	
		@glade['trvPlayers'].append_column(col_name)
		@glade['trvPlayers'].append_column(col_score)
		@glade['trvPlayers'].append_column(col_ping)
	end
	
	# Test which OS we're running under
	def DetectWindows
		return ['mswin32', 'mingw32', 'cygwin'].include?(Config::CONFIG['host_os'].downcase)
	end
	
	def LaunchURL (url = "http://www.cooldark.com/aa/browser/stats.php")
		if DetectWindows()
			shell=WIN32OLE.new("WScript.Shell")
			shell.Run("cmd /c start \"\" \"#{url}\"", 0, false)
		else
			system("#{@config['browser']} \"#{url}\" &")
		end		
	end
	
	# Called when the user selects a different server in the server TreeView
	def ShowPlayers server
		@selectedserver = server
#		puts 'ShowPlayers for ' + server
		playerinfo = @serverlink.get_player_info(server)
		@playerlist.clear()
		
		# Fill out the storage model for the player TreeView
		playerinfo.each do
			|player|
			row = @playerlist.append()
			if player['ping'].to_i != 0
				row[0] = "<b>#{player['name']}</b>"
			else
				row[0] = player['name']
			end
			row[1] = player['score'].to_i
			row[2] = player['ping'].to_i
		end
		
		serverinfo = @serverlink.get_server_info(server)
		
		# Fill out the "Server Information" pane
		{	'lblAdmin'=>'Admin',
			'lblWebsite'=>'website',
			'lblFragLimit'=>'fraglimit',
			'lblTimeLimit'=>'timelimit',
			'lblMaxPlayers'=>'maxclients',
			'lblMaxSpectators'=>'maxspectators',
			'lblVersion'=>'version',
			'lblDmflags'=>'dmflags'}.each do
				|label, itemkey|
			@glade[label].text = serverinfo.include?(itemkey)?serverinfo[itemkey] : ''
		end
	end

	def onRefresh
		@serverlist.clear()
		@playerlist.clear()
#		masters = ['master.corservers.com:27900','www.cooldark.com:12345']
		masters = ['master.corservers.com:27900','master2.corservers.com:27900']
		@serverlink.set_protocol('master')
		responsive_masters = @serverlink.query(masters)
		if responsive_masters.length == 0
			@window.title = $title + ' - ' + 'no response from master(s)'
			return
		end

		@window.title = $title + ' - ' + 'reply from ' + responsive_masters.length.to_s + ' master(s)...'
		servers = Array.new
		responsive_masters.each do
			|master|
			servers += @serverlink.get_master_list(master)
		end
		servers = servers.uniq
		
		@serverlink.set_protocol('game')
		responsive_servers = @serverlink.query(servers)
		@window.title = $title + ' - ' + responsive_servers.length.to_s + ' servers online'
		
		# Fill out the storage model for the server TreeView
		responsive_servers.each do
			|server|
			serverinfo = @serverlink.get_server_info(server)
			row = @serverlist.append()
			row[0] = server
			row[1] = serverinfo['hostname']
			row[2] = serverinfo['mapname'].downcase
			row[3] = serverinfo['numplayers']
			row[4] = serverinfo['ping']
		end
	end
	
	def onJoin
		if @selectedserver == nil
			return
		end
		if $debug == true
			puts 'Joining server '+@selectedserver
		end

		if DetectWindows()
			shell=WIN32OLE.new("WScript.Shell")
			shell.Run("cmd /K cd \"#{@config['gamedir']}\" & #{@config['gameexe']} +set game arena +connect #{@selectedserver}", 0, FALSE)
		else
			# Note: exec() in Linux replaces the current running process (ruby no longer runs), so using system() instead
			system("cd #{@config['gamedir']};./#{@config['gameexe']} +set game arena +connect #{@selectedserver} &")
		end		
	end

	def onClickTitle
		LaunchURL()
	end
	
	def onClickWebsite evtbox, par1
		# Assume this method is always called with a label inside an eventbox
		url = evtbox.children[0].text
		if url != ''
			LaunchURL(url)
		end
	end
	
	# Load the @config hash form file, if exists, else set defaults
	def LoadConfig
		# Set up default parameters
		parameters = {'gamedir'=>'../../', 'browser'=>'firefox'}
		if DetectWindows()
			parameters['gameexe'] = 'crx'
		else
			parameters['gameexe'] = 'crx.sdl'
		end
		
		@config.clear # empty hash table
		file = open('browser.ini', 'r')
		file.each do
			|line|
			key = line.split('=')[0].chomp  # chomp removes any line feed present at end of line
			value = line.split('=')[1]
			if parameters.include?(key) and value != nil
				if value.chomp.length != 0
					@config[key]=value.chomp
				end
			end
		end
		file.close
		
		parameters.each do
			|key, value|
			if !@config.include?(key)
				@config[key] = value
				@saveconfig = true
			end
		end
	
		rescue SystemCallError
			$stderr.print "Unable to load config - #{$!}\n"
			$stderr.print "Using defaults\n"
			@config = parameters
			@saveconfig = true

	end
	
	# If required, save the contents of the @config hash to file
	def SaveConfig
		if(@saveconfig == false)
			return
		end
		
		file = open('browser.ini', 'w')
		@config.each do
			|key, value|
			file.puts key+'='+value
		end
		file.close
		
		rescue SystemCallError
			$stderr.print 'Unable to save config - ' + $!
	end
	
end

# Start the whole lot up here
Browser.new()
Gtk.main
