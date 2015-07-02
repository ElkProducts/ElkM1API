using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Net.Sockets;
using System.Net.Security;

namespace CSharpTester
{
    class Program
    {
        static void Main(string[] args)
        {
            //SecureConnection conn = new SecureConnection();
            //// Connect and dispatch Proxy Manager
            //conn.Connect("dev1.elklink.com", 8891);
            //
            //C1M1Tunnel tunn = new C1M1Tunnel(conn);
            //Console.Write(tunn.Authenticate("telssadmin", "Elk12345", "0050C2688037"));
            //
            //
            CSharpConnection conn = new CSharpConnection();
            
            conn.Connect("192.168.101.151", 2101);

            M1AsciiAPI m1api = new M1AsciiAPI(conn);
            m1api.run();

            foreach (int index in m1api.getConfiguredZones())
            {
                Console.Write(m1api.getTextDescription(TextDescriptionType.TEXT_ZoneName, index) + "\n");
            }

            Thread.Sleep(30000);
            m1api.stop();

        }
    }
}
