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
    class CSharpConnection : M1Connection
    {
        protected TcpClient tcp;

        public override bool Connect(string location, int port)
        {
            tcp = new TcpClient(location, port);
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

    class SecureConnection : CSharpConnection
    {
        SslStream innerStream;
        public override bool Connect(string location, int port)
        {
            base.Connect(location, port);
            innerStream = new SslStream(
                tcp.GetStream(),
                false,
                new RemoteCertificateValidationCallback(delegate { return true; }),
                null
                );
            try
            {
                innerStream.AuthenticateAsClient(location);
            }
            catch
            {
                return false;
            }
            return true;
        }

        public override void Send(CharVector data)
        {
            byte[] send = new byte[data.Count];
            for (int i = 0; i < data.Count; i++)
                send[i] = (byte)data[i];
            innerStream.Write(send, 0, data.Count);
        }

        public override CharVector Recieve()
        {
            byte[] recv = new byte[256];
            int recieved = innerStream.Read(recv, 0, recv.Length);
            CharVector cv = new CharVector(recieved);
            for (int i = 0; i < recieved; i++)
            {
                cv.Add((char)recv[i]);
            }
            return cv;
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            SecureConnection conn = new SecureConnection();
            // Connect and dispatch Proxy Manager
            conn.Connect("dev1.elklink.com", 8891);

            C1M1Tunnel tunn = new C1M1Tunnel(conn);
            tunn.Authenticate("telssadmin", "Elk12345", "0050C2688037");

            M1AsciiAPI m1api = new M1AsciiAPI(conn);
            m1api.run();

            foreach (TempDevicePair tdp in m1api.getConfiguredTempDevices())
            {
                Console.Write(tdp.first);
                Console.Write(' ');
                Console.WriteLine(tdp.second);
            }

            Thread.Sleep(30000);
            m1api.stop();

        }
    }
}
