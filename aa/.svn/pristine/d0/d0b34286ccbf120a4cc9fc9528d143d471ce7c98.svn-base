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
    public class masterServer
    {
        public static masterServer sServer = new masterServer();

        static UdpClient sListener;        
        public static bool runListener = false;
        private static int FrameTime = 0;

        static ServerList Servers = new ServerList();

        public masterServer()
        {
            //nothing to do yet.
        }

        public class ServerList
        {
            public List<string> Address;
            public List<string> Ip;
            public List<ushort> Port;
            public List<int> LastHeartbeat;

            public ServerList()
            {
                Address = new List<string>();
                Ip = new List<string>();
                Port = new List<ushort>();
                LastHeartbeat = new List<int>();
            }

            public void Add(string Ip, ushort Port, int LastHeartbeat)
            {
                Address.Add(Ip + ":" + Port.ToString());
                this.Ip.Add(Ip);
                this.Port.Add(Port);
                this.LastHeartbeat.Add(LastHeartbeat);
            }

            public void Drop(string Address)
            {
                int idx = this.Address.IndexOf(Address);

                Ip.RemoveAt(idx);
                Port.RemoveAt(idx);
                LastHeartbeat.RemoveAt(idx);
            }

            public void Clear()
            {
                Ip.RemoveAll(AllStrings);
                Port.RemoveAll(AllUShorts);
                LastHeartbeat.RemoveAll(AllInts);
            }

            private static bool AllStrings(String s)
            {
                return true;
            }

            private static bool AllUShorts(ushort i)
            {
                return true;
            }

            private static bool AllInts(int i)
            {
                return true;
            }
        }

        static void RunServerCheck()
        {
            //Heartbeats are sent by servers every 5 minutes.  If we don't receive a heartbeat, we ping the server for two minutes(once a minute).
            //If it doesn't respond after two minutes, shut it down.
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                //Frametime has looped, so in this case update times of servers to be current.
                if (Servers.LastHeartbeat[i] > FrameTime)
                {
                    Servers.LastHeartbeat[i] = FrameTime;
                }
                else if (FrameTime - Servers.LastHeartbeat[i] > 8)
                {
                    //Never received a response from the ping sent
                    ACCServer.sDialog.UpdateMasterStatus("Shutting down " + Servers.Address[i] + ".", ACCServer.sLevel.WARNING);
                    Servers.Drop(Servers.Address[i]);
                }
                else if (FrameTime - Servers.LastHeartbeat[i] > 5) 
                {
                    IPAddress ip = IPAddress.Parse(Servers.Ip[i]);

                    string message = "ÿÿÿÿping";

                    IPEndPoint dest = new IPEndPoint(ip, Servers.Port[i]);

                    byte[] send_buffer = Encoding.Default.GetBytes(message);

                    //Send to client
                    try
                    {
                        sListener.Send(send_buffer, send_buffer.Length, dest);
                        ACCServer.sDialog.UpdateMasterStatus("Sending ping to " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".", ACCServer.sLevel.UPDATE);
                    }
                    catch (Exception exc) { MessageBox.Show(exc.ToString()); }
                }
            }
        }

        static void SendServerListToClient(IPEndPoint dest)
        {
            //Check if this IP is coming from a banned player
            if (DBOperations.CheckIfBanned(dest.Address.ToString()))
                return;

            string message = "ÿÿÿÿservers ";

            byte[] send_buffer = Encoding.Default.GetBytes(message);
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                IPAddress ip = IPAddress.Parse(Servers.Ip[i]);

                Array.Resize(ref send_buffer, send_buffer.Length + ip.GetAddressBytes().Length);
                Array.Copy(ip.GetAddressBytes(), 0, send_buffer, send_buffer.Length - ip.GetAddressBytes().Length, ip.GetAddressBytes().Length);

                byte[] port = BitConverter.GetBytes(Servers.Port[i]);
                Array.Reverse(port);

                Array.Resize(ref send_buffer, send_buffer.Length + port.Length);
                Array.Copy(port, 0, send_buffer, send_buffer.Length - port.Length, port.Length);
            }

            //Send to client
            try
            {
                sListener.Send(send_buffer, send_buffer.Length, dest);
                ACCServer.sDialog.UpdateMasterStatus("Sending server list to " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".", ACCServer.sLevel.INFO);
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        static void HeartBeat(IPEndPoint dest)
        {
            ACCServer.sDialog.UpdateMasterStatus("Heartbeat from " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".", ACCServer.sLevel.INFO);

            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    //Matched, so update the hearbeat time of this server
                    Servers.LastHeartbeat[i] = FrameTime;

                    string message = "ÿÿÿÿack";

                    byte[] send_buffer = Encoding.Default.GetBytes(message);

                    //Send to client
                    try
                    {
                        ACCServer.sDialog.UpdateMasterStatus("Sending Ack to " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".", ACCServer.sLevel.INFO);
                        sListener.Send(send_buffer, send_buffer.Length, dest);
                        return;
                    }
                    catch (Exception exc) { MessageBox.Show(exc.ToString()); }
                }
            }

            AddServerToList(dest);
        }

        static void Ack(IPEndPoint dest)
        {
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    //Matched, so update the hearbeat time of this server
                    Servers.LastHeartbeat[i] = FrameTime;
                    ACCServer.sDialog.UpdateMasterStatus("Ack from " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".", ACCServer.sLevel.UPDATE);
                    break;
                }
            }
        }

        static void AddServerToList(IPEndPoint dest)
        {
            bool duplicate = false;

            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate)
            {
                Servers.Add(dest.Address.ToString(), Convert.ToUInt16(dest.Port), FrameTime);
                ACCServer.sDialog.UpdateMasterStatus("Added " + dest.Address.ToString() + ":" + dest.Port.ToString() + " to list!", ACCServer.sLevel.UPDATE);
            }
        }

        static void Shutdown(IPEndPoint dest)
        {
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                if (dest.Address.ToString() == Servers.Ip[i] && dest.Port == Servers.Port[i])
                {
                    ACCServer.sDialog.UpdateMasterStatus("Shutting down " + dest.Address.ToString() + ":" + dest.Port.ToString() + ".", ACCServer.sLevel.WARNING);
                    Servers.Drop(Servers.Address[i]);
                    break;
                }
            }
        }

        static void ParseData(string message, IPEndPoint source)
        {
            if (message.Contains("ÿÿÿÿgetservers") || message.Contains("query"))
            {                
                SendServerListToClient(source);
            }
            else if (message.Contains("ÿÿÿÿping"))
            {
                AddServerToList(source);
            }
            else if(message.Contains("ÿÿÿÿack"))
            {
                Ack(source);
            }
            else if(message.Contains("ÿÿÿÿheartbeat"))
            {
                HeartBeat(source);
            }
            else if(message.Contains("ÿÿÿÿshutdown"))
            {
                Shutdown(source);
            }
            else
               ACCServer.sDialog.UpdateMasterStatus("Unknown command from " + source.Address.ToString() + ":" + source.Port.ToString() + ".", ACCServer.sLevel.WARNING);
        }

        public void GetServerList()
        {
            for (int i = 0; i < Servers.Ip.Count; i++)
            {
                Stats.Servers.Add(Servers.Ip[i], Servers.Port[i], "Server", "Map");
            }
        }

        public void Listen()
        {
            sListener = new UdpClient(27900);
            sListener.Ttl = 100;
            int LastCheck = 0;

            IPEndPoint source = new IPEndPoint(0, 0);
            string received_data;
            byte[] receive_byte_array;
            try
            {
                while (runListener)
                {
                    FrameTime = DateTime.UtcNow.DayOfYear * 1440 + DateTime.UtcNow.Hour * 60 + DateTime.UtcNow.Minute;
                    if (FrameTime == 0)
                        LastCheck = 0;

                    receive_byte_array = sListener.Receive(ref source);
                    received_data = Encoding.Default.GetString(receive_byte_array, 0, receive_byte_array.Length);
                    if (received_data.Length > 1)
                        ParseData(received_data, source);
                    
                    if (FrameTime - LastCheck >= 1)
                    {
                        ACCServer.sDialog.UpdateMasterStatus("Server Check at: " + FrameTime.ToString(), ACCServer.sLevel.SYSTEM);
                        LastCheck = FrameTime;
                        RunServerCheck();
                    }
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }

            try
            {
                sListener.Close();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

    }
}