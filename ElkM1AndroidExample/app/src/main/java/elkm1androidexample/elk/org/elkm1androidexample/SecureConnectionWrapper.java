package elkm1androidexample.elk.org.elkm1androidexample;

import android.util.Log;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.net.Socket;

import javax.net.SocketFactory;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

import elkm1api.CharVector;
import elkm1api.M1Connection;

/**
 * Created by feilen on 8/10/16.
 */
public class SecureConnectionWrapper extends M1Connection {
    protected SSLSocket ssltcp;
    protected BufferedInputStream bis;
    //AutoResetEvent evt = new AutoResetEvent(true);

    public boolean Connect(String location, int port) {
        try {
            // Largely from https://developer.android.com/training/articles/security-ssl.html
            SocketFactory sf = SSLSocketFactory.getDefault();
            ssltcp = (SSLSocket) sf.createSocket(location, port);
            HostnameVerifier hv = HttpsURLConnection.getDefaultHostnameVerifier();
            SSLSession s = ssltcp.getSession();

            if (!hv.verify(location, s)) {
                ssltcp.close();
                return false;
            }

            bis = new BufferedInputStream(ssltcp.getInputStream());

            return ssltcp.isConnected();
        } catch (IOException ioe) {
            return false;
        }
    }

    public CharVector Recieve() {
        byte[] recv = new byte[256];
        int recieved = 0;
        try {
            recieved = bis.read(recv, 0, recv.length);
        } catch (IOException ioe) {
            Log.v("Recieved", "Error recieving: " + ioe.toString());
        }
        //evt.Set();
        CharVector cv = new CharVector(recieved);
        Log.v("Recieved", new String(recv, 0, recieved));

        for (int i = 0; i < recieved; i++) {
            cv.add((char) recv[i]);
        }
        return cv;
    }

    public void Send(CharVector data) {
        byte[] send = new byte[(int) data.size()];
        for (int i = 0; i < data.size(); i++)
            send[i] = (byte) data.get(i);
        //evt.WaitOne(300);
        // TODO: Rate limiting
        try {
            ssltcp.getOutputStream().write(send, 0, (int) data.size());
            ssltcp.getOutputStream().flush();
            Log.v("Sent", new String(send, 0, (int) data.size()));
        } catch (IOException ioe) {
            Log.v("Sent", "Error while sending: " + ioe.toString());
        }
        //Console.WriteLine(Encoding.ASCII.GetString(send, 0, data.Count));
    }

    public void Disconnect() {
        // evt.set();
        try {
            ssltcp.close();
        } catch (IOException ioe) {
            Log.v("Sent", ioe.toString());
        }
    }
}
