using System;
using System.Windows.Forms;

using System.Data.SqlClient;
using System.IO;

namespace Alien_Arena_Account_Server_Manager
{
    public class DBOperations
    {
        public static pProfile CheckPlayer(string Name)
        {
            pProfile Profile;

            Profile.Name = "Invalid";
            Profile.Location = "Invalid";
            Profile.Password = "Invalid";
            Profile.StatPoints = 0.0f;
            Profile.TotalFrags = 0.0f;
            Profile.TotalTime = 0.0f;
            Profile.Status = "Inactive";

            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlDataReader rdr = null;

                SqlCommand cmd = new SqlCommand("SELECT Name, Password, Points, TotalFrags, TotalTime, Location, Status FROM Players WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                rdr = cmd.ExecuteReader();
                while (rdr.Read())
                {
                    if (Name == rdr["Name"].ToString())
                    {
                        Profile.Name = rdr["Name"].ToString();
                        Profile.Location = rdr["Location"].ToString();
                        Profile.Password = rdr["Password"].ToString();
                        Profile.StatPoints = Convert.ToDouble(rdr["Points"]);
                        Profile.TotalFrags = Convert.ToDouble(rdr["TotalFrags"]);
                        Profile.TotalTime = Convert.ToDouble(rdr["TotalTime"]);
                        Profile.Status = rdr["Status"].ToString();
                    }
                }
                rdr.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            return Profile;
        }

        public static bool CheckIfBanned(string Location)
        {
            pProfile Profile;

            Profile.Name = "Invalid";
            Profile.Location = "Invalid";
            Profile.Password = "Invalid";
            Profile.StatPoints = 0.0f;
            Profile.TotalFrags = 0.0f;
            Profile.TotalTime = 0.0f;
            Profile.Status = "Inactive";

            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlDataReader rdr = null;

                SqlCommand cmd = new SqlCommand("SELECT Name, Password, Points, TotalFrags, TotalTime, Location, Status FROM Players WHERE Location = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Location));
                rdr = cmd.ExecuteReader();
                while (rdr.Read())
                {
                    if (Location == rdr["Location"].ToString())
                    {
                        if (rdr["Status"].ToString() == "Banned")
                            return true;
                    }
                }
                rdr.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            return false;
        }

        public static void AddProfile(string Name, string Password, string Location)
        {
            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("If NOT exists (select name from sysobjects where name = 'Players') CREATE TABLE Players(Name varchar(32), Password varchar(256), Points float, TotalFrags int, TotalTime int, Location varchar(32), Status varchar(16));", sqlConn);

                cmd.ExecuteNonQuery();

                cmd.CommandText = "if NOT exists (SELECT * FROM Players WHERE Name = @0) INSERT INTO Players(Name, Password, Points, TotalFrags, TotalTime, Location, Status) VALUES(@0, @1, 0.0, 0.0, 0, @2, @3)";
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", Password));
                cmd.Parameters.Add(new SqlParameter("2", Location));
                cmd.Parameters.Add(new SqlParameter("3", "Active"));
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void SetPlayerStatus(string Name, string Status)
        {
            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Status = @1 WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", Status));
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void ChangePlayerPassword(string Name, string NewPassword)
        {
            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Password = @1 WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", NewPassword));
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void UpdatePlayer(string Name, string Points, string TotalFrags, string TotalTime)
        {
            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Points = @1, TotalFrags = @2, TotalTime = @3 WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.Parameters.Add(new SqlParameter("1", Math.Round(Convert.ToDouble(Points), 3)));
                cmd.Parameters.Add(new SqlParameter("2", Convert.ToInt32(TotalFrags)));
                cmd.Parameters.Add(new SqlParameter("3", Convert.ToInt32(TotalTime)));
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void GenerateRankingsList()
        {
            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlDataReader rdr = null;

                SqlCommand cmd = new SqlCommand("SELECT * FROM Players ORDER BY Points DESC", sqlConn);

                Stats.allPlayers.Clear();

                rdr = cmd.ExecuteReader();
                while (rdr.Read())
                {
                    pProfile Profile;

                    Profile.Name = rdr["Name"].ToString();
                    Profile.Location = rdr["Location"].ToString();
                    Profile.Password = rdr["Password"].ToString();
                    Profile.StatPoints = Convert.ToDouble(rdr["Points"]);
                    Profile.TotalFrags = Convert.ToDouble(rdr["TotalFrags"]);
                    Profile.TotalTime = Convert.ToDouble(rdr["TotalTime"]);
                    Profile.Status = rdr["Status"].ToString();

                    Stats.allPlayers.Add(Profile);

                    //add all players marked active to our active player list.
                    if (Profile.Status == "Active")
                        accountServer.sServer.players.AddPlayer(Profile.Name);
                }
                rdr.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void GenerateStatsFile()
        {
            pProfile Profile;

            Profile.Name = "Invalid";
            Profile.Location = "Invalid";
            Profile.Password = "Invalid";
            Profile.StatPoints = 0.0f;
            Profile.TotalFrags = 0.0f;
            Profile.TotalTime = 0.0f;
            Profile.Status = "Inactive";

            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                int total = 0;
                SqlDataReader rdr = null;

                SqlCommand cmd = new SqlCommand("SELECT * FROM Players ORDER BY Points DESC", sqlConn);

                StreamWriter file = new System.IO.StreamWriter(@"playerrank.db");

                rdr = cmd.ExecuteReader();
                while (rdr.Read() && total < 1000)
                {
                    file.WriteLine(rdr["Name"].ToString());
                    file.WriteLine("127.0.0.1");
                    file.WriteLine(rdr["Points"].ToString());
                    //current frags
                    file.WriteLine("0");
                    //total fragrate
                    file.WriteLine(rdr["TotalFrags"].ToString());
                    //current time
                    file.WriteLine("0");
                    file.WriteLine(rdr["TotalTime"].ToString());
                    //next two not needed any longer(server ip and poll number)
                    file.WriteLine("0");
                    file.WriteLine("0");

                    total++;
                }
                rdr.Close();
                file.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void ClearAllStats()
        {
            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Points = 0.0, TotalFrags = 0, TotalTime = 0 WHERE Name = *", sqlConn);
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }

        public static void ResetPlayer(string Name)
        {
            SqlConnection sqlConn = new SqlConnection("Server=SASHA\\SQLEXPRESS; Database = AAPlayers; Trusted_Connection = true");

            try
            {
                sqlConn.Open();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                SqlCommand cmd = new SqlCommand("UPDATE Players SET Points = 0.0, TotalFrags = 0, TotalTime = 0, Status = 'Inactive' WHERE Name = @0", sqlConn);
                cmd.Parameters.Add(new SqlParameter("0", Name));
                cmd.ExecuteNonQuery();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }

            try
            {
                sqlConn.Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.ToString());
            }
        }       
    }
}
