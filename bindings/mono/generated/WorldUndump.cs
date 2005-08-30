// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public  class WorldUndump : RC.WorldStore {

		~WorldUndump()
		{
			Dispose();
		}

		[Obsolete]
		protected WorldUndump(GLib.GType gtype) : base(gtype) {}
		public WorldUndump(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_undump_new(IntPtr filename);

		public WorldUndump (string filename) : base (IntPtr.Zero)
		{
			if (GetType () != typeof (WorldUndump)) {
				throw new InvalidOperationException ("Can't override this constructor.");
			}
			IntPtr filename_as_native = GLib.Marshaller.StringToPtrGStrdup (filename);
			Raw = rc_world_undump_new(filename_as_native);
			GLib.Marshaller.Free (filename_as_native);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_world_undump_get_type();

		public static new GLib.GType GType { 
			get {
				IntPtr raw_ret = rc_world_undump_get_type();
				GLib.GType ret = new GLib.GType(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_world_undump_load(IntPtr raw, IntPtr filename);

		public void Load(string filename) {
			IntPtr filename_as_native = GLib.Marshaller.StringToPtrGStrdup (filename);
			rc_world_undump_load(Handle, filename_as_native);
			GLib.Marshaller.Free (filename_as_native);
		}


		static WorldUndump ()
		{
			GtkSharp.LibredcarpetSharp.ObjectManager.Initialize ();
		}
#endregion
	}
}
