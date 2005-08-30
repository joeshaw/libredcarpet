// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class PackageUpdate : RC.PackageSpec {

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_signature_url(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_signature_url(IntPtr raw, IntPtr value);

		public string SignatureUrl {
			get  {
				IntPtr raw_ret = rc_package_update_get_signature_url(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
				return ret;
			}
			set  {
				IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_package_update_set_signature_url(Handle, value_as_native);
				GLib.Marshaller.Free (value_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_license(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_license(IntPtr raw, IntPtr value);

		public string License {
			get  {
				IntPtr raw_ret = rc_package_update_get_license(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
				return ret;
			}
			set  {
				IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_package_update_set_license(Handle, value_as_native);
				GLib.Marshaller.Free (value_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_md5sum(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_md5sum(IntPtr raw, IntPtr value);

		public string Md5sum {
			get  {
				IntPtr raw_ret = rc_package_update_get_md5sum(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
				return ret;
			}
			set  {
				IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_package_update_set_md5sum(Handle, value_as_native);
				GLib.Marshaller.Free (value_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_parent(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_parent(IntPtr raw, IntPtr parent);

		public RC.Package Parent {
			get  {
				IntPtr raw_ret = rc_package_update_get_parent(Handle);
				RC.Package ret = raw_ret == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (raw_ret, typeof (RC.Package), false);
				return ret;
			}
			set  {
				rc_package_update_set_parent(Handle, value == null ? IntPtr.Zero : value.Handle);
			}
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_signature_size(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_signature_size(IntPtr raw, uint value);

		public uint SignatureSize {
			get  {
				uint raw_ret = rc_package_update_get_signature_size(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set  {
				rc_package_update_set_signature_size(Handle, value);
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static uint rcsharp_rc_packageupdate_get_package_offset ();

		static uint package_offset = rcsharp_rc_packageupdate_get_package_offset ();
		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_package(IntPtr raw);

		public RC.Package Package {
			get  {
				IntPtr raw_ret = rc_package_update_get_package(Handle);
				RC.Package ret = raw_ret == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (raw_ret, typeof (RC.Package), false);
				return ret;
			}
			set {
				unsafe {
					IntPtr* raw_ptr = (IntPtr*)(((byte*)Handle) + package_offset);
					*raw_ptr = value == null ? IntPtr.Zero : value.Handle;
				}
			}
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_package_size(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_package_size(IntPtr raw, uint value);

		public uint PackageSize {
			get  {
				uint raw_ret = rc_package_update_get_package_size(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set  {
				rc_package_update_set_package_size(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_description(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_description(IntPtr raw, IntPtr value);

		public string Description {
			get  {
				IntPtr raw_ret = rc_package_update_get_description(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
				return ret;
			}
			set  {
				IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_package_update_set_description(Handle, value_as_native);
				GLib.Marshaller.Free (value_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_installed_size(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_installed_size(IntPtr raw, uint value);

		public uint InstalledSize {
			get  {
				uint raw_ret = rc_package_update_get_installed_size(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set  {
				rc_package_update_set_installed_size(Handle, value);
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static uint rcsharp_rc_packageupdate_get_spec_offset ();

		static uint spec_offset = rcsharp_rc_packageupdate_get_spec_offset ();
		public RC.PackageSpec Spec {
			get {
				unsafe {
					IntPtr* raw_ptr = (IntPtr*)(((byte*)Handle) + spec_offset);
					return (*raw_ptr) == IntPtr.Zero ? null : (RC.PackageSpec) GLib.Opaque.GetOpaque ((*raw_ptr), typeof (RC.PackageSpec), false);
				}
			}
			set {
				unsafe {
					IntPtr* raw_ptr = (IntPtr*)(((byte*)Handle) + spec_offset);
					*raw_ptr = value == null ? IntPtr.Zero : value.Handle;
				}
			}
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_update_get_hid(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_hid(IntPtr raw, uint value);

		public uint Hid {
			get  {
				uint raw_ret = rc_package_update_get_hid(Handle);
				uint ret = raw_ret;
				return ret;
			}
			set  {
				rc_package_update_set_hid(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_get_package_url(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_package_url(IntPtr raw, IntPtr value);

		public string PackageUrl {
			get  {
				IntPtr raw_ret = rc_package_update_get_package_url(Handle);
				string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
				return ret;
			}
			set  {
				IntPtr value_as_native = GLib.Marshaller.StringToPtrGStrdup (value);
				rc_package_update_set_package_url(Handle, value_as_native);
				GLib.Marshaller.Free (value_as_native);
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_package_update_get_importance(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_update_set_importance(IntPtr raw, int value);

		public RC.PackageImportance Importance {
			get  {
				int raw_ret = rc_package_update_get_importance(Handle);
				RC.PackageImportance ret = (RC.PackageImportance) raw_ret;
				return ret;
			}
			set  {
				rc_package_update_set_importance(Handle, (int) value);
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
		static extern void rc_package_update_slist_free(IntPtr update_slist);

		public static void SlistFree(GLib.SList update_slist) {
			rc_package_update_slist_free(update_slist.Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_copy(IntPtr raw);

		public RC.PackageUpdate Copy() {
			IntPtr raw_ret = rc_package_update_copy(Handle);
			RC.PackageUpdate ret = raw_ret == IntPtr.Zero ? null : (RC.PackageUpdate) GLib.Opaque.GetOpaque (raw_ret, typeof (RC.PackageUpdate), true);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_slist_sort(IntPtr old_slist);

		public static GLib.SList SlistSort(GLib.SList old_slist) {
			IntPtr raw_ret = rc_package_update_slist_sort(old_slist.Handle);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		public PackageUpdate(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_update_new();

		public PackageUpdate () 
		{
			Raw = rc_package_update_new();
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_update_free(IntPtr raw);

		protected override void Free (IntPtr raw)
		{
			rc_package_update_free (raw);
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
