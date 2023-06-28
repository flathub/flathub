using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Alien_Arena_Account_Server_Manager
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            ACCServer.sDialog = new ACCServer();
            Application.Run(ACCServer.sDialog);

            //close listener socket
            accountServer.sServer.Stop_Server();
        }
    }
}
