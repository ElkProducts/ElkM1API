using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Reflection;
using System.Threading;

namespace ElkM1DesktopApp
{

    public class ArmStatusUpdateHandler : ArmStatusVectorCallback
    {
        Action<ArmStatusVector> lamb;

        public ArmStatusUpdateHandler(Action<ArmStatusVector> lambda)
        {
            lamb = lambda;
        }

        public override void run(ArmStatusVector status)
        {
            lamb(status);
        }
    }

    public partial class ElkM1App : Form
    {
        CSharpConnection cs;
        M1AsciiAPI m1;
        bool connected = false;

        public ElkM1App()
        {
            InitializeComponent();
            cs = new CSharpConnection();
            m1 = new M1AsciiAPI(cs);
            AreasList.LargeImageList = new ImageList();
            AreasList.LargeImageList.ImageSize = new Size(48, 48);
            AreasList.LargeImageList.ColorDepth = ColorDepth.Depth32Bit;
            AreasList.LargeImageList.Images.Add("armaway", Properties.Resources.armaway_image_c);
            AreasList.LargeImageList.Images.Add("armstay", Properties.Resources.armstay_image_c);
            AreasList.LargeImageList.Images.Add("disarm", Properties.Resources.disarm_image_c);
            
            m1.onArmStatusChange = new ArmStatusUpdateHandler(HandleArmStatusChange);
        }

        public void HandleArmStatusChange(ArmStatusVector v)
        {
            if (InvokeRequired)
            {
                Invoke((MethodInvoker)delegate { HandleArmStatusChange(v); });
                return;
            }

            m1.collectNames(TextDescriptionType.TEXT_AreaName);

            foreach (int i in m1.getConfiguredAreas())
            {
                String areaname = m1.getTextDescription(TextDescriptionType.TEXT_AreaName, i);
                if (String.IsNullOrEmpty(areaname))
                    areaname = "Area " + (i + 1);

                ListViewItem Item;
                if (AreasList.Items.ContainsKey(i.ToString()))
                    Item = AreasList.Items.Find(i.ToString(), false)[0];
                else
                    Item = AreasList.Items.Add(new ListViewItem
                    {
                        Name = i.ToString()
                    });

                switch (v[i].mode)
                {
                    case ArmMode.ARM_AWAY:
                    case ArmMode.ARM_AWAYNEXT:
                    case ArmMode.ARM_VACATION:
                        Item.Text = areaname;
                        Item.ImageKey = "armaway";
                        break;
                    case ArmMode.ARM_DISARMED:
                        Item.Text = areaname;
                        Item.ImageKey = "disarm";
                        break;
                    case ArmMode.ARM_NIGHT:
                    case ArmMode.ARM_NIGHTINSTANT:
                    case ArmMode.ARM_STAY:
                    case ArmMode.ARM_STAYINSTANT:
                    case ArmMode.ARM_STAYNEXT:
                        Item.Text = areaname;
                        Item.ImageKey = "armstay";
                        break;
                }
            }
            return;
        }

        private void Connect_Click(object sender, EventArgs e)
        {
            ThreadPool.QueueUserWorkItem(o => {
                if (!connected)
                {
                    cs.Connect("192.168.101.104", 2101);
                    m1.run();
                    if (InvokeRequired)
                    {
                        Invoke((MethodInvoker)delegate
                        {
                            Connect.Text = "Disconnect";
                        });
                    }
                    else
                    {
                        Connect.Text = "Disconnect";
                    }

                    HandleArmStatusChange(m1.getArmStatus()); // Callback handles the result.
                }
                else
                {
                    if (InvokeRequired)
                    {
                        Invoke((MethodInvoker)delegate
                        {
                            AreasList.Items.Clear();
                            Connect.Text = "Connect";
                        });
                    }
                    else
                    {
                        AreasList.Items.Clear();
                        Connect.Text = "Connect";
                    }
                    m1.stop();
                }
                connected = !connected;
            });
        }

        private void AreasList_DoubleClick(object sender, EventArgs e)
        {
            m1.armDisarm(Int32.Parse(AreasList.SelectedItems[0].Name), ArmMode.ARM_AWAY, "1111");
        }
    }
}
