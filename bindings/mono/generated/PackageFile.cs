// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	[StructLayout(LayoutKind.Sequential)]
	public struct PackageFile {

		public string Filename;
		public uint Size;
		public string Md5sum;
		public uint Uid;
		public uint Gid;
		public ushort Mode;
		public int Mtime;
		public bool Ghost;

		public static RC.PackageFile Zero = new RC.PackageFile ();

		public static RC.PackageFile New(IntPtr raw) {
			if (raw == IntPtr.Zero) {
				return RC.PackageFile.Zero;
			}
			RC.PackageFile self = new RC.PackageFile();
			self = (RC.PackageFile) Marshal.PtrToStructure (raw, self.GetType ());
			return self;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_file_new();

		public static PackageFile New()
		{
			return PackageFile.New (rc_package_file_new());
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_file_slist_free(IntPtr files);

		public static void SlistFree(GLib.SList files) {
			rc_package_file_slist_free(files.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_file_free(ref RC.PackageFile raw);

		public void Free() {
			rc_package_file_free(ref this);
		}

#endregion
	}
}
