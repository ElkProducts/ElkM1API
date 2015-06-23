using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace CSharpTester
{
    class Program
    {
        static void Main(string[] args)
        {
            M1Connection conn = new ElkTCP();
            conn.Connect("192.168.101.104");
            M1AsciiAPI m1api = new M1AsciiAPI(conn);
            m1api.run();
            foreach(M1API.SZoneDefinition zone in m1api.getZoneDefinitions()) {
                Console.WriteLine(zone.zd);
            }
            Thread.Sleep(10000);
            m1api.stop();
        }
    }
}
