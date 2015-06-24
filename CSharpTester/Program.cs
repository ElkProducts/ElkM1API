using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace CSharpTester
{
    class TestCallback : IntCallback
    {
        M1API api;

        public TestCallback(M1API aapi)
            : base()
        {
            api = aapi;
        }

        public override void run(int arg1)
        {
            Console.WriteLine(api.getTextDescription(M1API.TextDescriptionType.TEXT_ZoneName, arg1));
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            M1Connection conn = new ElkTCP();
            conn.Connect("192.168.101.104");
            M1AsciiAPI m1api = new M1AsciiAPI(conn);
            TestCallback funct = new TestCallback(m1api);
            m1api.run();

            m1api.forEachConfiguredZone(funct);
        
            Thread.Sleep(30000);
            m1api.stop();
        }
    }
}
