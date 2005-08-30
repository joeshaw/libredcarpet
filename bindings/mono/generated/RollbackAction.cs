// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class RollbackAction : GLib.Opaque {

		[DllImport("libredcarpet")]
		static extern IntPtr rc_rollback_action_get_package_update(IntPtr raw);

		public RC.PackageUpdate PackageUpdate { 
			get {
				IntPtr raw_ret = rc_rollback_action_get_package_update(Handle);
				RC.PackageUpdate ret = raw_ret == IntPtr.Zero ? null : (RC.PackageUpdate) GLib.Opaque.GetOpaque (raw_ret, typeof (RC.PackageUpdate), false);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_rollback_action_slist_free(IntPtr actions);

		public static void SlistFree(GLib.SList actions) {
			rc_rollback_action_slist_free(actions.Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_rollback_action_get_synth_package(IntPtr raw);

		public RC.Package SynthPackage { 
			get {
				IntPtr raw_ret = rc_rollback_action_get_synth_package(Handle);
				RC.Package ret = raw_ret == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (raw_ret, typeof (RC.Package), false);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_rollback_action_is_install(IntPtr raw);

		public bool IsInstall { 
			get {
				bool raw_ret = rc_rollback_action_is_install(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_rollback_action_get_package(IntPtr raw);

		public RC.Package Package { 
			get {
				IntPtr raw_ret = rc_rollback_action_get_package(Handle);
				RC.Package ret = raw_ret == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (raw_ret, typeof (RC.Package), false);
				return ret;
			}
		}

		public RollbackAction(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern void rc_rollback_action_free(IntPtr raw);

		protected override void Free (IntPtr raw)
		{
			rc_rollback_action_free (raw);
		}

#endregion
	}
}
