// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class PackageUpdate : RC.PackageSpec {

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_license(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_license(IntPtr raw, string value);

		public string License { 
			get {
				IntPtr raw_ret = rc_package_update_get_license(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_update_set_license(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_slist_copy(IntPtr old_update);

		public static GLib.SList SlistCopy(GLib.SList old_update) {
			IntPtr raw_ret = rc_package_update_slist_copy(old_update.Handle);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_package_url(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_package_url(IntPtr raw, string value);

		public string PackageUrl { 
			get {
				IntPtr raw_ret = rc_package_update_get_package_url(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_update_set_package_url(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_package_size(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_package_size(IntPtr raw, uint value);

		public uint PackageSize { 
			get {
				uint raw_ret = rc_package_update_get_package_size(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set {
				rc_package_update_set_package_size(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_package(IntPtr raw);

		public RC.Package Package { 
			get {
				IntPtr raw_ret = rc_package_update_get_package(Handle);
				RC.Package ret;
				if (raw_ret == IntPtr.Zero)
					ret = null;
				else
					ret = new RC.Package(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_package_update_get_importance(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_importance(IntPtr raw, int value);

		public RC.PackageImportance Importance { 
			get {
				int raw_ret = rc_package_update_get_importance(Handle);
				RC.PackageImportance ret = (RC.PackageImportance)raw_ret;
				return ret;
			}
			set {
				rc_package_update_set_importance(Handle, (int) value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_signature_url(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_signature_url(IntPtr raw, string value);

		public string SignatureUrl { 
			get {
				IntPtr raw_ret = rc_package_update_get_signature_url(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_update_set_signature_url(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_slist_sort(IntPtr old_slist);

		public static GLib.SList SlistSort(GLib.SList old_slist) {
			IntPtr raw_ret = rc_package_update_slist_sort(old_slist.Handle);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_update_free(IntPtr raw);

		public void Free() {
			rc_package_update_free(Handle);
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_installed_size(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_installed_size(IntPtr raw, uint value);

		public uint InstalledSize { 
			get {
				uint raw_ret = rc_package_update_get_installed_size(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set {
				rc_package_update_set_installed_size(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_update_slist_free(IntPtr update_slist);

		public static void SlistFree(GLib.SList update_slist) {
			rc_package_update_slist_free(update_slist.Handle);
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_hid(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_hid(IntPtr raw, uint value);

		public uint Hid { 
			get {
				uint raw_ret = rc_package_update_get_hid(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set {
				rc_package_update_set_hid(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_signature_size(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_signature_size(IntPtr raw, uint value);

		public uint SignatureSize { 
			get {
				uint raw_ret = rc_package_update_get_signature_size(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set {
				rc_package_update_set_signature_size(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_description(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_description(IntPtr raw, string value);

		public string Description { 
			get {
				IntPtr raw_ret = rc_package_update_get_description(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_update_set_description(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_md5sum(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_md5sum(IntPtr raw, string value);

		public string Md5sum { 
			get {
				IntPtr raw_ret = rc_package_update_get_md5sum(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_update_set_md5sum(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_copy(IntPtr raw);

		public RC.PackageUpdate Copy() {
			IntPtr raw_ret = rc_package_update_copy(Handle);
			RC.PackageUpdate ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.PackageUpdate(raw_ret);
			return ret;
		}

		public PackageUpdate(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_new();

		public PackageUpdate () 
		{
			Raw = rc_package_update_new();
		}

#endregion
#region Customized extensions
#line 1 "PackageUpdate.custom"
/* -*- Mode: csharp; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

//namespace {
    public void ToXml (System.Xml.XmlTextWriter writer)
    {
        writer.WriteStartElement ("update");

        if (this.HasEpoch)
            writer.WriteElementString ("epoch", System.Xml.XmlConvert.ToString (this.Epoch));

        writer.WriteElementString ("version", this.Version);
        writer.WriteElementString ("release", this.Release);

        if (this.PackageUrl != null)
            writer.WriteElementString ("filename", System.IO.Path.GetFileName (this.PackageUrl));

        writer.WriteElementString ("filesize", System.Xml.XmlConvert.ToString (this.PackageSize));
        writer.WriteElementString ("installedsize", System.Xml.XmlConvert.ToString (this.InstalledSize));

        if (this.SignatureUrl != null) {
            writer.WriteElementString ("signaturename", this.SignatureUrl);
            writer.WriteElementString ("signaturesize", System.Xml.XmlConvert.ToString (this.SignatureSize));
        }

        if (this.Md5sum != null)
            writer.WriteElementString ("md5sum", this.Md5sum);

        writer.WriteElementString ("importance", Global.ImportanceToString (this.Importance));
        writer.WriteElementString ("description", this.Description);

        if (this.Hid != 0)
            writer.WriteElementString ("hid", System.Xml.XmlConvert.ToString (this.Hid));

        if (this.License != null)
            writer.WriteElementString ("license", this.License);

        writer.WriteEndElement ();
    }

#endregion
	}
}
