// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class RollbackInfo : GLib.Opaque {

		[DllImport("libredcarpet")]
		static extern void rc_rollback_info_free(IntPtr raw);

		public void Free() {
			rc_rollback_info_free(Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_rollback_info_save(IntPtr raw);

		public void Save() {
			rc_rollback_info_save(Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_rollback_info_discard(IntPtr raw);

		public void Discard() {
			rc_rollback_info_discard(Handle);
		}

		public RollbackInfo(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern unsafe IntPtr rc_rollback_info_new(IntPtr world, IntPtr install_packages, IntPtr remove_packages, out IntPtr err);

		public unsafe RollbackInfo (RC.World world, GLib.SList install_packages, GLib.SList remove_packages) 
		{
			IntPtr error = IntPtr.Zero;
			Raw = rc_rollback_info_new(world.Handle, install_packages.Handle, remove_packages.Handle, out error);
			if (error != IntPtr.Zero) throw new GLib.GException (error);
		}

#endregion
	}
}