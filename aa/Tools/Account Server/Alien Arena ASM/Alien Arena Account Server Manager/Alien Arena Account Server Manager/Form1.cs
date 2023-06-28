using System;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace Alien_Arena_Account_Server_Manager
{
    public partial class ACCServer : Form
    {
        public static ACCServer sDialog;
        private static string SelectedPlayerName = "Invalid";
        public enum sLevel
        {
            INFO = 0,
            WARNING = 1,
            UPDATE = 2,
            SYSTEM = 3
        }

        public ACCServer()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            accountServer.sServer.Start_Server();
        }
        
        private void StopServer_Click(object sender, EventArgs e)
        {
            accountServer.sServer.Stop_Server();
            PlayerList.Items.Clear();
            accountServer.sServer.players.Clear();
            UpdateStatus("stopped...", sLevel.WARNING);
        }

        public delegate void UpdateStatusDelegate(string message, sLevel level);

        public void UpdateStatusList(string message, sLevel level)
        {
            try
            {
                ListViewItem pItem = null;

                //good idea to clear the list after say 1000 entries.
                if (StatusList.Items.Count > 1000)
                    StatusList.Items.Clear();

                pItem = StatusList.Items.Add(message);
                pItem.Selected = false;
                pItem.Focused = false;
                pItem.ForeColor = SetColor(level);

                pItem.EnsureVisible();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void UpdateStatus(string message, sLevel level)
        {
            if (this.InvokeRequired == false)
            {
                UpdateStatusList(message, level);
            }
            else
            {
                UpdateStatusDelegate updateStatus = new UpdateStatusDelegate(UpdateStatusList);
                this.Invoke(updateStatus, new object[] { message, level });
            }

        }

        public delegate void UpdateVPlayerListDelegate();

        public void UpdateVPlayerList()
        {
            try
            {
                PlayerList.Items.Clear();

                for (int i = 0; i < accountServer.sServer.players.name.Count; i++)
                {
                    ListViewItem pItem = null;
                    pItem = PlayerList.Items.Add(accountServer.sServer.players.name[i]);
                    pItem.Selected = false;
                    pItem.Focused = true;
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void UpdatePlayerList()
        {
            if(this.InvokeRequired == false)
            {
                UpdateVPlayerList();
            }
            else
            {
                UpdateVPlayerListDelegate updatePlayerList = new UpdateVPlayerListDelegate(UpdateVPlayerList);
                this.Invoke(updatePlayerList, new object[] {});
            }
        }

        private System.Drawing.Color SetColor(sLevel level)
        {
            switch (level)
            {
                default:
                case sLevel.INFO:
                    return System.Drawing.Color.Ivory;
                 case sLevel.WARNING:
                    return System.Drawing.Color.Red;
                case sLevel.UPDATE:
                    return System.Drawing.Color.Green;
                case sLevel.SYSTEM:
                    return System.Drawing.Color.Aqua;
            }
        }

        public delegate void UpdateMasterStatusDelegate(string message, sLevel level);

        public void UpdateMasterStatusList(string message, sLevel level)
        {
            try
            {
                ListViewItem pItem = null;

                //good idea to clear the list after say 1000 entries.
                if (MasterList.Items.Count > 1000)
                    MasterList.Items.Clear();

                pItem = MasterList.Items.Add(message);
                pItem.Selected = false;
                pItem.Focused = false;
                pItem.ForeColor = SetColor(level);
               
                pItem.EnsureVisible();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }

        public void UpdateMasterStatus(string message, sLevel level)
        {
            if (this.InvokeRequired == false)
            {
                UpdateMasterStatusList(message, level);
            }
            else
            {
                UpdateMasterStatusDelegate updateMasterStatus = new UpdateMasterStatusDelegate(UpdateMasterStatusList);
                this.Invoke(updateMasterStatus, new object[] { message, level });
            }

        }

        public delegate void UpdateSChartDelegate();

        public void UpdateSChart()
        {
            try
            {
                this.servercount.Series.Clear();

                Series series = this.servercount.Series.Add("Servers");

                series.YAxisType = AxisType.Primary;

                for (int i = Stats.ServerCount.Count - 1; i >= 0; i--)
                {
                    series.Points.Add(Stats.ServerCount[i]);
                }

                this.servercount.ChartAreas[0].RecalculateAxesScale();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateServerChart()
        {
            if (this.InvokeRequired == false)
            {
                UpdateSChart();
            }
            else
            {
                UpdateSChartDelegate updateSChart = new UpdateSChartDelegate(UpdateSChart);
                this.Invoke(updateSChart, new object[] {});
            }
        }

        public delegate void UpdatePChartDelegate();

        public void UpdatePChart()
        {
            try
            {
                this.playercount.Series.Clear();

                Series series = this.playercount.Series.Add("All Clients");

                for (int i = Stats.PlayerCount.Count - 1; i >= 0; i--)
                {
                    series.Points.Add(Stats.PlayerCount[i]);
                }

                Series vseries = this.playercount.Series.Add("Validated");

                for (int i = Stats.ValidatedPlayerCount.Count - 1; i >= 0; i--)
                {
                    vseries.Points.Add(Stats.ValidatedPlayerCount[i]);
                }

                this.playercount.ChartAreas[0].RecalculateAxesScale();
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdatePlayerChart()
        {
            if (this.InvokeRequired == false)
            {
                UpdatePChart();
            }
            else
            {
                UpdatePChartDelegate updatePChart = new UpdatePChartDelegate(UpdatePChart);
                this.Invoke(updatePChart, new object[] { });
            }
        }

        public delegate void UpdateSTypeChartDelegate();

        public void UpdateSTypeChart()
        {
            try
            {
                this.ServerTypes.Series.Clear();

                Series series = this.ServerTypes.Series.Add("Servers");

                series.ChartType = SeriesChartType.Pie;
                series.Points.Add(Stats.DMServers);                
                series.Points.Add(Stats.CTFServers);
                series.Points.Add(Stats.TACServers);

                series.Points[0].AxisLabel = "Deathmatch";
                series.Points[1].AxisLabel = "CTF";
                series.Points[2].AxisLabel = "Tactical";
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateServerTypeChart()
        {
            if (this.InvokeRequired == false)
            {
                UpdateSTypeChart();
            }
            else
            {
                UpdateSTypeChartDelegate updateSTypeChart = new UpdateSTypeChartDelegate(UpdateSTypeChart);
                this.Invoke(updateSTypeChart, new object[] { });
            }
        }

        public delegate void UpdateRankingsDelegate();

        public void UpdateRankings()
        {
            try
            {
                this.Rankings.Items.Clear();
                DBOperations.GenerateRankingsList();
                for (int i = 0; i < Stats.allPlayers.player.Count; i++)
                {
                    ListViewItem pItem = null;
                    pItem = Rankings.Items.Add((i+1).ToString());
                    pItem.Selected = false;
                    pItem.Focused = true;

                    pItem.SubItems.Add(Stats.allPlayers.player[i].Name);
                    pItem.SubItems.Add(Stats.allPlayers.player[i].StatPoints.ToString());
                    pItem.SubItems.Add(Stats.allPlayers.player[i].Status);
                }
                
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateRankingList()
        {
            if (this.InvokeRequired == false)
            {
                UpdateRankings();
            }
            else
            {
                UpdateRankingsDelegate updateRankings = new UpdateRankingsDelegate(UpdateRankings);
                this.Invoke(updateRankings, new object[] { });
            }
        }

        public delegate void UpdateServersDelegate();

        public void UpdateServers()
        {
            try
            {
                this.ServerList.Items.Clear();

                for (int i = 0; i < Stats.Servers.Name.Count; i++)
                {
                    ListViewItem pItem = null;
                    pItem = ServerList.Items.Add(Stats.Servers.Name[i]);
                    pItem.Selected = false;
                    pItem.Focused = true;

                    pItem.SubItems.Add(Stats.Servers.Ip[i] + ":" + Stats.Servers.Port[i]);
                }
            }
            catch (Exception exc) { MessageBox.Show(exc.ToString()); }
        }
        public void UpdateServerList()
        {
            if (this.InvokeRequired == false)
            {
                UpdateServers();
            }
            else
            {
                UpdateServersDelegate updateServers = new UpdateServersDelegate(UpdateServers);
                this.Invoke(updateServers, new object[] { });
            }
        }

        private void ClearStats_Click(object sender, EventArgs e)
        {
            DBOperations.ClearAllStats();
            ACCServer.sDialog.UpdateRankingList();
        }

        private void SetActive_Click(object sender, EventArgs e)
        {
            accountServer.sServer.players.AddPlayer(SelectedPlayerName);
            ACCServer.sDialog.UpdateRankingList();
        }

        private void SetInactive_Click(object sender, EventArgs e)
        {
            accountServer.sServer.players.RemovePlayer(SelectedPlayerName);
            ACCServer.sDialog.UpdateRankingList();
        }

        private void ResetPlayer_Click(object sender, EventArgs e)
        {
            DBOperations.ResetPlayer(SelectedPlayerName);
            ACCServer.sDialog.UpdateRankingList();
        }

        private void BanPlayer_Click(object sender, EventArgs e)
        {
            DBOperations.SetPlayerStatus(SelectedPlayerName, "Banned");
            ACCServer.sDialog.UpdateRankingList();
        }

        private void UnBanPlayer_Click(object sender, EventArgs e)
        {
            DBOperations.SetPlayerStatus(SelectedPlayerName, "Inactive");
            ACCServer.sDialog.UpdateRankingList();
        }

        private void Rankings_SelectedIndexChanged(object sender, EventArgs e)
        {
            SelectedPlayerName = Stats.allPlayers.player[Rankings.SelectedIndices[0]].Name;
        }
    }
}
