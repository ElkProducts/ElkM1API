package elkm1androidexample.elk.org.elkm1androidexample;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import elkm1api.C1M1Tunnel;
import elkm1api.IntVector;
import elkm1api.M1AsciiAPI;
import elkm1api.NetworkType;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("elkm1api");
    }

    SecureConnectionWrapper cw;
    M1AsciiAPI m1api;
    C1M1Tunnel tunn;
    boolean connected = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

    }

    public void onConnectClick(View v) {
        final Button button = (Button)v;
        // Create ConnectionWrapper
        if(!connected) {
            new Thread(){
                public void run() {
                    cw = new SecureConnectionWrapper();

                    // ((TextView)this.findViewById(R.id.AddressEnter)).getText().toString()

                    if(cw.Connect("dev.elklink.com", 8891)) {

                        tunn = new C1M1Tunnel(cw);
                        if (tunn.Authenticate("USERNAME", "PASSWORD", "SERNUM") != NetworkType.NETWORKTYPE_NONE)
                        {
                            // Create M1AsciiAPI(cs)

                            m1api = new M1AsciiAPI(cw);

                            // Run m1.run();
                            m1api.run();
                            final IntVector version = m1api.getM1VersionNumber();
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    ((TextView)findViewById(R.id.textView)).setText(version.get(0) + "." + version.get(1) + "." + version.get(2));
                                }
                            });

                            button.setText("Disconnect");
                            connected = true;

                        } else
                        {
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    ((TextView)findViewById(R.id.textView)).setText("failed to authenticate!");
                                }
                            });
                        }
                    } else {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                ((TextView)findViewById(R.id.textView)).setText("failed to connect!");
                            }
                        });

                    }
                }
            }.start();
        } else {
            m1api.stop();
            cw.Disconnect();
            button.setText("Connect");
            ((TextView)this.findViewById(R.id.textView)).setText("");
            connected = false;
        }
    }

    public void onGetClick(View v) {
        Button button = (Button)v;

    }
}
