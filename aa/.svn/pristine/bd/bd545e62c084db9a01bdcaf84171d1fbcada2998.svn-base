using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

using System.Net.Sockets;
using System.Net;
using System.Threading;
using System.Text;

namespace Alien_Arena_Account_Server_Manager
{
    public struct pProfile
    {
        public string Name;
        public string Location;
        public string Password;
        public double StatPoints;
        public double TotalFrags;
        public double TotalTime;
        public string Status;
    }

    public class rankingList
    {
        public List<pProfile> player;

        public rankingList()
        {
            player = new List<pProfile>();
        }

        public void Add(pProfile Player)
        {
            player.Add(Player);
        }

        public void Clear()
        {
            player.RemoveAll(AllProfiles);
        }
        private static bool AllProfiles(pProfile i)
        {
            return true;
        }
    }

    public class playerList
    {
        public List<string> name;
        public List<int> score;
        public List<int> frags;
        public List<int> fragsThisPoll;
        public List<int> hour;
        public List<int> minutes;

        public playerList()
        {
            name = new List<string>();
            score = new List<int>();
            frags = new List<int>();
            fragsThisPoll = new List<int>();
            hour = new List<int>();
            minutes = new List<int>();
        }

        public void DropPlayer(string Name)
        {
            int idx = name.IndexOf(Name);

            name.RemoveAt(idx);
            score.RemoveAt(idx);
            frags.RemoveAt(idx);
            fragsThisPoll.RemoveAt(idx);
            hour.RemoveAt(idx);
            minutes.RemoveAt(idx);

            //we've removed a player, update the listview
            ACCServer.sDialog.UpdatePlayerList();
        }

        //this called only when a packet of "login" is received, and the player is validated
        public void AddPlayer(string Name)
        {
            if (name.Exists(name => name == Name))
                return;

            name.Add(Name);
            score.Add(0); //no score needed for this list
            frags.Add(0);
            fragsThisPoll.Add(0);

            //timestamp
            hour.Add(DateTime.UtcNow.Hour);
            minutes.Add(DateTime.UtcNow.Minute);

            //we've added a player, update the listview
            ACCServer.sDialog.UpdatePlayerList();

            //set db field to "active"
            DBOperations.SetPlayerStatus(Name, "Active");
        }

        //logout player
        public void RemovePlayer(string Name)
        {
            if (Name.Length < 1)
                return;

            if (name.Exists(name => name == Name))
                DropPlayer(Name);

            //set db field to "inactive"
            DBOperations.SetPlayerStatus(Name, "Inactive");
        }

        public void AddPlayerInfo(string Name, int Score, int Frags, int Hour, int Minutes)
        {
            //we need to actually use insert and put these in a sorted order by fragrates
            int insertPos = 0;

            //check if this player is already in the list
            int idx = name.IndexOf(Name);

            if (name.Exists(name => name == Name))
                return;

            for(idx = 0; idx < name.Count; idx++)
            {
                if ((float)Frags / ((float)Hour * 60.0f + (float)Minutes) >= (float)frags[idx] / ((float)hour[idx] * 60.0f + (float)minutes[idx]))
                    insertPos = idx;
            }

            name.Insert(insertPos, Name);
            score.Insert(insertPos, Score);
            frags.Insert(insertPos, Frags);
            fragsThisPoll.Insert(insertPos, Frags); //can just use frags for initial value
            hour.Insert(insertPos, Hour);
            minutes.Insert(insertPos, Minutes);
        }       

        public int GetPlayerIndex(string Name)
        {
            for (int idx = 0; idx < name.Count; idx++)
            {
                if (Name == name[idx])
                {
                    return idx;
                }
            }

            return -1;
        }

        //clear the list completely
        public void Clear()
        {
            name.RemoveAll(AllStrings);
            score.RemoveAll(AllInts);
            frags.RemoveAll(AllInts);
            hour.RemoveAll(AllInts);
            minutes.RemoveAll(AllInts);
        }

