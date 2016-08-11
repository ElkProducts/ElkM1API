package elkm1androidexample.elk.org.elkm1androidexample;

import elkm1api.CharVector;
import elkm1api.M1Connection;

import java.io.IOException;
import java.net.Socket;

/**
 * Created by feilen on 8/10/16.
 */
public class ConnectionWrapper extends M1Connection {
    protected Socket tcp;
    //AutoResetEvent evt = new AutoResetEvent(true);

    public boolean Connect(String location, int port)
    {
        try {
            tcp = new Socket(location, port);
            return tcp.isConnected();
        } catch (IOException ioe) {
            return false;
        }
    }

    public CharVector Recieve()
    {
            byte[] recv = new byte[256];
            int recieved = 0;
            try {
                recieved = tcp.getInputStream().read(recv, 0, recv.length);
            } catch (IOException ioe) {

            }
            //evt.Set();
            CharVector cv = new CharVector(recieved);
            for (int i = 0; i < recieved; i++)
            {
                cv.add((char)recv[i]);
            }
            return cv;

    }

    public void Send(CharVector data)
    {
        byte[] send = new byte[(int)data.size()];
        for (int i = 0; i < data.size(); i++)
            send[i] = (byte)data.get(i);
        //evt.WaitOne(300);
        // TODO: Rate limiting
        try {
            tcp.getOutputStream().write(send, 0, (int)data.size());
        } catch (IOException ioe) {

        }
        //Console.WriteLine(Encoding.ASCII.GetString(send, 0, data.Count));
    }

    public void Disconnect()
    {
        // evt.set();
        try {
            tcp.close();
        } catch (IOException ioe) {
            //
        }
    }

}

