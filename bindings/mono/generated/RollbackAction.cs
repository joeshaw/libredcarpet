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
				RC.PackageUpdate ret;
				if (raw_ret == IntPtr.Zero)
					ret = null;
				else
					ret = new RC.PackageUpdate(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_rollback_action_slist_free(IntPtr actions);

		public static void SlistFree(GLib.SList actions) {
			rc_rollback_action_slist_free(actions.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_rollback_action_free(IntPtr raw);

		public void Free() {
			rc_rollback_action_free(Handle);
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
				RC.Package ret;
				if (raw_ret == IntPtr.Zero)
					ret = null;
				else
					ret = new RC.Package(raw_ret);
				return ret;
			}
		}

		public RollbackAction(IntPtr raw) : base(raw) {}

#endregion
	}
}