        private static bool AllStrings(String s)
        {
            return true;
        }

        private static bool AllInts(int i)
        {
            return true;
        }
    }    

    public class accountServer
    {
        public static accountServer sServer = new accountServer();
        static UdpClient sListener;
        static Thread RunStats;
        static Thread UploadStats;
        static Thread RunListener;
        static Thread RunMaster;
        static bool runListener = false;

        public playerList players = new playerList();

        public accountServer()
        {
            //nothing to do yet.
        }

        static string ObtainVStringForPlayer(string Name)
        {
            string vString = "";

            Random rnd = new Random();

            //create randomstring
            for (int i = 0; i < 32; i++)
            {
                vString += Convert.ToChar(rnd.Next(0, 78) + 30);
            }
           
            return vString;
        }

        static bool ValidatePlayer(string Name, string Password, string Location)
        {
            //look for existing account in DB
            pProfile Profile = DBOperations.CheckPlayer(Name);

            //if no profile, create one and return true
            if (Profile.Name == "Invalid")
            {
                //add to database
                ACCServer.sDialog.UpdateStatus("Adding " + Name + " to database.", ACCServer.sLevel.UPDATE);
                DBOperations.AddProfile(Name, Password, Location);
                return true;
            }
            else
            {                
                if (Password == Profile.Password)
                {
                    //matched!
                    ACCServer.sDialog.UpdateStatus("Found " + Name + " in database.", ACCServer.sLevel.INFO);
                    return true;
                }
                else
                {
                    ACCServer.sDialog.UpdateStatus("Mismatched password for " + Name + " .", ACCServer.sLevel.WARNING);
                    return false;
                }
            }
        }

        static void SendValidationToClient(IPEndPoint dest)
        {
            string message = "ÿÿÿÿvalidated";

            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sListener.Send(send_buffer, send_buffer.Length, dest);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }


        static void SendVStringToClient(string Name, IPEndPoint dest)
        {
            string message = "ÿÿÿÿvstring ";

            message += ObtainVStringForPlayer(Name);
            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sListener.Send(send_buffer, send_buffer.Length, dest);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        static void ParseData(string message, IPEndPoint source)
        {
            if (message.Contains("ÿÿÿÿrequestvstring"))
            {
                //It's a valid request string
                string[] sParams = message.Split('\\');

                //Check protocol
                if (sParams[2] != "1")
                {                    
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] +  " from " + source.Address.ToString() + ".", ACCServer.sLevel.WARNING);
                    return;
                }

                //Send verification string to client.
                ACCServer.sDialog.UpdateStatus("Sending verification string to " + sParams[4] + " at " + source.Address.ToString() + ":" + source.Port.ToString() + ".", ACCServer.sLevel.INFO);
                SendVStringToClient(sParams[4], source);
            }

            else if (message.Contains("ÿÿÿÿlogin"))
            {
                //Process a login request

                //Check protocol
                string[] sParams = message.Split('\\');

                if (sParams[2] != "1")
                {
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] + " from " + source.Address.ToString() + ".", ACCServer.sLevel.WARNING);
                    return;
                }

