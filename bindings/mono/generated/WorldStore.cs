// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class WorldStore : RC.World {

		~WorldStore()
		{
			Dispose();
		}

		protected WorldStore(GLib.GType gtype) : base(gtype) {}
		public WorldStore(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_store_new();

		public WorldStore () : base (IntPtr.Zero)
		{
			if (GetType () != typeof (WorldStore)) {
				CreateNativeObject (new string [0], new GLib.Value[0]);
				return;
			}
			Raw = rc_world_store_new();
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_remove_packages(IntPtr raw, IntPtr channel);

		public void RemovePackages(RC.Channel channel) {
			rc_world_store_remove_packages(Handle, channel.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_add_packages_from_slist(IntPtr raw, IntPtr slist);

		public void AddPackagesFromSlist(GLib.SList slist) {
			rc_world_store_add_packages_from_slist(Handle, slist.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_add_lock(IntPtr raw, IntPtr _lock);

		public new void AddLock(RC.PackageMatch _lock) {
			rc_world_store_add_lock(Handle, _lock.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_add_channel(IntPtr raw, IntPtr channel);

		public void AddChannel(RC.Channel channel) {
			rc_world_store_add_channel(Handle, channel.Handle);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_world_store_add_package(IntPtr raw, IntPtr package);

		public bool AddPackage(RC.Package package) {
			bool raw_ret = rc_world_store_add_package(Handle, package.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_remove_channel(IntPtr raw, IntPtr channel);

		public void RemoveChannel(RC.Channel channel) {
			rc_world_store_remove_channel(Handle, channel.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_clear_locks(IntPtr raw);

		public new void ClearLocks() {
			rc_world_store_clear_locks(Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_remove_lock(IntPtr raw, IntPtr _lock);

		public new void RemoveLock(RC.PackageMatch _lock) {
			rc_world_store_remove_lock(Handle, _lock.Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_store_get_type();

		public static new GLib.GType GType { 
			get {
				IntPtr raw_ret = rc_world_store_get_type();
				GLib.GType ret = new GLib.GType(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_clear(IntPtr raw);

		public void Clear() {
			rc_world_store_clear(Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_store_remove_package(IntPtr raw, IntPtr package);

		public void RemovePackage(RC.Package package) {
			rc_world_store_remove_package(Handle, package.Handle);
		}


		static WorldStore ()
		{
			GtkSharp.LibredcarpetSharp.ObjectManager.Initialize ();
		}
#endregion
	}
}
