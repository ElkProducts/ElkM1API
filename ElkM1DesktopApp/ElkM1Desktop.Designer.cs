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
            this.AreasList = new System.Windows.Forms.ListView();
            this.Temperature = new System.Windows.Forms.TabPage();
            this.Lighting = new System.Windows.Forms.TabPage();
            this.Outputs = new System.Windows.Forms.TabPage();
            this.Tasks = new System.Windows.Forms.TabPage();
            this.Custom_Settings = new System.Windows.Forms.TabPage();
            this.Connect = new System.Windows.Forms.Button();
            this.tabControls.SuspendLayout();
            this.Security.SuspendLayout();
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
            // 
            // Security
            // 
            this.Security.Controls.Add(this.AreasList);
            this.Security.Location = new System.Drawing.Point(4, 22);
            this.Security.Name = "Security";
            this.Security.Size = new System.Drawing.Size(592, 363);
            this.Security.TabIndex = 0;
            this.Security.Text = "Security";
            this.Security.UseVisualStyleBackColor = true;
            // 
            // AreasList
            // 
            this.AreasList.Location = new System.Drawing.Point(3, 3);
            this.AreasList.Name = "AreasList";
            this.AreasList.Size = new System.Drawing.Size(586, 357);
            this.AreasList.TabIndex = 0;
            this.AreasList.UseCompatibleStateImageBehavior = false;
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
            this.Outputs.Location = new System.Drawing.Point(4, 22);
            this.Outputs.Name = "Outputs";
            this.Outputs.Size = new System.Drawing.Size(592, 363);
            this.Outputs.TabIndex = 3;
            this.Outputs.Text = "Outputs";
            this.Outputs.UseVisualStyleBackColor = true;
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
    }
}

