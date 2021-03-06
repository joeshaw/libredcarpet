// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	[StructLayout(LayoutKind.Sequential)]
	public struct QueueItem_Install {

		private IntPtr _parent;
		public RC.QueueItem Parent {
			get {
				return _parent == IntPtr.Zero ? null : (RC.QueueItem) GLib.Opaque.GetOpaque (_parent, typeof (RC.QueueItem), false);
			}
			set {
				_parent = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _package;
		public RC.Package Package {
			get {
				return _package == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (_package, typeof (RC.Package), false);
			}
			set {
				_package = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _upgrades;
		public RC.Package Upgrades {
			get {
				return _upgrades == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (_upgrades, typeof (RC.Package), false);
			}
			set {
				_upgrades = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _deps_satisfied_by_this_install;
		private IntPtr _needed_by;
		public int ChannelPriority;
		public int OtherPenalty;
		private uint _bitfield0;

		[DllImport ("libredcarpetsharpglue-0")]
		extern static bool rcsharp_rc_queueitem_install_get_explicitly_requested (ref RC.QueueItem_Install raw);
		[DllImport ("libredcarpetsharpglue-0")]
		extern static void rcsharp_rc_queueitem_install_set_explicitly_requested (ref RC.QueueItem_Install raw, bool value);
		public bool ExplicitlyRequested {
			get {
				return rcsharp_rc_queueitem_install_get_explicitly_requested (ref this);
			}
			set {
				rcsharp_rc_queueitem_install_set_explicitly_requested (ref this, value);
			}
		}


		public static RC.QueueItem_Install Zero = new RC.QueueItem_Install ();

		public static RC.QueueItem_Install New(IntPtr raw) {
			if (raw == IntPtr.Zero) {
				return RC.QueueItem_Install.Zero;
			}
			RC.QueueItem_Install self = new RC.QueueItem_Install();
			self = (RC.QueueItem_Install) Marshal.PtrToStructure (raw, self.GetType ());
			return self;
		}

		private static GLib.GType GType {
			get { return GLib.GType.Pointer; }
		}
#endregion
	}
}
