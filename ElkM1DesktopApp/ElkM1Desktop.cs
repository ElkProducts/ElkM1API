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

namespace ElkM1DesktopApp
{
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
            AreasList.LargeImageList.Images.Add("armaway", Properties.Resources.armaway_image_c);
            AreasList.LargeImageList.Images.Add("armstay", Properties.Resources.armstay_image_c);
            AreasList.LargeImageList.Images.Add("disarm", Properties.Resources.disarm_image_c);

        }

        private void Connect_Click(object sender, EventArgs e)
        {

            if (!connected)
            {
                AreasList.Items.Clear();
                cs.Connect("192.168.101.151", 2101);
                m1.run();

                ArmStatusVector v = m1.getArmStatus();
                m1.collectNames(TextDescriptionType.TEXT_AreaName);

                foreach (int i in m1.getConfiguredAreas())
                {
                    String areaname = m1.getTextDescription(TextDescriptionType.TEXT_AreaName, i);
                    if(String.IsNullOrEmpty(areaname))
                        areaname = "Area " + (i + 1);
                    switch (v[i].mode)
                    {
                        case ArmMode.ARM_AWAY:
                        case ArmMode.ARM_AWAYNEXT:
                        case ArmMode.ARM_VACATION:
                            AreasList.Items.Add(areaname, "armaway");
                            break;
                        case ArmMode.ARM_DISARMED:
                            AreasList.Items.Add(areaname, "disarm");
                            break;
                        case ArmMode.ARM_NIGHT:
                        case ArmMode.ARM_NIGHTINSTANT:
                        case ArmMode.ARM_STAY:
                        case ArmMode.ARM_STAYINSTANT:
                        case ArmMode.ARM_STAYNEXT:
                            AreasList.Items.Add(areaname, "armstay");
                            break;
                    }
                }
                Connect.Text = "Disconnect";
            }
            else
            {
                m1.stop();
                AreasList.Items.Clear();
                Connect.Text = "Connect";
            }
            connected = !connected;
        }
    }
}
