namespace ElkM1DesktopApp
{
    partial class ElkM1App
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.tabControls = new System.Windows.Forms.TabControl();
            this.Security = new System.Windows.Forms.TabPage();
            this.ZonesList = new System.Windows.Forms.ListView();
            this.columnZoneName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnLogicalZoneState = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnPhysicalZoneState = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.AreasList = new System.Windows.Forms.ListView();
            this.Temperature = new System.Windows.Forms.TabPage();
            this.Lighting = new System.Windows.Forms.TabPage();
            this.Outputs = new System.Windows.Forms.TabPage();
            this.ShowAllOutputsCheckbox = new System.Windows.Forms.CheckBox();
            this.OutputsList = new System.Windows.Forms.ListView();
            this.Tasks = new System.Windows.Forms.TabPage();
            this.Custom_Settings = new System.Windows.Forms.TabPage();
            this.Connect = new System.Windows.Forms.Button();
            this.tabControls.SuspendLayout();
            this.Security.SuspendLayout();
            this.Outputs.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControls
            // 
            this.tabControls.Controls.Add(this.Security);
            this.tabControls.Controls.Add(this.Temperature);
            this.tabControls.Controls.Add(this.Lighting);
            this.tabControls.Controls.Add(this.Outputs);
            this.tabControls.Controls.Add(this.Tasks);
            this.tabControls.Controls.Add(this.Custom_Settings);
            this.tabControls.Location = new System.Drawing.Point(12, 12);
            this.tabControls.Name = "tabControls";
            this.tabControls.SelectedIndex = 0;
            this.tabControls.Size = new System.Drawing.Size(600, 389);
            this.tabControls.TabIndex = 0;
            this.tabControls.Selecting += new System.Windows.Forms.TabControlCancelEventHandler(this.tabControls_TabIndexChanged);
            this.tabControls.TabIndexChanged += new System.EventHandler(this.tabControls_TabIndexChanged);
            // 
            // Security
            // 
            this.Security.Controls.Add(this.ZonesList);
            this.Security.Controls.Add(this.AreasList);
            this.Security.Location = new System.Drawing.Point(4, 22);
            this.Security.Name = "Security";
            this.Security.Size = new System.Drawing.Size(592, 363);
            this.Security.TabIndex = 0;
            this.Security.Text = "Security";
            this.Security.UseVisualStyleBackColor = true;
            // 
            // ZonesList
            // 
            this.ZonesList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnZoneName,
            this.columnLogicalZoneState,
            this.columnPhysicalZoneState});
            this.ZonesList.Location = new System.Drawing.Point(3, 101);
            this.ZonesList.Name = "ZonesList";
            this.ZonesList.Size = new System.Drawing.Size(586, 259);
            this.ZonesList.TabIndex = 1;
            this.ZonesList.UseCompatibleStateImageBehavior = false;
            this.ZonesList.View = System.Windows.Forms.View.Details;
            // 
            // columnZoneName
            // 
            this.columnZoneName.Text = "Zone Name";
            this.columnZoneName.Width = 68;
            // 
            // columnLogicalZoneState
            // 
            this.columnLogicalZoneState.Text = "Logical State";
            this.columnLogicalZoneState.Width = 74;
            // 
            // columnPhysicalZoneState
            // 
            this.columnPhysicalZoneState.Text = "Physical State";
            this.columnPhysicalZoneState.Width = 440;
            // 
            // AreasList
            // 
            this.AreasList.Location = new System.Drawing.Point(3, 3);
            this.AreasList.Name = "AreasList";
            this.AreasList.Size = new System.Drawing.Size(586, 92);
            this.AreasList.TabIndex = 0;
            this.AreasList.UseCompatibleStateImageBehavior = false;
            this.AreasList.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.AreasList_ItemSelectionChanged);
            this.AreasList.DoubleClick += new System.EventHandler(this.AreasList_DoubleClick);
            // 
            // Temperature
            // 
            this.Temperature.Location = new System.Drawing.Point(4, 22);
            this.Temperature.Name = "Temperature";
            this.Temperature.Size = new System.Drawing.Size(592, 363);
            this.Temperature.TabIndex = 1;
            this.Temperature.Text = "Temperature";
            this.Temperature.UseVisualStyleBackColor = true;
            // 
            // Lighting
            // 
            this.Lighting.Location = new System.Drawing.Point(4, 22);
            this.Lighting.Name = "Lighting";
            this.Lighting.Size = new System.Drawing.Size(592, 363);
            this.Lighting.TabIndex = 2;
            this.Lighting.Text = "Lighting";
            this.Lighting.UseVisualStyleBackColor = true;
            // 
            // Outputs
            // 
            this.Outputs.Controls.Add(this.ShowAllOutputsCheckbox);
            this.Outputs.Controls.Add(this.OutputsList);
            this.Outputs.Location = new System.Drawing.Point(4, 22);
            this.Outputs.Name = "Outputs";
            this.Outputs.Size = new System.Drawing.Size(592, 363);
            this.Outputs.TabIndex = 3;
            this.Outputs.Text = "Outputs";
            this.Outputs.UseVisualStyleBackColor = true;
            // 
            // ShowAllOutputsCheckbox
            // 
            this.ShowAllOutputsCheckbox.AutoSize = true;
            this.ShowAllOutputsCheckbox.Location = new System.Drawing.Point(482, 343);
            this.ShowAllOutputsCheckbox.Name = "ShowAllOutputsCheckbox";
            this.ShowAllOutputsCheckbox.Size = new System.Drawing.Size(107, 17);
            this.ShowAllOutputsCheckbox.TabIndex = 1;
            this.ShowAllOutputsCheckbox.Text = "Show All Outputs";
            this.ShowAllOutputsCheckbox.UseVisualStyleBackColor = true;
            this.ShowAllOutputsCheckbox.CheckedChanged += new System.EventHandler(this.ShowAllOutputsCheckbox_CheckedChanged);
            // 
            // OutputsList
            // 
            this.OutputsList.Location = new System.Drawing.Point(3, 3);
            this.OutputsList.Name = "OutputsList";
            this.OutputsList.Size = new System.Drawing.Size(586, 334);
            this.OutputsList.TabIndex = 0;
            this.OutputsList.UseCompatibleStateImageBehavior = false;
            this.OutputsList.View = System.Windows.Forms.View.List;
            // 
            // Tasks
            // 
            this.Tasks.Location = new System.Drawing.Point(4, 22);
            this.Tasks.Name = "Tasks";
            this.Tasks.Size = new System.Drawing.Size(592, 363);
            this.Tasks.TabIndex = 4;
            this.Tasks.Text = "Tasks";
            this.Tasks.UseVisualStyleBackColor = true;
            // 
            // Custom_Settings
            // 
            this.Custom_Settings.Location = new System.Drawing.Point(4, 22);
            this.Custom_Settings.Name = "Custom_Settings";
            this.Custom_Settings.Size = new System.Drawing.Size(592, 363);
            this.Custom_Settings.TabIndex = 5;
            this.Custom_Settings.Text = "Custom Settings";
            this.Custom_Settings.UseVisualStyleBackColor = true;
            // 
            // Connect
            // 
            this.Connect.Location = new System.Drawing.Point(537, 407);
            this.Connect.Name = "Connect";
            this.Connect.Size = new System.Drawing.Size(75, 23);
            this.Connect.TabIndex = 1;
            this.Connect.Text = "Connect";
            this.Connect.UseVisualStyleBackColor = true;
            this.Connect.Click += new System.EventHandler(this.Connect_Click);
            // 
            // ElkM1App
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(624, 442);
            this.Controls.Add(this.Connect);
            this.Controls.Add(this.tabControls);
            this.Name = "ElkM1App";
            this.Text = "Elk M1 Desktop";
            this.tabControls.ResumeLayout(false);
            this.Security.ResumeLayout(false);
            this.Outputs.ResumeLayout(false);
            this.Outputs.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControls;
        private System.Windows.Forms.TabPage Security;
        private System.Windows.Forms.TabPage Temperature;
        private System.Windows.Forms.TabPage Lighting;
        private System.Windows.Forms.TabPage Outputs;
        private System.Windows.Forms.TabPage Tasks;
        private System.Windows.Forms.TabPage Custom_Settings;
        private System.Windows.Forms.Button Connect;
        private System.Windows.Forms.ListView AreasList;
        private System.Windows.Forms.ListView ZonesList;
        private System.Windows.Forms.ListView OutputsList;
        private System.Windows.Forms.CheckBox ShowAllOutputsCheckbox;
        private System.Windows.Forms.ColumnHeader columnZoneName;
        private System.Windows.Forms.ColumnHeader columnLogicalZoneState;
        private System.Windows.Forms.ColumnHeader columnPhysicalZoneState;
    }
}

