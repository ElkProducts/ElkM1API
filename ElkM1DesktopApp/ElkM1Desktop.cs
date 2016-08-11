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
    public partial class ElkM1App : Form
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

        public class BoolUpdateHandler : BoolCallback
        {
            Action<Boolean> lamb;

            public BoolUpdateHandler(Action<Boolean> lambda)
            {
                lamb = lambda;
            }

            public override void run(Boolean status)
            {
                lamb(status);
            }
        }

        SecureConnection cs;
        M1AsciiAPI m1;
        bool connected = false;

        public ElkM1App()
        {
            InitializeComponent();
            cs = new SecureConnection();
            m1 = new M1AsciiAPI(cs);
            AreasList.LargeImageList = new ImageList();
            AreasList.LargeImageList.ImageSize = new Size(48, 48);
            AreasList.LargeImageList.ColorDepth = ColorDepth.Depth32Bit;
            AreasList.LargeImageList.Images.Add("armaway", Properties.Resources.armaway_image_c);
            AreasList.LargeImageList.Images.Add("armstay", Properties.Resources.armstay_image_c);
            AreasList.LargeImageList.Images.Add("disarm", Properties.Resources.disarm_image_c);
            AreasList.LargeImageList.Images.Add("armnight", Properties.Resources.armnight_image_c);


            ZonesList.LargeImageList = new ImageList();
            ZonesList.LargeImageList.ColorDepth = ColorDepth.Depth32Bit;
            ZonesList.LargeImageList.Images.Add("zone_bypassed", Properties.Resources.zone_bypassed);
            ZonesList.LargeImageList.Images.Add("zone_trouble", Properties.Resources.zone_trouble);
            ZonesList.LargeImageList.Images.Add("zone_normal", Properties.Resources.zone_normal);
            ZonesList.LargeImageList.Images.Add("zone_violated", Properties.Resources.zone_violated);
            ZonesList.SmallImageList = ZonesList.LargeImageList;

            OutputsList.LargeImageList = new ImageList();
            OutputsList.LargeImageList.Images.Add("on_symbol", Properties.Resources.on_symbol);
            OutputsList.LargeImageList.Images.Add("off_symbol", Properties.Resources.off_symbol);
            OutputsList.SmallImageList = OutputsList.LargeImageList;



            m1.onArmStatusChange = new ArmStatusUpdateHandler(HandleArmStatusChange);
            m1.onRPConnection = new BoolUpdateHandler(HandleRPConnection);
        }

        public void ClearViews()
        {
            AreasList.Items.Clear();
            ZonesList.Items.Clear();
            OutputsList.Items.Clear();
        }

        public void HandleArmStatusChange(ArmStatusVector v)
        {
            //m1.collectNames(TextDescriptionType.TEXT_AreaName);

            RunOnGUIThread(this, () =>
            {
                foreach (int i in m1.getConfiguredAreas())
                {
                    String areaname = ""; 
                    try {
                        areaname = m1.getTextDescription(TextDescriptionType.TEXT_AreaName, i);
                    } catch {
                        // ...
                    }
                    if (String.IsNullOrEmpty(areaname))
                        areaname = "Area " + (i + 1);

                    ListViewItem Item = (AreasList.Items.ContainsKey(i.ToString())) ?
                        AreasList.Items.Find(i.ToString(), false)[0] :
                        AreasList.Items.Add(new ListViewItem
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
                            Item.Text = areaname;
                            Item.ImageKey = "armnight";
                            break;
                        case ArmMode.ARM_STAY:
                        case ArmMode.ARM_STAYINSTANT:
                        case ArmMode.ARM_STAYNEXT:
                            Item.Text = areaname;
                            Item.ImageKey = "armstay";
                            break;
                    }
                }
            });
        }

        public void HandleRPConnection(Boolean rpconnected)
        {
            if (rpconnected)
            {
                // TODO: Grey out all controls, display "Installer Connected"
            }
            else
            {
                // TODO: Reenable all controls
                RunOnGUIThread(this, ClearViews);
            }
        }

        public void HandleControlOutputChange(BoolVector v)
        {
            ThreadPool.QueueUserWorkItem(o =>
            {
                //m1.collectNames(TextDescriptionType.TEXT_OutputName);

                RunOnGUIThread(this, () =>
                {
                    for (int i = 0; i < v.Count; i++)
                    {
                        if (ShowAllOutputsCheckbox.Checked) // || (m1.getTextDescription(TextDescriptionType.TEXT_OutputName, i) != ""))
                        {
                            ListViewItem Item = (OutputsList.Items.ContainsKey(i.ToString())) ?
                                OutputsList.Items.Find(i.ToString(), false)[0] :
                                OutputsList.Items.Add(new ListViewItem
                                {
                                    Name = i.ToString()
                                });

                            Item.Text = m1.getTextDescription(TextDescriptionType.TEXT_OutputName, i);
                            if (Item.Text == "")
                                Item.Text = "Output " + (i + 1).ToString();
                            if (v[i])
                            {
                                Item.ImageKey = "on_symbol";
                            }
                            else
                            {
                                Item.ImageKey = "off_symbol";
                            }
                        }
                    }
                });
            });
        }

        private void RunOnGUIThread(Control c, Action fun)
        {
            if (c.InvokeRequired)
            {
                Invoke((MethodInvoker)delegate
                {
                    fun();
                });
            }
            else
            {
                fun();
            }
        }

        private void Connect_Click(object sender, EventArgs e)
        {
            ThreadPool.QueueUserWorkItem(o =>
            {
                if (!connected)
                {
                    if(cs.Connect("dev.elklink.com", 8891)) {

                        C1M1Tunnel tunn = new C1M1Tunnel(cs);
                        if (tunn.Authenticate("xiong", "Elk12345", "0050C2688038") != NetworkType.NETWORKTYPE_NONE)
                        {

                            connected = !connected;
                            m1.run();
                            RunOnGUIThread(this, () =>
                            {
                                Connect.Text = "Disconnect";
                                UpdateTab(tabControls.SelectedTab);
                            });
                            // Takes the longest, might as well 'sync'
                            HandleControlOutputChange(m1.getControlOutputs());
                        } else
                        {
                            Console.WriteLine("Failed to auth!");
                        }
                    } else
                    {
                        Console.WriteLine("Failed to connect!");
                    }
                }
                else
                {
                    RunOnGUIThread(this, () =>
                    {
                        ClearViews();
                        Connect.Text = "Connect";
                    });
                    try
                    {
                        m1.stop();
                        connected = !connected;
                    }
                    catch
                    {
                        //
                    }
                }
            });
        }

        private void AreasList_DoubleClick(object sender, EventArgs e)
        {
            m1.armDisarm(Int32.Parse(AreasList.SelectedItems[0].Name), ArmMode.ARM_AWAY, "1111");
        }

        private void AreasList_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
        {
            ZonesList.Items.Clear();
            if (AreasList.SelectedItems.Count > 0)
            {
                int area = Int32.Parse(AreasList.SelectedItems[0].Name);

                //m1.collectNames(TextDescriptionType.TEXT_ZoneName);
                // For each zone defined which uses this area...
                var states = m1.getZoneStatuses();
                foreach (int index in m1.getZonePartitions().Select((zArea, index) =>
                {
                    if (area == zArea)
                        return index;
                    else
                        return -1;
                }).Intersect(m1.getConfiguredZones()))
                {
                    // Add it to the list
                    ZonesList.Items.Add(new ListViewItem
                    {
                        Name = index.ToString(),
                        Text = m1.getTextDescription(TextDescriptionType.TEXT_ZoneName, index),
                    });
                    ZonesList.Columns[0].Width = -1;
                    switch (states[index].logicalState)
                    {
                        case LogicalZoneState.LZS_BYPASSED:
                            ZonesList.Items[index.ToString()].ImageKey = "zone_bypassed";
                            ZonesList.Items[index.ToString()].SubItems.Add("Bypassed");
                            break;
                        case LogicalZoneState.LZS_NORMAL:
                            ZonesList.Items[index.ToString()].ImageKey = "zone_normal";
                            ZonesList.Items[index.ToString()].SubItems.Add("Normal");
                            break;
                        case LogicalZoneState.LZS_TROUBLE:
                            ZonesList.Items[index.ToString()].ImageKey = "zone_trouble";
                            ZonesList.Items[index.ToString()].SubItems.Add("Troubled");
                            break;
                        case LogicalZoneState.LZS_VIOLATED:
                            ZonesList.Items[index.ToString()].ImageKey = "zone_violated";
                            ZonesList.Items[index.ToString()].SubItems.Add("Violated");
                            break;
                    }
                    switch (states[index].physicalState)
                    {
                        case PhysicalZoneState.PZS_EOL:
                            ZonesList.Items[index.ToString()].SubItems.Add("EOL");
                            break;
                        case PhysicalZoneState.PZS_OPEN:
                            ZonesList.Items[index.ToString()].SubItems.Add("Open");
                            break;
                        case PhysicalZoneState.PZS_SHORT:
                            ZonesList.Items[index.ToString()].SubItems.Add("Short");
                            break;
                    }
                }
                ZonesList.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);
            }
        }

        private void UpdateTab(TabPage tab)
        {
            if (!connected)
                return;
            switch (tab.Name)
            {
                case "Security":
                    m1.getArmStatus(); // Callback handles the result.
                    break;
                case "Temperature":

                    break;
                case "Lighting":

                    break;
                case "Outputs":
                    HandleControlOutputChange(m1.getControlOutputs());
                    break;

            }
        }

        private void tabControls_TabIndexChanged(object sender, EventArgs e)
        {
            UpdateTab(tabControls.SelectedTab);
        }

        private void tabControls_TabIndexChanged(object sender, TabControlCancelEventArgs e)
        {
            UpdateTab(tabControls.SelectedTab);
        }

        private void ShowAllOutputsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (!connected)
                return;
            OutputsList.Items.Clear();
            HandleControlOutputChange(m1.getControlOutputs());
        }

        private void ZonesList_DoubleClick(object sender, EventArgs e)
        {
            if (ZonesList.SelectedItems.Count > 0)
                m1.zoneBypass(Int32.Parse(ZonesList.SelectedItems[0].Name), "1111");
            AreasList_ItemSelectionChanged(null, null);
        }

        private void OutputsList_DoubleClick(object sender, EventArgs e)
        {
            if (OutputsList.SelectedItems.Count > 0)
                m1.toggleControlOutput(Int32.Parse(OutputsList.SelectedItems[0].Name));
            HandleControlOutputChange(m1.getControlOutputs());
        }
    }
}
