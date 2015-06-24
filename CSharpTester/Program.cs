using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Net.Sockets;

namespace CSharpTester
{
    class CSharpConnection : M1Connection
    {
        TcpClient tcp;

        public override bool Connect(string location)
        {
            tcp = new TcpClient(location, 2101);
            return tcp.Connected;
        }

        public override void Disconnect()
        {
            tcp.Close();
        }

        public override CharVector Recieve()
        {
            byte[] recv = new byte[256];
            int recieved = tcp.GetStream().Read(recv, 0, recv.Length);
            CharVector cv = new CharVector(recieved);
            for (int i = 0; i < recieved; i++)
            {
                cv.Add((char)recv[i]);
            }
            return cv;
        }

        public override void Send(CharVector data)
        {
            byte[] send = new byte[data.Count];
            for (int i = 0; i < data.Count; i++)
                send[i] = (byte)data[i];
            tcp.GetStream().Write(send, 0, data.Count);
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            M1Connection conn = new CSharpConnection();
            conn.Connect("192.168.101.104");
            M1AsciiAPI m1api = new M1AsciiAPI(conn);
            m1api.run();

            foreach (ushort u in m1api.getCustomValues())
            {
                Console.WriteLine(u);
            }

            Thread.Sleep(30000);
            m1api.stop();
        }
    }
}