                if(ValidatePlayer(sParams[4], sParams[6], source.Address.ToString()))
                {
                    ACCServer.sDialog.UpdateStatus("Adding " + sParams[4] + " to active player list.", ACCServer.sLevel.UPDATE);
                    sServer.players.AddPlayer(sParams[4]);
                    SendValidationToClient(source);
                }
            }

            else if (message.Contains("ÿÿÿÿlogout"))
            {
                //Process a logout request

                //Check protocol
                string[] sParams = message.Split('\\');

                if (sParams[2] != "1")
                {
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] + " from " + source.Address.ToString() + ".", ACCServer.sLevel.WARNING);
                    return;
                }

                if (ValidatePlayer(sParams[4], sParams[6], source.Address.ToString()))
                {
                    ACCServer.sDialog.UpdateStatus("Removing " + sParams[4] + " from active player list.", ACCServer.sLevel.UPDATE);
                    sServer.players.RemovePlayer(sParams[4]);
                }

            }

            else if (message.Contains("ÿÿÿÿchangepw"))
            {
                //Process a password change request

                //Check protocol
                string[] sParams = message.Split('\\');

                if (sParams[2] != "1")
                {
                    ACCServer.sDialog.UpdateStatus("Wrong protocol " + sParams[2] + " from " + source.Address.ToString() + ".", ACCServer.sLevel.WARNING);
                    return;
                }

                if (sParams[6] == "password") //Setting from a new system for an existing player
                {
                    if (ValidatePlayer(sParams[4], sParams[8], source.Address.ToString()))
                    {
                        ACCServer.sDialog.UpdateStatus("Setting password for " + sParams[4] + " .", ACCServer.sLevel.UPDATE);
                        DBOperations.ChangePlayerPassword(sParams[4], sParams[8]);
                        SendValidationToClient(source);
                    }
                }
                else 
                {
                    if (ValidatePlayer(sParams[4], sParams[6], source.Address.ToString()))
                    {
                        ACCServer.sDialog.UpdateStatus("Changing password for " + sParams[4] + " .", ACCServer.sLevel.UPDATE);
                        DBOperations.ChangePlayerPassword(sParams[4], sParams[8]);
                        SendValidationToClient(source);
                    }
                }
            }            
            else
            {
                //Unknown request
                ACCServer.sDialog.UpdateStatus("Unknown request! " + message, ACCServer.sLevel.WARNING);
            }
        }
               
        public void RequestServerList()
        {
            IPEndPoint Master = new IPEndPoint(IPAddress.Parse("69.243.97.80"), 27900);
            //master 2 149.210.138.19
            //master 1 69.243.97.80

            Socket sending_socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            
            string message = "query";

            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sending_socket.SendTo(send_buffer, Master);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            byte[] bytes = new byte[1024];
            try
            {
                sending_socket.Receive(bytes);

                int start = 12;
                int result = bytes.Length - 12;
                while (result > 0)
                {
                    //read 32 bit IP address (network byte order)
                    byte[] ip = bytes.Skip(start).Take(4).ToArray();
                    IPAddress sIP = new IPAddress(ip);

                    if (sIP.ToString() == "0.0.0.0")
                        break;

                    start += 4;

                    byte[] port = bytes.Skip(start).Take(2).ToArray();
                    Array.Reverse(port);
                    ushort sPort = BitConverter.ToUInt16(port, 0);
                    start += 2;

                    result -= 6; //6 bytes per server entry

                    //Add to list
                    Stats.Servers.Add(sIP.ToString(), sPort, "Server", "Map");
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            //Close this socket
            try
            {
                sending_socket.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void GetServerInfo(string Ip, ushort Port, int sNum)
        {
            IPEndPoint Server = new IPEndPoint(IPAddress.Parse(Ip), Port);

            Socket sending_socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            sending_socket.Ttl = 60;

            string message = "\xFF\xFF\xFF\xFFstatus\n";

            byte[] send_buffer = Encoding.Default.GetBytes(message);

            //Send to client
            try
            {
                sending_socket.SendTo(send_buffer, Server);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            byte[] bytes = new byte[1024];
            try
            {
                sending_socket.ReceiveTimeout = 3;
                sending_socket.Receive(bytes);

                message = Encoding.Default.GetString(bytes, 0, bytes.Length);

                string[] sParams = message.Split('\\');

                for (int i = 0; i < sParams.Length; i++)
                {
                    //after "mods" comes the large space delimited piece with player info
                    if(i != 0)
                    {
                        if (sParams[i - 1] == "hostname")
                            Stats.Servers.Name[sNum] = sParams[i];
                        if(sParams[i - 1] == "mapname")
                            Stats.Servers.Map[sNum] = sParams[i];

                        if (sParams[i - 1] == "mods" && sParams[i].Length > 0)
                        {
                            if (sParams[i].Contains("ctf"))
                            {
                                Stats.Servers.Status[sNum] = "Ctf";
                                Stats.CTFServers++;
                            }
                            if (sParams[i].Contains("g_tactical"))
                            {
                                Stats.Servers.Status[sNum] = "Tactical";
                                Stats.TACServers++;
                            }
                            else
                                Stats.DMServers++;

                            string[] sPlayers = sParams[i].Split('\n');
                            for(int j = 0; j < sPlayers.Length; j++)
                            {
                                if (sPlayers[j].Length > 0 && j > 0)
                                {
                                    string[] sPlayer = sPlayers[j].Split(' ');
                                    if (sPlayer.Length > 2)
                                    {
                                        string Name = sPlayer[2].Trim('"');
                                        //Get index of this player in active, signed in player list
                                        Stats.TotalPlayers++;
                                        //If this player is not logged in, forget them
                                        int idx = players.GetPlayerIndex(Name);
                                        if (idx != -1)
                                        {
                                            int Score = Convert.ToInt32(sPlayer[0]);
                                            int Hours = DateTime.UtcNow.Hour - players.hour[idx];
                                            int Minutes = DateTime.UtcNow.Minute - players.minutes[idx];
                                            if (Hours < 0)
                                            {
                                                Hours += 24;
                                                Minutes = 60 - Minutes;
                                            }
                                            int Frags = players.frags[idx] + (Score - players.score[idx] > 0? Score - players.score[idx]:0);
                                            players.fragsThisPoll[idx] = Score - players.score[idx];
                                            //check score vs what's already in the registered list to get frags to add to total
                                            Stats.players.AddPlayerInfo(Name, Score, Frags, Hours, Minutes);
                                            Stats.playersIngame.AddPlayerInfo(Name, Score, Frags, Hours, Minutes);
                                            //Then update the registered list with the updated score and frag total.
                                            players.frags[idx] = Frags;
                                            players.score[idx] = Score;                                         
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception exc) {  }

            //Close this socket
            try
            {
                sending_socket.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void Listen()
        {
            sListener = new UdpClient(27902);
            sListener.Ttl = 100;
            IPEndPoint source = new IPEndPoint(0, 0);
            string received_data;
            byte[] receive_byte_array;
            try
            {
                while (runListener)
                {
                    receive_byte_array = sListener.Receive(ref source);
                    received_data = Encoding.Default.GetString(receive_byte_array, 0, receive_byte_array.Length);
                    if (received_data.Length > 1)
                        ParseData(received_data, source);
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            try
            {
                sListener.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void Start_Server()
        {
            ACCServer.sDialog.UpdateStatus("Listening on port 27902...", ACCServer.sLevel.SYSTEM);

            ACCServer.sDialog.UpdateMasterStatus("Listening on port 27900...", ACCServer.sLevel.SYSTEM);

            //Start a thread for the account listener.
            runListener = true;
            RunListener = new Thread(new ThreadStart(Listen));
            RunListener.Start();

            //Start a thread for the master listener.
            masterServer.runListener = true;
            RunMaster = new Thread(new ThreadStart(masterServer.sServer.Listen));
            RunMaster.Start();

            //Start new thread for stats collection.
            Stats.getStats = true;
            RunStats = new Thread(new ThreadStart(Stats.StatsGen));
            RunStats.Start();

            //Start new thread for stats upload.
            Stats.uploadStats = true;
            UploadStats = new Thread(new ThreadStart(Stats.UploadStats));
            UploadStats.Start();
        }

        public void Stop_Server()
        {
            try
            {
                runListener = false;
                masterServer.runListener = false;
                Stats.getStats = false;
                Stats.uploadStats = false;                       
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
    }
}