// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class PackageSpec : GLib.Opaque {

		[DllImport("libredcarpet")]
		static extern bool rc_package_spec_has_epoch(IntPtr raw);

		public bool HasEpoch { 
			get {
				bool raw_ret = rc_package_spec_has_epoch(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_package_spec_compare_name(IntPtr a, IntPtr b);

		public static int CompareName(IntPtr a, IntPtr b) {
			int raw_ret = rc_package_spec_compare_name(a, b);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_spec_get_name(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_set_name(IntPtr raw, string value);

		public string Name { 
			get {
				IntPtr raw_ret = rc_package_spec_get_name(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_spec_set_name(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_spec_to_str(IntPtr raw);

		public override string ToString() {
			IntPtr raw_ret = rc_package_spec_to_str(Handle);
			string ret = GLib.Marshaller.PtrToStringGFree(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_spec_version_to_str(IntPtr raw);

		public string VersionToString() {
			IntPtr raw_ret = rc_package_spec_version_to_str(Handle);
			string ret = GLib.Marshaller.PtrToStringGFree(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern uint rc_package_spec_hash(IntPtr ptr);

		public static uint Hash(IntPtr ptr) {
			uint raw_ret = rc_package_spec_hash(ptr);
			uint ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_spec_slist_find_name(IntPtr specs, string name);

		public static IntPtr SlistFindName(GLib.SList specs, string name) {
			IntPtr raw_ret = rc_package_spec_slist_find_name(specs.Handle, name);
			IntPtr ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_init(IntPtr raw, string name, bool has_epoch, uint epoch, string version, string release);

		public void Init(string name, bool has_epoch, uint epoch, string version, string release) {
			rc_package_spec_init(Handle, name, has_epoch, epoch, version, release);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_spec_get_version(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_set_version(IntPtr raw, string value);

		public string Version { 
			get {
				IntPtr raw_ret = rc_package_spec_get_version(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_spec_set_version(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_package_spec_get_epoch(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_set_epoch(IntPtr raw, int value);

		public int Epoch { 
			get {
				int raw_ret = rc_package_spec_get_epoch(Handle);
				int ret = raw_ret;
				return ret;
			}
			set {
				rc_package_spec_set_epoch(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_free_members(IntPtr raw);

		public void FreeMembers() {
			rc_package_spec_free_members(Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_free(IntPtr raw);

		public void Free() {
			rc_package_spec_free(Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_spec_get_release(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_set_release(IntPtr raw, string value);

		public string Release { 
			get {
				IntPtr raw_ret = rc_package_spec_get_release(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_spec_set_release(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_spec_copy(IntPtr raw, IntPtr old);

		public void Copy(RC.PackageSpec old) {
			rc_package_spec_copy(Handle, old.Handle);
		}

		public PackageSpec(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_spec_new(string name, bool has_epoch, uint epoch, string version, string release);

		public PackageSpec (string name, bool has_epoch, uint epoch, string version, string release) 
		{
			Raw = rc_package_spec_new(name, has_epoch, epoch, version, release);
		}

#endregion
#region Customized extensions
#line 1 "PackageSpec.custom"

      public PackageSpec () {}

      [DllImport("libredcarpet")]
	  static extern int rc_package_spec_equal(IntPtr a, IntPtr b);

	  public override bool Equals (object other) {
	      int ret = rc_package_spec_equal (this.Handle,
                                               ((PackageSpec) other).Handle);
	      if (ret == 0)
                  return false;

	      return true;
	  }

#endregion
	}
}
