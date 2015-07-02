using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Security;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

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
}
